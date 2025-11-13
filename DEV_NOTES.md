# Development Notes

## Recent Updates (2025-11-13)

### UI System Improvements
- ✅ 创建 `ui_common.h` 统一UI常量定义
- ✅ 所有UI界面适配240×320分辨率（竖屏模式）
- ✅ 创建详细UI布局文档 `docs/UI_LAYOUT_240x320.md`
- ✅ 定义统一颜色方案和字体大小规范
- ✅ 每个UI屏幕添加精确布局说明和像素计算

### Input System Improvements  
- ✅ 旋转编码器添加500ms自动清零消抖机制
- ✅ 防止机械振动导致的计数漂移
- ✅ 更新 `encoder_driver.cpp` 和 `encoder_driver.h`
- ✅ 新增需求 F-INPUT-04

### Screen Layouts Verified
- ✅ 状态栏: 20px (GPS/电池/SD卡图标)
- ✅ 自行车码表: 20+120+180 = 320px
- ✅ GPS记录器: 20+100+120+80 = 320px  
- ✅ P-Box: 20+140+80+80 = 320px
- ✅ GNSS信息: 20+60+40+200 = 320px
- ✅ 设置菜单: 20+280 = 300px (含20px缓冲)

---

## Hardware Verification Checklist

### I2C Devices
- [ ] LSM6DSR (0xD5): WHO_AM_I = 0x6B
- [ ] LIS2MDL (0x3D): WHO_AM_I = 0x40
- [ ] BMP388 (0xED): CHIP_ID = 0x50

### GPIO Tests
- [ ] Display backlight PWM working
- [ ] GPS LDO enable/disable
- [ ] SD card detection
- [ ] Button press detection
- [ ] Encoder rotation counting
- [ ] Battery ADC reading

### Communication Tests
- [ ] UART0 (Debug): 115200 bps
- [ ] UART1 (GNSS): 115200 bps, NMEA sentences received
- [ ] SPI3 (Display): Commands sent successfully
- [ ] SDIO (SD card): File read/write working

## Current Implementation Status

### Completed ✅
- [x] Project structure and build system
- [x] All hardware driver interfaces
- [x] Sensor fusion algorithm (complementary filter)
- [x] GPS logger with GPX format
- [x] Calibration system with NVS storage
- [x] RTC manager with GPS sync
- [x] Diagnostics system
- [x] Input handling (encoder + button)
- [x] Battery monitoring
- [x] Four app modes (bike computer, GPS logger, P-Box, GNSS info)
- [x] UI architecture (placeholders for LVGL)
- [x] GNSS satellite information parsing (GSV/GSA sentences)
- [x] Multi-constellation support (GPS/GLONASS/Galileo/BeiDou)
- [x] GNSS information display UI with optimized layout for 240x320 LCD

### TODO / Known Issues ⚠️

1. **LVGL Integration**
   - Need to integrate LovyanGFX or ESP_LCD for ST7789
   - Create actual LVGL widgets for each screen
   - Implement touch-free navigation (encoder-based)
   - Implement satellite list scrolling with LVGL table/list widget
   - Add color coding for satellite status (gray/yellow/green)
   - Apply ui_common.h color definitions to LVGL styles

2. **GNSS UBX Protocol**
   - Currently only basic NMEA parsing
   - Need full UBX implementation for:
     - Rate configuration with ACK/NACK
     - Constellation configuration
     - Better time/position accuracy

3. **BMP388 Calibration**
   - Current implementation uses raw values
   - Need to read and apply calibration coefficients

4. **Sensor Axis Testing**
   - Verify axis transformations with real hardware
   - May need adjustment based on PCB orientation

5. **Power Optimization**
   - Implement sleep modes
   - GPS power cycling when not in use
   - Display dimming/off timeout

6. **Error Handling**
   - Add watchdog timer
   - Recovery from sensor failures
   - SD card hot-plug handling

---

## Design Changes Log

### 2025-11-13: UI系统统一适配 (v0.0.2)

**变更原因**: 确保所有UI界面完美适配240×320分辨率LCD

**新增文件**:
- `main/ui/ui_common.h` - 统一UI常量定义
  - 屏幕尺寸 (240×320)
  - 字体大小 (8px-64px)
  - 间距规范 (5px/10px/15px)
  - 颜色方案 (GPS、卫星状态、信号强度等)

**修改文件**:
- `main/ui/ui_bike_computer.cpp` - 添加精确布局: 20+120+45×4=320px
- `main/ui/ui_gps_logger.cpp` - 添加精确布局: 20+100+120+40×2=320px
- `main/ui/ui_pbox.cpp` - 添加精确布局: 20+140+80+40×2=320px
- `main/ui/ui_gnss_info.cpp` - 已验证布局: 20+60+40+200=320px
- `main/ui/ui_settings.cpp` - 菜单布局: 20+40×7=300px
- `main/ui/ui_statusbar.cpp` - 状态栏布局: 240px宽

**设计决策**:
- 所有UI文件统一引用 `ui_common.h`
- 每个屏幕精确计算布局，确保总高度=320px
- 使用宏定义替代硬编码数值
- 添加详细的布局计算注释

**影响范围**:
- 提升UI系统可维护性
- 方便后续LVGL实现
- 零性能开销（编译时常量）

