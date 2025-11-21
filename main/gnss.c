#include "gnss.h"
#include "config.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <ctype.h>

static const char *TAG = "GNSS";
#define BUF_SIZE 2048 // Increased buffer for fragmentation handling

// UBX Constants
#define UBX_SYNC_CHAR_1 0xB5
#define UBX_SYNC_CHAR_2 0x62
#define UBX_CLASS_CFG   0x06
#define UBX_CLASS_ACK   0x05
#define UBX_ID_ACK_ACK  0x01
#define UBX_ID_ACK_NAK  0x00
#define UBX_ID_CFG_VALSET 0x8A

static void send_ubx_msg(uint8_t class, uint8_t id, uint8_t *payload, uint16_t payload_len) {
    uint8_t header[6];
    header[0] = UBX_SYNC_CHAR_1;
    header[1] = UBX_SYNC_CHAR_2;
    header[2] = class;
    header[3] = id;
    header[4] = payload_len & 0xFF;
    header[5] = (payload_len >> 8) & 0xFF;

    uint8_t ck_a = 0, ck_b = 0;

    // Calc header part checksum (starting from Class)
    for(int i=2; i<6; i++) {
        ck_a += header[i];
        ck_b += ck_a;
    }
    // Calc payload part checksum
    for(int i=0; i<payload_len; i++) {
        ck_a += payload[i];
        ck_b += ck_a;
    }

    uart_write_bytes(GNSS_UART_NUM, (const char*)header, 6);
    if (payload_len > 0) {
        uart_write_bytes(GNSS_UART_NUM, (const char*)payload, payload_len);
    }
    uart_write_bytes(GNSS_UART_NUM, (const char*)&ck_a, 1);
    uart_write_bytes(GNSS_UART_NUM, (const char*)&ck_b, 1);
}

static void gnss_configure_baud_rate(void) {
    // U-Blox Generation 9/10 (MAX-F10S) uses CFG-VALSET.
    // Key: CFG-UART1-BAUDRATE = 0x40520001
    // Value: 115200 = 0x0001C200

    // Payload for CFG-VALSET
    // Version (1) + Layers (1) + Reserved (2) + Key (4) + Value (4) = 12 bytes
    uint8_t payload[12];
    memset(payload, 0, 12);

    payload[0] = 0x00; // Version
    payload[1] = 0x01; // Layer: RAM
    payload[2] = 0x00; // Reserved
    payload[3] = 0x00; // Reserved

    // Key: 0x40520001 (Little Endian) -> 01 00 52 40
    payload[4] = 0x01;
    payload[5] = 0x00;
    payload[6] = 0x52;
    payload[7] = 0x40;

    // Value: 115200 = 0x0001C200 (Little Endian) -> 00 C2 01 00
    payload[8] = 0x00;
    payload[9] = 0xC2;
    payload[10] = 0x01;
    payload[11] = 0x00;

    ESP_LOGI(TAG, "Sending U-Blox CFG-VALSET to switch baud rate to 115200...");
    send_ubx_msg(UBX_CLASS_CFG, UBX_ID_CFG_VALSET, payload, 12);

    // Wait for transmission and module processing
    vTaskDelay(pdMS_TO_TICKS(200));

    ESP_LOGI(TAG, "Reconfiguring UART to 115200...");
    uart_set_baudrate(GNSS_UART_NUM, 115200);

    // Flush buffers
    uart_flush_input(GNSS_UART_NUM);
    ESP_LOGI(TAG, "Baud rate switched.");
}

