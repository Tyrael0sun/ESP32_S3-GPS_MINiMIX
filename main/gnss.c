#include "gnss.h"
#include "config.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static const char *TAG = "GNSS";
#define BUF_SIZE 2048

volatile gnss_data_t g_gnss_data = {0};

#define UBX_SYNC_CHAR_1 0xB5
#define UBX_SYNC_CHAR_2 0x62
#define UBX_CLASS_CFG   0x06
#define UBX_CLASS_ACK   0x05
#define UBX_ID_CFG_PRT  0x00
#define UBX_ID_CFG_RATE 0x08
#define UBX_ID_CFG_GNSS 0x3E
#define UBX_ID_ACK_ACK  0x01
#define UBX_ID_ACK_NAK  0x00

static void send_ubx_msg(uint8_t class, uint8_t id, uint8_t *payload, uint16_t payload_len) {
    uint8_t header[6];
    header[0] = UBX_SYNC_CHAR_1;
    header[1] = UBX_SYNC_CHAR_2;
    header[2] = class;
    header[3] = id;
    header[4] = payload_len & 0xFF;
    header[5] = (payload_len >> 8) & 0xFF;

    uint8_t ck_a = 0, ck_b = 0;
    for(int i=2; i<6; i++) { ck_a += header[i]; ck_b += ck_a; }
    for(int i=0; i<payload_len; i++) { ck_a += payload[i]; ck_b += ck_a; }

    uart_write_bytes(GNSS_UART_NUM, (const char*)header, 6);
    if (payload_len > 0) uart_write_bytes(GNSS_UART_NUM, (const char*)payload, payload_len);
    uart_write_bytes(GNSS_UART_NUM, (const char*)&ck_a, 1);
    uart_write_bytes(GNSS_UART_NUM, (const char*)&ck_b, 1);
}

static void gnss_configure_baud_rate_legacy(void) {
    // Method 1: Try CFG-PRT (20 bytes) for M8
    uint8_t payload[20];
    memset(payload, 0, 20);
    payload[0] = 0x01; // Port 1
    payload[4] = 0xD0; payload[5] = 0x08; // 8N1
    payload[8] = 0x00; payload[9] = 0xC2; payload[10] = 0x01; // 115200
    payload[12] = 0x03; payload[14] = 0x03; // UBX+NMEA

    ESP_LOGI(TAG, "Sending UBX CFG-PRT (115200)...");
    send_ubx_msg(UBX_CLASS_CFG, UBX_ID_CFG_PRT, payload, 20);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Method 2: Try NMEA $PUBX,41 (Port Config) as backup/alternative
    // $PUBX,41,1,0007,0003,115200,0*1E\r\n
    // 1=UART1, 0007=8N1, 0003=UBX+NMEA, 115200=Baud
    // const char *cmd_pubx = "$PUBX,41,1,0007,0003,115200,0*1E\r\n";
    // Checksum calculation needed if hardcoded string not verified.
    // 41,1,0007,0003,115200,0 -> Checksum
    // Let's rely on UBX CFG-PRT first, but send it multiple times.

    send_ubx_msg(UBX_CLASS_CFG, UBX_ID_CFG_PRT, payload, 20);
    vTaskDelay(pdMS_TO_TICKS(100));
}

static void gnss_configure_rate_5hz(void) {
    uint8_t payload[6] = {0xC8, 0x00, 0x01, 0x00, 0x01, 0x00}; // 200ms
    send_ubx_msg(UBX_CLASS_CFG, UBX_ID_CFG_RATE, payload, 6);
}

