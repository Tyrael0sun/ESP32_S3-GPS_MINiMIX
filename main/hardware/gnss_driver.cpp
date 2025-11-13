/**
 * @file gnss_driver.cpp
 * @brief GNSS module driver implementation
 */

#include "gnss_driver.h"
#include "config.h"
#include "esp_log.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include <cstring>
#include <cstdlib>

static const char* TAG = "GNSS";

static bool gnss_initialized = false;
static GnssData latest_data = {};
static QueueHandle_t uart_queue = NULL;

// NMEA parser helper
static bool parse_nmea_gga(const char* sentence);
static bool parse_nmea_rmc(const char* sentence);
static bool parse_nmea_gsv(const char* sentence);
static bool parse_nmea_gsa(const char* sentence);

bool gnss_init(void) {
    // Enable GPS LDO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPS_LDO_EN_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_level((gpio_num_t)GPS_LDO_EN_GPIO, 1);
    
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for power up
    
    // Configure UART with initial baudrate (9600 - GPS module default)
    uart_config_t uart_config = {};
    uart_config.baud_rate = GNSS_UART_BAUDRATE_INIT;  // Start with 9600
    uart_config.data_bits = UART_DATA_8_BITS;
    uart_config.parity = UART_PARITY_DISABLE;
    uart_config.stop_bits = UART_STOP_BITS_1;
    uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    uart_config.source_clk = UART_SCLK_DEFAULT;
    
    ESP_ERROR_CHECK(uart_driver_install(GNSS_UART_NUM, 2048, 2048, 20, &uart_queue, 0));
    ESP_ERROR_CHECK(uart_param_config(GNSS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GNSS_UART_NUM, GNSS_TX_GPIO, GNSS_RX_GPIO, 
                                  UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    
    ESP_LOGI(TAG, "GNSS UART initialized at %d baud", GNSS_UART_BAUDRATE_INIT);
    
    // Wait for GPS module to be ready
    vTaskDelay(pdMS_TO_TICKS(500));
    
    // Configure GPS module to use 115200 baud (NMEA command)
    // PMTK command to set baudrate: $PMTK251,115200*1F\r\n
    const char* baudrate_cmd = "$PMTK251,115200*1F\r\n";
    uart_write_bytes(GNSS_UART_NUM, baudrate_cmd, strlen(baudrate_cmd));
    uart_wait_tx_done(GNSS_UART_NUM, pdMS_TO_TICKS(100));
    
    ESP_LOGI(TAG, "Sent baudrate change command to GPS");
    
    // Wait for command to take effect
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Now switch ESP32 UART to 115200
    uart_config.baud_rate = GNSS_UART_BAUDRATE;
    ESP_ERROR_CHECK(uart_param_config(GNSS_UART_NUM, &uart_config));
    
    ESP_LOGI(TAG, "GNSS module initialized at %d baud", GNSS_UART_BAUDRATE);
    gnss_initialized = true;
    
    return true;
}

bool gnss_read(GnssData* data) {
    if (!gnss_initialized || !data) return false;
    
    uint8_t buffer[512];
    int len = uart_read_bytes(GNSS_UART_NUM, buffer, sizeof(buffer) - 1, pdMS_TO_TICKS(10));
    
    if (len > 0) {
        buffer[len] = '\0';
        
        // Parse NMEA sentences
        char* line = strtok((char*)buffer, "\r\n");
        while (line != NULL) {
            if (strncmp(line, "$GPGGA", 6) == 0 || strncmp(line, "$GNGGA", 6) == 0) {
                parse_nmea_gga(line);
            } else if (strncmp(line, "$GPRMC", 6) == 0 || strncmp(line, "$GNRMC", 6) == 0) {
                parse_nmea_rmc(line);
            } else if (strncmp(line, "$GPGSV", 6) == 0 || strncmp(line, "$GLGSV", 6) == 0 ||
                       strncmp(line, "$GAGSV", 6) == 0 || strncmp(line, "$BDGSV", 6) == 0) {
                parse_nmea_gsv(line);
            } else if (strncmp(line, "$GPGSA", 6) == 0 || strncmp(line, "$GNGSA", 6) == 0) {
                parse_nmea_gsa(line);
            }
            line = strtok(NULL, "\r\n");
        }
    }
    
    *data = latest_data;
    return latest_data.fix_valid;
}

bool gnss_set_rate(uint8_t rate_hz) {
    // Send UBX command to set rate (placeholder)
    ESP_LOGI(TAG, "Setting GNSS rate to %d Hz", rate_hz);
    
    // TODO: Implement UBX protocol for MAX-F10S
    // Wait for ACK/NACK response as per F-SYS-04
    
    return true;
}

bool gnss_set_constellation(bool use_gps, bool use_glonass, bool use_galileo, bool use_beidou) {
    ESP_LOGI(TAG, "Configuring constellations: GPS=%d GLONASS=%d Galileo=%d BeiDou=%d",
             use_gps, use_glonass, use_galileo, use_beidou);
    
    // TODO: Implement UBX protocol
    
    return true;
}

bool gnss_has_fix(void) {
    return latest_data.fix_valid;
}

uint8_t gnss_get_satellites(void) {
    return latest_data.satellites;
}

// Simple NMEA GGA parser
static bool parse_nmea_gga(const char* sentence) {
    char* token;
    char* saveptr;
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    
    int field = 0;
    token = strtok_r(buffer, ",", &saveptr);
    
    while (token != NULL && field < 15) {
        switch (field) {
            case 6: // Fix quality
                latest_data.fix_valid = (atoi(token) > 0);
                break;
            case 7: // Number of satellites
                latest_data.satellites = atoi(token);
                break;
            case 9: // Altitude
                latest_data.altitude = atof(token);
                break;
        }
        token = strtok_r(NULL, ",", &saveptr);
        field++;
    }
    
    return true;
}

// Simple NMEA RMC parser
static bool parse_nmea_rmc(const char* sentence) {
    // Simplified parser - full implementation would parse all fields
    latest_data.timestamp = xTaskGetTickCount() * portTICK_PERIOD_MS;
    return true;
}

// Parse NMEA GSV (Satellites in View)
static bool parse_nmea_gsv(const char* sentence) {
    char* token;
    char* saveptr;
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    
    int field = 0;
    token = strtok_r(buffer, ",", &saveptr);
    
    // Determine constellation type from sentence ID
    ConstellationType constellation = CONSTELLATION_GPS;
    if (strncmp(sentence, "$GLGSV", 6) == 0) {
        constellation = CONSTELLATION_GLONASS;
    } else if (strncmp(sentence, "$GAGSV", 6) == 0) {
        constellation = CONSTELLATION_GALILEO;
    } else if (strncmp(sentence, "$BDGSV", 6) == 0) {
        constellation = CONSTELLATION_BEIDOU;
    }
    
    int total_msgs = 0, msg_num = 0, sats_in_view = 0;
    
    while (token != NULL && field < 20) {
        switch (field) {
            case 1: total_msgs = atoi(token); break;
            case 2: msg_num = atoi(token); break;
            case 3: sats_in_view = atoi(token); break;
            default:
                // Parse satellite info (groups of 4 fields)
                if (field >= 4 && (field - 4) % 4 == 0) {
                    int sat_idx = latest_data.satellites_in_view;
                    if (sat_idx < MAX_SATELLITES) {
                        latest_data.satellites_info[sat_idx].sat_id = atoi(token);
                        latest_data.satellites_info[sat_idx].constellation = constellation;
                    }
                } else if (field >= 5 && (field - 5) % 4 == 0) {
                    int sat_idx = latest_data.satellites_in_view;
                    if (sat_idx < MAX_SATELLITES) {
                        latest_data.satellites_info[sat_idx].elevation = atoi(token);
                    }
                } else if (field >= 6 && (field - 6) % 4 == 0) {
                    int sat_idx = latest_data.satellites_in_view;
                    if (sat_idx < MAX_SATELLITES) {
                        latest_data.satellites_info[sat_idx].azimuth = atoi(token);
                    }
                } else if (field >= 7 && (field - 7) % 4 == 0) {
                    int sat_idx = latest_data.satellites_in_view;
                    if (sat_idx < MAX_SATELLITES && strlen(token) > 0) {
                        latest_data.satellites_info[sat_idx].cn0 = atoi(token);
                        latest_data.satellites_info[sat_idx].status = SAT_TRACKING;
                        latest_data.satellites_in_view++;
                    }
                }
                break;
        }
        token = strtok_r(NULL, ",", &saveptr);
        field++;
    }
    
    return true;
}

// Parse NMEA GSA (DOP and Active Satellites)
static bool parse_nmea_gsa(const char* sentence) {
    char* token;
    char* saveptr;
    char buffer[256];
    strncpy(buffer, sentence, sizeof(buffer) - 1);
    
    int field = 0;
    uint8_t used_sats[12] = {0};
    int used_count = 0;
    
    token = strtok_r(buffer, ",", &saveptr);
    
    while (token != NULL && field < 18) {
        if (field >= 3 && field <= 14 && strlen(token) > 0) {
            // Satellite PRN being used
            used_sats[used_count++] = atoi(token);
        } else if (field == 15) {
            latest_data.pdop = atof(token);
        } else if (field == 16) {
            latest_data.hdop = atof(token);
        } else if (field == 17) {
            latest_data.vdop = atof(token);
        }
        token = strtok_r(NULL, ",", &saveptr);
        field++;
    }
    
    // Mark satellites as used
    for (int i = 0; i < latest_data.satellites_in_view && i < MAX_SATELLITES; i++) {
        bool is_used = false;
        for (int j = 0; j < used_count; j++) {
            if (latest_data.satellites_info[i].sat_id == used_sats[j]) {
                is_used = true;
                break;
            }
        }
        if (is_used) {
            latest_data.satellites_info[i].status = SAT_USED;
        }
    }
    
    return true;
}

uint8_t gnss_get_satellite_info(SatelliteInfo* sat_info, uint8_t max_sats) {
    if (!sat_info) return 0;
    
    uint8_t count = latest_data.satellites_in_view;
    if (count > max_sats) count = max_sats;
    
    for (uint8_t i = 0; i < count; i++) {
        sat_info[i] = latest_data.satellites_info[i];
    }
    
    return count;
}

const char* gnss_get_constellation_name(ConstellationType type) {
    switch (type) {
        case CONSTELLATION_GPS: return "GPS";
        case CONSTELLATION_GLONASS: return "GLO";
        case CONSTELLATION_GALILEO: return "GAL";
        case CONSTELLATION_BEIDOU: return "BDS";
        default: return "UNK";
    }
}
