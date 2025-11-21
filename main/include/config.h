#ifndef CONFIG_H
#define CONFIG_H

#include <driver/gpio.h>

// GNSS UART
// Spec: GNSS_TX / GNSS_RX / GPS_LDO_EN | 17 / 18 / 14
// This usually means Pin 17 is GNSS_TX (Output from GNSS) and 18 is GNSS_RX (Input to GNSS)
// For ESP32 UART Config:
// uart_set_pin(uart_num, TX_PIN, RX_PIN, ...)
// TX_PIN should be ESP32 TX (Connected to GNSS RX)
// RX_PIN should be ESP32 RX (Connected to GNSS TX)
// So ESP32_TX = 18 (GNSS_RX), ESP32_RX = 17 (GNSS_TX)
// Wait, if "GNSS_TX" means the pin label on the schematic is "GNSS_TX", it usually connects to ESP32 RX.
// Let's assume the labels in the spec refer to the functionality on the ESP32 side (common in GPIO tables).
// "GNSS_TX" -> Pin 17. If this is ESP32's Pin 17 driving GNSS TX line? No, GNSS TX drives ESP32 RX.
// Usually:
// Pin 17 = GNSS_TX (The signal coming FROM GNSS). So ESP32 should treat 17 as RX.
// Pin 18 = GNSS_RX (The signal going TO GNSS). So ESP32 should treat 18 as TX.
// Let's swap definitions to match ESP32 perspective for uart_set_pin(tx, rx).
// UART_TX = 18
// UART_RX = 17
#define GNSS_UART_NUM       UART_NUM_1
#define GNSS_TX_PIN_ESP     18
#define GNSS_RX_PIN_ESP     17
#define GNSS_LDO_EN_PIN     14
#define GNSS_BAUD_RATE      115200

// I2C
#define I2C_MASTER_NUM      I2C_NUM_0
#define I2C_SCL_PIN         39
#define I2C_SDA_PIN         40
#define I2C_MASTER_FREQ_HZ  1000000

// SPI Display
#define DISP_SPI_HOST       SPI3_HOST
#define DISP_SCK_PIN        5
#define DISP_MOSI_PIN       8
#define DISP_CS_PIN         7
#define DISP_DC_PIN         6
#define DISP_RST_PIN        4
#define DISP_BL_PIN         9

// SD Card (4-bit SDIO)
// Based on spec: SD_D0~D3 / CMD / CLK / D1 | 37-34 / 35 / 36 / 38
// Interpreted as:
// CMD: 35, CLK: 36
// D0: 37, D1: 38, D3: 34
// D2: Not explicitly listed, assuming 33 (common adjacent pin) or needs verification.
#define SD_CMD_PIN          35
#define SD_CLK_PIN          36
#define SD_D0_PIN           37
#define SD_D1_PIN           38
#define SD_D2_PIN           33 // TODO: Verify D2 pin. Common mapping suggests 33.
#define SD_D3_PIN           34

// Encoder & Buttons
#define ENC_A_PIN           1
#define ENC_B_PIN           3
#define KEY_MAIN_PIN        2

// Debug UART
#define DEBUG_UART_NUM      UART_NUM_0
#define DEBUG_TX_PIN        43
#define DEBUG_RX_PIN        44

// Battery
#define BAT_ADC_PIN         12
#define CHRG_STATUS_PIN     21

// Sensors
#define IMU_I2C_ADDR        0x6A
#define MAG_I2C_ADDR        0x1E
#define BARO_I2C_ADDR       0x76

#endif // CONFIG_H
