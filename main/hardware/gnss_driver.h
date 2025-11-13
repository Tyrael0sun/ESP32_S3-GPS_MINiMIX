/**
 * @file gnss_driver.h
 * @brief GNSS module (MAX-F10S/ATGM336H) driver
 */

#ifndef GNSS_DRIVER_H
#define GNSS_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

enum SatelliteStatus {
    SAT_SEARCHING = 0,
    SAT_TRACKING,
    SAT_USED
};

enum ConstellationType {
    CONSTELLATION_GPS = 0,
    CONSTELLATION_GLONASS,
    CONSTELLATION_GALILEO,
    CONSTELLATION_BEIDOU,
    CONSTELLATION_UNKNOWN
};

struct SatelliteInfo {
    uint8_t sat_id;                 // Satellite ID/PRN
    ConstellationType constellation; // Constellation type
    uint8_t cn0;                    // CN0 (dBHz)
    SatelliteStatus status;         // Current status
    uint8_t elevation;              // Elevation angle (degrees)
    uint16_t azimuth;               // Azimuth angle (degrees)
};

#define MAX_SATELLITES 32

struct GnssData {
    bool fix_valid;
    uint8_t satellites;
    double latitude;        // degrees
    double longitude;       // degrees
    float altitude;         // m
    float speed;            // km/h
    float heading;          // degrees
    float hdop;             // Horizontal dilution of precision
    float vdop;             // Vertical dilution of precision
    float pdop;             // Position dilution of precision
    uint32_t timestamp;     // ms
    
    // Time info
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t day;
    uint8_t month;
    uint16_t year;
    
    // Satellite details
    uint8_t satellites_in_view;
    SatelliteInfo satellites_info[MAX_SATELLITES];
};

/**
 * @brief Initialize GNSS module
 * @return true on success, false on failure
 */
bool gnss_init(void);

/**
 * @brief Read latest GNSS data
 * @param data Pointer to GnssData structure
 * @return true if data is valid, false otherwise
 */
bool gnss_read(GnssData* data);

/**
 * @brief Set GNSS update rate
 * @param rate_hz Update rate in Hz (1, 5, 10, 25)
 * @return true on success, false on failure
 */
bool gnss_set_rate(uint8_t rate_hz);

/**
 * @brief Configure GNSS constellation
 * @param use_gps Enable GPS
 * @param use_glonass Enable GLONASS
 * @param use_galileo Enable Galileo
 * @param use_beidou Enable BeiDou
 * @return true on success, false on failure
 */
bool gnss_set_constellation(bool use_gps, bool use_glonass, bool use_galileo, bool use_beidou);

/**
 * @brief Check if GNSS has valid fix
 * @return true if fix is valid
 */
bool gnss_has_fix(void);

/**
 * @brief Get number of satellites in view
 * @return Number of satellites
 */
uint8_t gnss_get_satellites(void);

/**
 * @brief Get detailed satellite information
 * @param sat_info Array to store satellite info
 * @param max_sats Maximum number of satellites to retrieve
 * @return Number of satellites retrieved
 */
uint8_t gnss_get_satellite_info(SatelliteInfo* sat_info, uint8_t max_sats);

/**
 * @brief Get constellation name string
 * @param type Constellation type
 * @return Constellation name
 */
const char* gnss_get_constellation_name(ConstellationType type);

#endif // GNSS_DRIVER_H