static void gnss_configure_constellation(void) {
    // 5 Blocks: GPS(0), SBAS(1), Galileo(2), BeiDou(3), GLONASS(6)
    // 4 (header) + 5*8 (blocks) = 44 bytes
    uint8_t payload[44];
    memset(payload, 0, 44);

    payload[0] = 0x00; payload[1] = 0x20; payload[2] = 0x20; payload[3] = 0x05;
    int offset = 4;

    // GPS Enable
    payload[offset] = 0x00; payload[offset+4] = 0x01; payload[offset+6] = 0x01; offset+=8;
    // SBAS Enable
    payload[offset] = 0x01; payload[offset+4] = 0x01; payload[offset+6] = 0x01; offset+=8;
    // Galileo Disable
    payload[offset] = 0x02; payload[offset+4] = 0x00; payload[offset+6] = 0x01; offset+=8;
    // BeiDou Enable
    payload[offset] = 0x03; payload[offset+4] = 0x01; payload[offset+6] = 0x01; offset+=8;
    // GLONASS Disable
    payload[offset] = 0x06; payload[offset+4] = 0x00; payload[offset+6] = 0x01; offset+=8;

    send_ubx_msg(UBX_CLASS_CFG, UBX_ID_CFG_GNSS, payload, 44);
}

esp_err_t gnss_init(void) {
    ESP_LOGI(TAG, "Init GNSS (9600)...");

    // 1. Init UART at 9600
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    ESP_ERROR_CHECK(uart_driver_install(GNSS_UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(GNSS_UART_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(GNSS_UART_NUM, GNSS_TX_PIN_ESP, GNSS_RX_PIN_ESP, -1, -1));

    gpio_set_direction(GNSS_LDO_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(GNSS_LDO_EN_PIN, 1);

    vTaskDelay(pdMS_TO_TICKS(2000)); // Wait for boot

    // 2. Try to Switch Baud Rate
    gnss_configure_baud_rate_legacy();
    uart_wait_tx_done(GNSS_UART_NUM, pdMS_TO_TICKS(200));

    // 3. Reconfigure ESP32 to 115200
    ESP_LOGI(TAG, "Switching Host UART to 115200...");
    uart_flush_input(GNSS_UART_NUM);
    uart_set_baudrate(GNSS_UART_NUM, 115200);
    vTaskDelay(pdMS_TO_TICKS(100));

    // 4. Configure Rate and Constellation at high speed
    // Note: If baud switch failed, these will be lost (sent at 115200 to a 9600 device).
    // But if switch succeeded, these configure the device.
    gnss_configure_rate_5hz();
    vTaskDelay(pdMS_TO_TICKS(100));
    gnss_configure_constellation();

    return ESP_OK;
}

static void parse_nmea_gga(char *line) {
    // Simple parser logic (same as before)
    char *p = line;
    int idx = 0;
    char *f;
    float lat=0, lon=0;
    int q=0, sats=0;

    while ((f = strsep(&p, ",")) != NULL) {
        if (idx==2 && *f) lat = strtof(f, NULL);
        else if (idx==3 && *f=='S') lat = -lat;
        else if (idx==4 && *f) lon = strtof(f, NULL);
        else if (idx==5 && *f=='W') lon = -lon;
        else if (idx==6 && *f) q = atoi(f);
        else if (idx==7 && *f) sats = atoi(f);
        idx++;
    }

    g_gnss_data.lat = (int)(lat/100) + (lat-(int)(lat/100)*100)/60.0f;
    g_gnss_data.lon = (int)(lon/100) + (lon-(int)(lon/100)*100)/60.0f;
    g_gnss_data.sats = sats;
    g_gnss_data.fix = (q > 0);
}

void gnss_task_entry(void *pvParameters) {
    gnss_init();
    uint8_t *data = malloc(BUF_SIZE);
    char nmea_buf[256];
    int nmea_idx = 0;

    while (1) {
        int len = uart_read_bytes(GNSS_UART_NUM, data, BUF_SIZE, pdMS_TO_TICKS(50));
        if (len <= 0) continue;

        for (int i=0; i<len; i++) {
            char c = (char)data[i];
            if (c == '$') { nmea_idx = 0; }
            if (nmea_idx < 255) nmea_buf[nmea_idx++] = c;
            if (c == '\n') {
                nmea_buf[nmea_idx] = 0;
                if (strstr(nmea_buf, "GGA")) parse_nmea_gga(nmea_buf);
                nmea_idx = 0;
            }
        }
    }
    free(data);
    vTaskDelete(NULL);
}
