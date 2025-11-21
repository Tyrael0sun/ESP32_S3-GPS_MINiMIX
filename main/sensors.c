#include "sensors.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include <math.h>

static const char *TAG = "SENSORS";

static i2c_master_bus_handle_t bus_handle = NULL;
static i2c_master_dev_handle_t imu_handle = NULL;
static i2c_master_dev_handle_t mag_handle = NULL;
static i2c_master_dev_handle_t baro_handle = NULL;

// Alpha for Low Pass Filter (Gravity isolation)
#define ALPHA_GRAVITY 0.2f
static float g_grav_x = 0.0f, g_grav_y = 0.0f, g_grav_z = 0.0f;
static bool grav_init = false;

// BMP388 Floating Point Calibration Data
struct bmp388_calib_float {
    double T1, T2, T3;
    double P1, P2, P3, P4, P5, P6, P7, P8, P9, P10, P11;
} calib_float;

static esp_err_t i2c_bus_init(void) {
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = false,
    };
    return i2c_new_master_bus(&i2c_mst_config, &bus_handle);
}

static esp_err_t i2c_register_device(uint8_t addr, i2c_master_dev_handle_t *handle) {
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    return i2c_master_bus_add_device(bus_handle, &dev_cfg, handle);
}

static esp_err_t read_register(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t *data) {
    return i2c_master_transmit_receive(handle, &reg, 1, data, 1, -1);
}

static esp_err_t read_registers(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t *data, size_t len) {
    return i2c_master_transmit_receive(handle, &reg, 1, data, len, -1);
}

static esp_err_t write_register(i2c_master_dev_handle_t handle, uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    return i2c_master_transmit(handle, write_buf, sizeof(write_buf), -1);
}

static void bmp388_read_calib_data(void) {
    uint8_t data[21];
    if (read_registers(baro_handle, 0x31, data, 21) == ESP_OK) {
        // Read raw integer coeffs
        uint16_t t1 = (data[1] << 8) | data[0];
        uint16_t t2 = (data[3] << 8) | data[2];
        int8_t   t3 = data[4];
        int16_t  p1 = (data[6] << 8) | data[5];
        int16_t  p2 = (data[8] << 8) | data[7];
        int8_t   p3 = data[9];
        int8_t   p4 = data[10];
        uint16_t p5 = (data[12] << 8) | data[11];
        uint16_t p6 = (data[14] << 8) | data[13];
        int8_t   p7 = data[15];
        int8_t   p8 = data[16];
        int16_t  p9 = (data[18] << 8) | data[17];
        int8_t   p10 = data[19];
        int8_t   p11 = data[20];

        // Convert to floating point with scaling factors (Bosch Datasheet)
        calib_float.T1 = (double)t1 / 0.00390625f; // 2^-8
        calib_float.T2 = (double)t2 / 1073741824.0f; // 2^30
        calib_float.T3 = (double)t3 / 281474976710656.0f; // 2^48

        calib_float.P1 = (double)(p1 - 16384) / 1048576.0f; // (P1-2^14)/2^20
        calib_float.P2 = (double)(p2 - 16384) / 536870912.0f; // (P2-2^14)/2^29
        calib_float.P3 = (double)p3 / 4294967296.0f; // 2^32
        calib_float.P4 = (double)p4 / 137438953472.0f; // 2^37
        calib_float.P5 = (double)p5 / 0.125f; // 2^-3
        calib_float.P6 = (double)p6 / 64.0f; // 2^6
        calib_float.P7 = (double)p7 / 256.0f; // 2^8
        calib_float.P8 = (double)p8 / 32768.0f; // 2^15
        calib_float.P9 = (double)p9 / 281474976710656.0f; // 2^48
        calib_float.P10 = (double)p10 / 281474976710656.0f; // 2^48
        calib_float.P11 = (double)p11 / 36893488147419103232.0f; // 2^65

        ESP_LOGI(TAG, "BMP388 Calibration Loaded (Float)");
    } else {
        ESP_LOGE(TAG, "Failed to read BMP388 Calibration");
    }
}