### 2025-11-13: 旋转编码器消抖机制 (v0.0.2)

**变更原因**: 防止机械振动导致的编码器计数漂移

**问题描述**:
- 快速旋转后计数值不归零
- 机械振动产生误触发
- 影响菜单滚动和数值调整

**解决方案**:
- 实现500ms自动清零机制
- 监控计数变化，无变化时清零
- 对上层应用透明

**修改文件**:
- `main/hardware/encoder_driver.cpp`
  - 新增变量: `last_encoder_count`, `last_encoder_change_time`
  - 新增宏: `ENCODER_DEBOUNCE_MS = 500`
  - 修改: `encoder_get_count()` 函数添加消抖逻辑
- `main/hardware/encoder_driver.h`
  - 更新函数注释说明自动清零功能

**技术实现**:
```cpp
// 伪代码
if (count != last_count) {
    last_count = count;
    reset_timer();
} else if (count != 0 && timeout > 500ms) {
    clear_count();
}
```

**测试建议**:
- 快速旋转编码器，验证500ms后自动归零
- 慢速旋转，验证正常计数不受影响
- 实际使用中测试菜单滚动体验

### 2025-11-13: GNSS卫星信息显示 (v0.0.1)

**变更原因**: 增加专业GNSS诊断功能

**新增功能**:
1. GNSS驱动增强
   - 新增 `SatelliteInfo` 结构体
   - 扩展 `GnssData` 添加VDOP/PDOP/卫星数组
   - 实现 GSV 句子解析（卫星视图）
   - 实现 GSA 句子解析（DOP和活跃卫星）
   - 支持多星座识别（GPS/GLONASS/Galileo/BeiDou）

2. UI界面设计
   - 定位信息: 经纬度、高度、速度 (60px)
   - 精度信息: HDOP/VDOP/PDOP (40px)
   - 卫星列表: 可滚动，最多32颗 (200px)
   - 颜色编码: 搜索(灰)/跟踪(黄)/使用(绿)

3. 模式集成
   - 添加 MODE_GNSS_INFO 到UI管理器
   - 更新模式切换循环
   - 集成到主程序更新逻辑

**文件变更**:
- `main/hardware/gnss_driver.h/cpp` - 卫星信息解析
- `main/ui/ui_gnss_info.h/cpp` - GNSS界面实现
- `main/ui/ui_manager.h/cpp` - 模式集成
- `main/main.cpp` - 更新主循环
- `main/CMakeLists.txt` - 添加源文件

**设计亮点**:
- 精确的240×320布局设计
- 支持>10颗卫星的滚动显示
- 多星座自动识别
- 卫星状态实时更新

**已知限制**:
- LVGL widgets为占位符，待实现
- 滚动功能需LVGL集成
- 颜色编码需LVGL样式系统

## Testing Procedures

### IMU Calibration Test
1. Place device on flat surface
2. Enter settings → IMU Calibration
3. Keep stationary for 10 seconds
4. Verify Z-axis reads ~9.81 m/s²

### Magnetometer Calibration Test
1. Enter settings → Mag Calibration
2. Rotate device in figure-8 pattern
3. Continue for ~10 seconds
4. Check heading accuracy with compass

### P-Box Auto-Start Test
1. Enter P-Box mode
2. Simulate forward acceleration >0.15G
3. Verify test starts automatically
4. Accelerate to 100 km/h
5. Verify test stops and shows time

### GPS Logger Test
1. Get GPS fix (status bar shows green)
2. Press and hold button (medium press)
3. Verify recording indicator (red flashing)
4. Move around for test track
5. Stop recording
6. Check GPX file in /sdcard/GPX/

### GNSS Information Display Test
1. Short press to cycle to GNSS info mode
2. Verify position display (Lat/Lon/Alt/Speed)
3. Check accuracy values (HDOP/VDOP/PDOP)
4. Verify satellite list shows:
   - Satellite ID and constellation type
   - CN0 signal strength
   - Status (SRCH/TRK/USE)
5. Test scrolling if >10 satellites visible
6. Verify color coding for different satellite states

## Debug Commands

```bash
# Monitor serial output
idf.py monitor

# Filter diagnostics
idf.py monitor | grep DIAG

# Check NVS contents
idf.py partition-table
idf.py nvs-partition-gen

# Erase flash completely
idf.py erase-flash
```

## Performance Targets

- UI refresh rate: 30 FPS minimum
- Sensor fusion update: 100 Hz
- GPS logging rate: 10 Hz
- Button debounce: 100 ms
- Battery read interval: 1 Hz

## Memory Usage Estimates

- LVGL buffer: ~150 KB (double buffer for 240x320x16bit)
- FreeRTOS tasks: ~50 KB
- GPX file buffer: 4 KB
- Total PSRAM usage: ~250 KB / 2 MB available

## Known Hardware Limitations

1. **ESP32-S3 ADC2**: 
   - Cannot use while WiFi active
   - Battery monitoring uses ADC2_CH1
   
2. **SDIO pins**: 
   - Conflicts with JTAG on some pins
   - Use USB serial for debugging when SD card active

3. **I2C speed**:
   - 1 MHz may be too fast for some sensors
   - Reduce to 400 kHz if communication errors occur