esp_err_t gnss_init(void) {
    ESP_LOGI(TAG, "Initializing GNSS UART...");

    // 1. Configure UART parameters (Default 9600 first)
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
    ESP_ERROR_CHECK(uart_set_pin(GNSS_UART_NUM, GNSS_TX_PIN, GNSS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    // 2. Enable LDO if needed
    gpio_config_t ldo_conf = {
        .pin_bit_mask = (1ULL << GNSS_LDO_EN_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&ldo_conf);
    gpio_set_level(GNSS_LDO_EN_PIN, 1);

    // Give it some time to boot
    vTaskDelay(pdMS_TO_TICKS(500));

    // 3. Switch Baud Rate
    gnss_configure_baud_rate();

    return ESP_OK;
}

// Simple parser state
typedef enum {
    PARSE_IDLE,
    PARSE_NMEA,
    PARSE_UBX_SYNC1,
    PARSE_UBX_CLASS,
    PARSE_UBX_ID,
    PARSE_UBX_LEN1,
    PARSE_UBX_LEN2,
    PARSE_UBX_PAYLOAD,
    PARSE_UBX_CKA,
    PARSE_UBX_CKB
} ParserState;

void gnss_task_entry(void *pvParameters) {
    gnss_init();

    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    uint8_t *ubx_payload = (uint8_t *) malloc(1024); // Max UBX payload
    uint8_t nmea_buf[256];

    ParserState state = PARSE_IDLE;
    int nmea_idx = 0;
    int ubx_idx = 0;
    int ubx_len = 0;
    uint8_t ubx_class = 0;
    uint8_t ubx_id = 0;
    uint8_t ubx_ck_a = 0;
    uint8_t ubx_ck_b = 0;

    while (1) {
        // Read data from UART
        int len = uart_read_bytes(GNSS_UART_NUM, data, BUF_SIZE, pdMS_TO_TICKS(50));
        if (len < 0) continue;

        for (int i = 0; i < len; i++) {
            uint8_t byte = data[i];

            // 1. NMEA Check ($...CRLF)
            if (state == PARSE_IDLE || state == PARSE_NMEA) {
                if (byte == '$') {
                    state = PARSE_NMEA;
                    nmea_idx = 0;
                    nmea_buf[nmea_idx++] = byte;
                } else if (state == PARSE_NMEA) {
                    if (nmea_idx < sizeof(nmea_buf) - 1) {
                        nmea_buf[nmea_idx++] = byte;
                        if (byte == '\n') {
                            nmea_buf[nmea_idx] = 0;
                            // Trim CR LF
                            char *crlf = strpbrk((char*)nmea_buf, "\r\n");
                            if (crlf) *crlf = 0;

                            ESP_LOGI(TAG, "NMEA: %s", nmea_buf);
                            state = PARSE_IDLE;
                        }
                    } else {
                        state = PARSE_IDLE; // Overflow
                    }
                }
            }

            // 2. UBX Check (0xB5 0x62 ...)
            // Note: If we are in NMEA mode, we shouldn't switch unless we see valid UBX start which isn't ASCII.
            // But '$' is not 0xB5.
            if (state == PARSE_IDLE && byte == UBX_SYNC_CHAR_1) {
                state = PARSE_UBX_SYNC1;
            } else if (state == PARSE_UBX_SYNC1) {
                if (byte == UBX_SYNC_CHAR_2) state = PARSE_UBX_CLASS;
                else state = PARSE_IDLE;
            } else if (state == PARSE_UBX_CLASS) {
                ubx_class = byte;
                state = PARSE_UBX_ID;
            } else if (state == PARSE_UBX_ID) {
                ubx_id = byte;
                state = PARSE_UBX_LEN1;
            } else if (state == PARSE_UBX_LEN1) {
                ubx_len = byte;
                state = PARSE_UBX_LEN2;
            } else if (state == PARSE_UBX_LEN2) {
                ubx_len |= (byte << 8);
                ubx_idx = 0;
                if (ubx_len > 1024) state = PARSE_IDLE; // Safety
                else state = PARSE_UBX_PAYLOAD;
            } else if (state == PARSE_UBX_PAYLOAD) {
                if (ubx_idx < ubx_len) {
                    ubx_payload[ubx_idx++] = byte;
                }
                if (ubx_idx == ubx_len) state = PARSE_UBX_CKA;
            } else if (state == PARSE_UBX_CKA) {
                ubx_ck_a = byte;
                state = PARSE_UBX_CKB;
            } else if (state == PARSE_UBX_CKB) {
                ubx_ck_b = byte;
                // Packet Complete, Verify Checksum? (Skipping verification for log-only, but recommended)
                // Log It
                if (ubx_class == UBX_CLASS_ACK) {
                    if (ubx_id == UBX_ID_ACK_ACK) {
                        // Payload: CLS ID of acked message
                        ESP_LOGI(TAG, "UBX ACK-ACK: For Msg 0x%02X-0x%02X", ubx_payload[0], ubx_payload[1]);
                    } else if (ubx_id == UBX_ID_ACK_NAK) {
                        ESP_LOGW(TAG, "UBX ACK-NAK: For Msg 0x%02X-0x%02X", ubx_payload[0], ubx_payload[1]);
                    }
                } else {
                    ESP_LOGI(TAG, "UBX Packet: Class=0x%02X ID=0x%02X Len=%d", ubx_class, ubx_id, ubx_len);
                }
                state = PARSE_IDLE;
            }
        }
    }
    free(data);
    free(ubx_payload);
    vTaskDelete(NULL);
}