esp_err_t sensors_init(void) {
    ESP_LOGI(TAG, "Initializing I2C Sensors (New Driver)...");
    ESP_ERROR_CHECK(i2c_bus_init());

    ESP_ERROR_CHECK(i2c_register_device(IMU_I2C_ADDR, &imu_handle));
    ESP_ERROR_CHECK(i2c_register_device(MAG_I2C_ADDR, &mag_handle));
    ESP_ERROR_CHECK(i2c_register_device(BARO_I2C_ADDR, &baro_handle));

    // IMU: LSM6DSR
    write_register(imu_handle, 0x10, 0x38);
    write_register(imu_handle, 0x11, 0x34);

    // Mag: LIS2MDL
    write_register(mag_handle, 0x60, 0x80);
    write_register(mag_handle, 0x62, 0x10);

    // Baro: BMP388
    write_register(baro_handle, 0x1B, 0x33);

    if (sensors_check_imu()) ESP_LOGI(TAG, "IMU detected.");
    else ESP_LOGE(TAG, "IMU not found!");

    if (sensors_check_mag()) ESP_LOGI(TAG, "Mag detected.");
    else ESP_LOGE(TAG, "Mag not found!");

    if (sensors_check_baro()) {
        ESP_LOGI(TAG, "Baro detected.");
        bmp388_read_calib_data();
    } else ESP_LOGE(TAG, "Baro not found!");

    return ESP_OK;
}

bool sensors_check_imu(void) {
    uint8_t who_am_i = 0;
    esp_err_t ret = read_register(imu_handle, 0x0F, &who_am_i);
    return (ret == ESP_OK && who_am_i == LSM6DSR_WHO_AM_I_VAL);
}

bool sensors_check_mag(void) {
    uint8_t who_am_i = 0;
    esp_err_t ret = read_register(mag_handle, 0x4F, &who_am_i);
    return (ret == ESP_OK && who_am_i == LIS2MDL_WHO_AM_I_VAL);
}

bool sensors_check_baro(void) {
    uint8_t who_am_i = 0;
    esp_err_t ret = read_register(baro_handle, 0x00, &who_am_i);
    return (ret == ESP_OK && who_am_i == BMP388_WHO_AM_I_VAL);
}

esp_err_t sensors_read_imu(float *ax, float *ay, float *az, float *gx, float *gy, float *gz, float *temp) {
    uint8_t raw[14];
    esp_err_t ret = read_registers(imu_handle, 0x20, raw, 14);
    if (ret != ESP_OK) return ret;

    int16_t t_raw = (int16_t)(raw[1] << 8 | raw[0]);
    int16_t g_x = (int16_t)(raw[3] << 8 | raw[2]);
    int16_t g_y = (int16_t)(raw[5] << 8 | raw[4]);
    int16_t g_z = (int16_t)(raw[7] << 8 | raw[6]);
    int16_t a_x = (int16_t)(raw[9] << 8 | raw[8]);
    int16_t a_y = (int16_t)(raw[11] << 8 | raw[10]);
    int16_t a_z = (int16_t)(raw[13] << 8 | raw[12]);

    float sensitivity_a = 0.122f / 1000.0f;
    float sensitivity_g = 17.5f / 1000.0f;

    *ax = a_x * sensitivity_a * -1.0f;
    *ay = a_y * sensitivity_a;
    *az = a_z * sensitivity_a * -1.0f;

    *gx = g_x * sensitivity_g * -1.0f;
    *gy = g_y * sensitivity_g;
    *gz = g_z * sensitivity_g * -1.0f;

    *temp = (t_raw / 256.0f) + 25.0f;

    return ESP_OK;
}

esp_err_t sensors_read_mag(float *mx, float *my, float *mz, float *temp) {
    uint8_t raw[8];
    // Read 8 bytes starting from OUTX_L_REG (0x68) to include TEMP_OUT_L_REG (0x6E) and TEMP_OUT_H_REG (0x6F)
    esp_err_t ret = read_registers(mag_handle, 0x68, raw, 8);
    if (ret != ESP_OK) return ret;

    int16_t m_x = (int16_t)(raw[1] << 8 | raw[0]);
    int16_t m_y = (int16_t)(raw[3] << 8 | raw[2]);
    int16_t m_z = (int16_t)(raw[5] << 8 | raw[4]);
    int16_t t_raw = (int16_t)(raw[7] << 8 | raw[6]);

    float sensitivity = 0.15f; // uT

    *mx = m_y * sensitivity;
    *my = m_x * sensitivity * -1.0f;
    *mz = m_z * sensitivity * -1.0f;

    *temp = (t_raw / 8.0f) + 25.0f;

    return ESP_OK;
}

