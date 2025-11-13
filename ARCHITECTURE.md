# ESP32-S3 GPS MINiMIX - Architecture Overview

## Project Structure

```
ESP32_S3-GPS_MINiMIX/
├── main/
│   ├── config.h                    # Central configuration
│   ├── main.cpp                    # Application entry point
│   ├── hardware/                   # Hardware abstraction layer
│   │   ├── display_driver.cpp/h    # ST7789 LCD driver
│   │   ├── imu_driver.cpp/h        # LSM6DSR IMU
│   │   ├── mag_driver.cpp/h        # LIS2MDL magnetometer
│   │   ├── baro_driver.cpp/h       # BMP388 barometer
│   │   ├── gnss_driver.cpp/h       # GNSS module
│   │   ├── sdcard_driver.cpp/h     # SD card (SDIO)
│   │   ├── encoder_driver.cpp/h    # Rotary encoder + button
│   │   └── battery_monitor.cpp/h   # Battery ADC
│   ├── core/                       # Core functionality
│   │   ├── sensor_fusion.cpp/h     # IMU/GPS fusion
│   │   ├── gps_logger.cpp/h        # GPX logging
│   │   ├── calibration.cpp/h       # Sensor calibration
│   │   └── rtc_manager.cpp/h       # RTC & time sync
│   ├── ui/                         # User interface (LVGL)
│   │   ├── ui_manager.cpp/h        # UI coordinator
│   │   ├── ui_statusbar.cpp/h      # Status bar
│   │   ├── ui_bike_computer.cpp/h  # Bike computer screen
│   │   ├── ui_gps_logger.cpp/h     # GPS logger screen
│   │   ├── ui_pbox.cpp/h           # P-Box screen
│   │   ├── ui_gnss_info.cpp/h      # GNSS information screen
│   │   └── ui_settings.cpp/h       # Settings screen
│   ├── app/                        # Application logic
│   │   ├── bike_computer_app.cpp/h # Bike computer logic
│   │   ├── pbox_app.cpp/h          # P-Box logic
│   │   └── gps_logger_app.cpp/h    # Logger logic
│   └── utils/                      # Utilities
│       └── diagnostics.cpp/h       # System diagnostics
├── CMakeLists.txt                  # CMake config
├── sdkconfig.defaults              # ESP-IDF defaults
├── partitions.csv                  # Partition table
└── build.sh                        # Build script
```

## Key Design Principles

### 1. Modular Architecture
- **Hardware Layer**: Direct hardware access, no business logic
- **Core Layer**: Reusable algorithms (sensor fusion, logging)
- **UI Layer**: Display logic only, minimal state
- **App Layer**: Business logic for each mode

### 2. Centralized Configuration
All hardware pins and constants in `config.h`:
- Easy to modify for hardware revisions
- Single source of truth
- Compile-time constants for optimization

### 3. Axis Transformations
Per hardware specifications:
- **IMU (LSM6DSR)**: X and Z axes inverted, Y unchanged
- **Magnetometer (LIS2MDL)**: X/Y swapped, then Y and Z inverted

### 4. Sensor Calibration
- Stored in NVS (non-volatile storage)
- Loaded on boot
- User-guided calibration through settings

### 5. GPS Logging
- Standard GPX format
- Extended tags for temperature, pressure, G-forces
- Automatic file naming with timestamps

## Task Architecture

### FreeRTOS Tasks:
1. **UI Task** (Priority 5): LVGL updates, display rendering
2. **App Task** (Priority 4): Application logic updates
3. **Diagnostics Task** (Priority 1): System health monitoring
4. **RTC Sync Task** (Priority 2): One-time GPS time sync

## Input Handling

### Rotary Encoder (PCNT)
- Hardware pulse counting for accurate rotation tracking
- No polling overhead

### Button (GPIO ISR)
- **Short press** (<1s): Mode switch (Bike Computer → GPS Logger → P-Box → GNSS Info → ...)
- **Medium press** (1-3s): Start/stop recording
- **Long press** (>3s): Enter/exit settings
- **Double click** (<500ms between): Reserved

## Display Update Strategy

1. Status bar updated every frame (GPS, battery, SD card)
2. Mode-specific content updated at 10Hz
3. LVGL timer handler called in UI task

## Power Management

