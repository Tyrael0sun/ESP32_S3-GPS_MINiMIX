/**
 * @file config.h
 * @brief Central configuration file for ESP32-S3 GPS MINiMIX
 * 
 * All hardware pin definitions and system constants
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

// ==================== Debug UART ====================
#define DEBUG_TX_GPIO           43
#define DEBUG_RX_GPIO           44
#define DEBUG_UART_NUM          UART_NUM_0
#define DEBUG_UART_BAUDRATE     115200

// ==================== Display (ST7789) ====================
#define DISP_SCK_GPIO           5
#define DISP_MOSI_GPIO          8
#define DISP_CS_GPIO            7
#define DISP_DC_GPIO            6
#define DISP_RST_GPIO           4
#define DISP_BL_GPIO            9
#define DISP_SPI_HOST           SPI3_HOST
#define DISP_WIDTH              240
#define DISP_HEIGHT             320
#define DISP_ROTATION           2       // 180 degrees
#define DISP_BL_FREQ_HZ         2000
#define DISP_BL_DEFAULT_DUTY    50      // 50%

// ==================== GNSS Module ====================
#define GNSS_TX_GPIO            17
#define GNSS_RX_GPIO            18
#define GNSS_UART_NUM           UART_NUM_1
#define GNSS_UART_BAUDRATE      115200
#define GPS_LDO_EN_GPIO         14

// ==================== I2C Bus ====================
#define I2C_SCL_GPIO            (gpio_num_t)39
#define I2C_SDA_GPIO            (gpio_num_t)40
#define I2C_NUM                 I2C_NUM_0
#define I2C_FREQ_HZ             1000000  // 1MHz

// ==================== Sensor Addresses ====================
#define LSM6DSR_I2C_ADDR        0xD5    // IMU (ACC + GYRO)
#define LIS2MDL_I2C_ADDR        0x3D    // Magnetometer
#define BMP388_I2C_ADDR         0xED    // Barometer

// ==================== Sensor Interrupts ====================
#define ACCGYRO_INT_GPIO        41      // Not used
#define MAG_INT_GPIO            42      // Not used
#define PRESS_INT_GPIO          13      // Not used

// ==================== SD Card (SDIO 4-bit) ====================
#define SD_D1_GPIO              38
#define SD_D0_GPIO              37
#define SD_CLK_GPIO             36
#define SD_CMD_GPIO             35
#define SD_D2_GPIO              34
#define SD_D3_GPIO              33
#define SD_MOUNT_POINT          "/sdcard"
#define GPX_DIR                 "/sdcard/GPX"

// ==================== Input (Encoder + Key) ====================
#define ENC_A_GPIO              1
#define ENC_B_GPIO              3
#define KEY_MAIN_GPIO           2

// Debounce and timing
#define KEY_DEBOUNCE_MS         100
#define KEY_SHORT_PRESS_MS      1000
#define KEY_MEDIUM_PRESS_MS     3000
#define KEY_LONG_PRESS_MS       3000
#define KEY_DOUBLE_CLICK_MS     500

// ==================== Power Management ====================
#define BAT_ADC_GPIO            12
#define BAT_ADC_CHANNEL         ADC_CHANNEL_1
#define BAT_ADC_UNIT            ADC_UNIT_2
#define CHRG_STATUS_GPIO        21

// Battery voltage calculation (1:1 divider)
#define BAT_VOLTAGE_DIVIDER     1.0f
#define BAT_MIN_VOLTAGE         3.0f
#define BAT_MAX_VOLTAGE         4.2f

// ==================== P-Box Thresholds ====================
#define PBOX_START_SPEED_KMPH   1.0f
#define PBOX_START_ACCEL_G      0.15f
#define PBOX_TARGET_SPEED_MIN   0.0f
#define PBOX_TARGET_SPEED_MAX   100.0f
#define PBOX_TARGET_SPEED_DEFAULT_START   0.0f
#define PBOX_TARGET_SPEED_DEFAULT_END     100.0f

// ==================== Diagnostics ====================
#define DIAG_FAST_LOG_DURATION_MS   5000    // 5 seconds
#define DIAG_FAST_LOG_INTERVAL_MS   1000    // Every 1 second
#define DIAG_SLOW_LOG_INTERVAL_MS   10000   // Every 10 seconds

// ==================== GNSS Configuration ====================
#define GNSS_DEFAULT_RATE_HZ    1
#define GNSS_MIN_RATE_HZ        1
#define GNSS_MAX_RATE_HZ        25

// ==================== Calibration ====================
#define NVS_NAMESPACE           "calib"
#define NVS_KEY_ACC_OFFSET      "acc_off"
#define NVS_KEY_MAG_OFFSET      "mag_off"
#define NVS_KEY_MAG_SCALE       "mag_scl"

// ==================== Task Priorities ====================
#define TASK_PRIORITY_UI        5
#define TASK_PRIORITY_SENSOR    4
#define TASK_PRIORITY_GPS       3
#define TASK_PRIORITY_LOGGER    2
#define TASK_PRIORITY_DIAG      1

// ==================== Stack Sizes ====================
// Note: Increased stack sizes to prevent overflow in ESP-IDF v6.x
#define STACK_SIZE_UI           8192  // UI and display updates
#define STACK_SIZE_SENSOR       4096  // Sensor fusion
#define STACK_SIZE_GPS          4096  // GPS data processing
#define STACK_SIZE_LOGGER       4096  // File I/O operations
#define STACK_SIZE_DIAG         4096  // Diagnostics with multiple sensor reads (was 2048, caused overflow)

#endif // CONFIG_H