// BMP388 Compensate Float Implementation
static float bmp388_compensate_temp(uint32_t uncomp_temp) {
    double partial_data1;
    double partial_data2;

    partial_data1 = (double)(uncomp_temp - calib_float.T1);
    partial_data2 = (double)(partial_data1 * calib_float.T2);

    return (float)(partial_data2 + (partial_data1 * partial_data1) * calib_float.T3);
}

static float bmp388_compensate_press(uint32_t uncomp_press, float t_lin) {
    double partial_data1;
    double partial_data2;
    double partial_data3;
    double partial_data4;
    double partial_out1;
    double partial_out2;

    partial_data1 = calib_float.P6 * t_lin;
    partial_data2 = calib_float.P7 * (t_lin * t_lin);
    partial_data3 = calib_float.P8 * (t_lin * t_lin * t_lin);
    partial_out1 = calib_float.P5 + partial_data1 + partial_data2 + partial_data3;

    partial_data1 = calib_float.P2 * t_lin;
    partial_data2 = calib_float.P3 * (t_lin * t_lin);
    partial_data3 = calib_float.P4 * (t_lin * t_lin * t_lin);
    partial_out2 = (double)uncomp_press * (calib_float.P1 + partial_data1 + partial_data2 + partial_data3);

    partial_data1 = (double)uncomp_press * (double)uncomp_press;
    partial_data2 = calib_float.P9 + calib_float.P10 * t_lin;
    partial_data3 = partial_data1 * partial_data2;
    partial_data4 = partial_data3 + (double)uncomp_press * (double)uncomp_press * (double)uncomp_press * calib_float.P11;

    return (float)(partial_out1 + partial_out2 + partial_data4);
}

esp_err_t sensors_read_baro(float *pressure, float *temp) {
    uint8_t raw[6];
    esp_err_t ret = read_registers(baro_handle, 0x04, raw, 6);
    if (ret != ESP_OK) return ret;

    uint32_t p_raw = (raw[2] << 16) | (raw[1] << 8) | raw[0];
    uint32_t t_raw = (raw[5] << 16) | (raw[4] << 8) | raw[3];

    float t_lin = bmp388_compensate_temp(t_raw);
    float p_comp = bmp388_compensate_press(p_raw, t_lin);

    *temp = t_lin;
    *pressure = p_comp / 100.0f; // Pa -> hPa

    return ESP_OK;
}

// Derived Calculations

void sensors_calc_gravity_linear(float ax, float ay, float az, float *grav_x, float *grav_y, float *grav_z, float *lin_x, float *lin_y, float *lin_z) {
    if (!grav_init) {
        g_grav_x = ax;
        g_grav_y = ay;
        g_grav_z = az;
        grav_init = true;
    } else {
        g_grav_x = ALPHA_GRAVITY * ax + (1.0f - ALPHA_GRAVITY) * g_grav_x;
        g_grav_y = ALPHA_GRAVITY * ay + (1.0f - ALPHA_GRAVITY) * g_grav_y;
        g_grav_z = ALPHA_GRAVITY * az + (1.0f - ALPHA_GRAVITY) * g_grav_z;
    }

    *grav_x = g_grav_x;
    *grav_y = g_grav_y;
    *grav_z = g_grav_z;

    *lin_x = ax - g_grav_x;
    *lin_y = ay - g_grav_y;
    *lin_z = az - g_grav_z;
}

float sensors_calc_heading(float mx, float my) {
    // Simple Atan2 heading (requires tilt compensation for accuracy, but basic here)
    float heading = atan2f(my, mx) * 180.0f / M_PI;
    if (heading < 0) heading += 360.0f;
    return heading;
}

float sensors_calc_altitude(float pressure_hpa, float temp_c) {
    // Hypsometric formula
    // h = ((P0/P)^(1/5.257) - 1) * (T + 273.15) / 0.0065
    const float P0 = 1013.25f;
    return 44330.0f * (1.0f - powf(pressure_hpa / P0, 0.1903f));
}