- Battery voltage via ADC2_CH1
- Charge status via GPIO
- GPS LDO controlled via GPIO14
- Display backlight PWM at 2kHz

## Diagnostics

Per F-DIAG requirements:
- First 5 seconds: High-frequency logging (1Hz)
- After 5 seconds: Low-frequency heartbeat (10s)
- Reports all sensor status and temperatures

## Building the Project

```bash
# Set up ESP-IDF environment
. $HOME/esp/esp-idf/export.sh

# Build
./build.sh

# Flash
idf.py -p /dev/ttyUSB0 flash

# Monitor
idf.py -p /dev/ttyUSB0 monitor
```

## GNSS Information Display

### Screen Layout (240x320)

The GNSS information screen is optimized for the 240x320 LCD display:

**Status Bar (20px)**
- Shared with other screens (GPS, battery, SD card status)

**Line 1 - Position Information (60px)**
- Display: Latitude, Longitude, Altitude, Speed
- Font: 12-14px for readability
- Example: "Lat: 22.54321° Lon: 113.94210°"
          "Alt: 125.5m  Spd: 45.3 km/h"

**Line 2 - Accuracy Information (40px)**
- Display: HDOP, VDOP, PDOP (Dilution of Precision values)
- Font: 10-12px
- Example: "HDOP:1.2 VDOP:1.5 PDOP:1.8"

**Line 3 - Satellite List (200px, scrollable)**
- Header: "ID  TYPE  CN0  STATUS"
- Item format: "123  GPS   45   USE"
- Font: 8-10px monospace for alignment
- Item height: 20px
- Visible items: ~10 satellites
- Color coding:
  - Gray: Searching (CN0 < 20 dBHz)
  - Yellow: Tracking (CN0 20-35 dBHz)
  - Green: Used in fix (CN0 > 35 dBHz)

### Satellite Information Parsing

The system parses NMEA sentences to extract detailed satellite information:

- **GSV (Satellites in View)**: Provides satellite ID, elevation, azimuth, and CN0
- **GSA (DOP and Active Satellites)**: Identifies which satellites are used in the position fix
- Supports multiple constellations:
  - GPS (GPGSV)
  - GLONASS (GLGSV)
  - Galileo (GAGSV)
  - BeiDou (BDGSV)

### UI Design Validation

✅ **Screen dimensions**: 240x320 pixels
✅ **Total layout height**: 20 + 60 + 40 + 200 = 320px (Perfect fit)
✅ **Text width**: All text fits within 240px with appropriate fonts
✅ **Scrolling**: Supported for >10 satellites
✅ **Readability**: Font sizes ensure clear visibility

## UI System Design

### Display Specifications
- **Resolution**: 240 × 320 pixels (portrait mode)
- **Orientation**: 180° rotation (DISP_ROTATION = 2)
- **Driver**: ST7789 LCD via SPI3
- **Framework**: LVGL v8+ (placeholder, to be implemented)

### Unified Layout System
All UI screens follow a consistent 240×320 layout defined in `ui_common.h`:
- **Status Bar**: 20px (shared across all screens)
- **Content Area**: 300px (screen-specific layouts)

**Common Constants** (`ui_common.h`):
```cpp
// Screen dimensions
#define UI_SCREEN_WIDTH  240
#define UI_SCREEN_HEIGHT 320
#define UI_STATUS_BAR_HEIGHT 20

// Font sizes
#define UI_FONT_SIZE_TINY   12
#define UI_FONT_SIZE_SMALL  16
#define UI_FONT_SIZE_NORMAL 20
#define UI_FONT_SIZE_LARGE  32
#define UI_FONT_SIZE_XLARGE 48
#define UI_FONT_SIZE_HUGE   64

// Color scheme
#define UI_COLOR_BG         0x000000  // Black
#define UI_COLOR_TEXT       0xFFFFFF  // White
#define UI_COLOR_PRIMARY    0x00FF00  // Green
#define UI_COLOR_WARNING    0xFFFF00  // Yellow
#define UI_COLOR_DANGER     0xFF0000  // Red
#define UI_COLOR_DIM        0x808080  // Gray

// Layout spacing
#define UI_PADDING_DEFAULT  5
#define UI_MARGIN_SMALL     10
#define UI_MARGIN_NORMAL    20
```

### Screen-Specific Layouts

**1. Bike Computer (MODE_BIKE_COMPUTER)**
- Speed display: 120px (large font 48px)
- Data rows: 4×45px (altitude, distance, time, recording)
- Total: 20+120+180 = 320px ✓

**2. GPS Logger (MODE_GPS_LOGGER)**
- Speed: 100px
- Track visualization: 120px
- Data rows: 2×40px (distance, time)
- Total: 20+100+120+80 = 320px ✓

**3. P-Box (MODE_PBOX)**
- Speed display: 140px (extra large font 64px)
- Timer: 80px (32px font)
- Info rows: 2×40px (target, status)
- Total: 20+140+80+80 = 320px ✓

**4. GNSS Info (MODE_GNSS_INFO)**
- Position: 60px (lat/lon, alt, speed)
- Accuracy: 40px (HDOP/VDOP/PDOP)
- Satellite list: 200px (scrollable, 10 visible)
- Total: 20+60+40+200 = 320px ✓

**5. Settings (MODE_SETTINGS)**
- Menu items: 40px each (7 visible max)
- Scrollable menu list
- Total: 20+280 = 300px (20px buffer)

### Input Handling

**Button Events**:
- Short press (<1s): Mode switching (cycle through 5 modes)
- Medium press (1-3s): Start/stop GPS recording
- Long press (>3s): Enter/exit settings
- Double click (<500ms): Reserved for future use

**Rotary Encoder with Smart Debounce**:
- Rotation: Scroll menus/lists, adjust values
- **500ms Auto-Clear Mechanism**:
  - Monitors count changes continuously
  - If no change for 500ms → auto-clear to zero
  - Prevents drift from mechanical vibration
  - Transparent to application layer
  
**Implementation** (`encoder_driver.cpp`):
```cpp
// State tracking
static int32_t last_encoder_count = 0;
static uint32_t last_encoder_change_time = 0;
#define ENCODER_DEBOUNCE_MS 500

// In encoder_get_count()
if (count != last_count) {
    last_count = count;
    last_change_time = now;
} else if (count != 0 && (now - last_change_time) > 500ms) {
    clear_count();  // Auto-clear after timeout
}
```

### GNSS Satellite Information System

**Data Structures**:
- `SatelliteInfo`: Individual satellite details
  - ID, constellation type, CN0 signal strength
  - Status: SEARCHING/TRACKING/USED
  - Elevation, azimuth angles
- `GnssData`: Extended with VDOP, PDOP, satellite array (max 32)

**NMEA Parsing**:
- **GSV (Satellites in View)**:
  - Parses satellite ID, elevation, azimuth, CN0
  - Supports multi-message sequences
  - Constellation detection from sentence prefix
- **GSA (DOP and Active Satellites)**:
  - Extracts PDOP, HDOP, VDOP values
  - Identifies satellites used in position fix
  - Cross-references with GSV to determine status

**Multi-Constellation Support**:
```
Sentence      Constellation
$GPGSV    →   GPS
$GLGSV    →   GLONASS  
$GAGSV    →   Galileo
$BDGSV    →   BeiDou
```

**Satellite Status Logic**:
1. Appears in GSV → Status: TRACKING
2. Used in GSA fix → Status: USED
3. Not in GSV → Status: SEARCHING

**Color Coding** (defined in `ui_common.h`):
- **Status Colors**:
  - SEARCHING: Gray (0x808080)
  - TRACKING: Yellow (0xFFFF00)
  - USED: Green (0x00FF00)
- **Signal Strength (CN0)**:
  - >45 dBHz: Green (excellent)
  - 35-45: Yellow-Green (good)
  - 25-35: Yellow (medium)
  - 15-25: Orange (weak)
  - <15: Red (poor)
- **Accuracy (HDOP)**:
  - <2.0: Green (excellent)
  - 2.0-5.0: Yellow (good)
  - >5.0: Red (poor)

## Future Enhancements

1. LVGL widget implementation (currently placeholder code exists)
2. UBX protocol for GNSS configuration
3. BMP388 temperature compensation coefficients
4. Power saving modes
5. Wireless data sync (BLE/WiFi)
6. Advanced P-Box features (multiple test profiles, 0-60mph)
7. Satellite sky plot visualization on GNSS info screen
8. Signal strength bar chart for each constellation
9. Custom LVGL themes optimized for 240×320
10. Touch screen support (if hardware upgrade)
