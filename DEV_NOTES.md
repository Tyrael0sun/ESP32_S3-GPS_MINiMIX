# Development Notes

## Recent Updates (2025-11-13)

### LVGL v8.3 Integration (v0.0.3)
- ✅ 完整集成LVGL v8.3通过ESP-IDF Component Manager
- ✅ ST7789显示驱动基于ESP LCD API (esp_lcd component)
- ✅ 双缓冲DMA传输，240×40像素/缓冲
- ✅ 实现所有5个UI界面 (Bike Computer, GPS Logger, P-Box, GNSS Info, Settings)
- ✅ 状态栏跨界面持久化，带自动清理机制
- ✅ 固件大小: 660.8KB (37%剩余空间)

### Critical Bug Fixes (v0.0.3)
- ✅ **Stack Overflow**: key_event任务栈从2048增至4096字节
- ✅ **Encoder Not Working**: GPIO1/3添加上拉电阻配置
- ✅ **Interface Switching Crash**: 状态栏重新初始化前自动清理
- ✅ **Encoder Sensitivity**: 实现3-step阈值过滤 (±3 steps = ±1 output)
- ✅ **LCD Test Text**: 移除启动时测试文本，干净显示

### Input System Improvements  
- ✅ 旋转编码器3-step阈值 + 500ms自动清零消抖机制
- ✅ GPIO pull-up配置在PCNT初始化前完成
- ✅ 手动计数重置在主循环处理后执行
- ✅ 防止机械振动导致的计数漂移
- ✅ 更新 `encoder_driver.cpp` 和 `encoder_driver.h`
- ✅ 新增需求 F-INPUT-04, F-INPUT-05

### UI System Improvements
- ✅ 创建 `ui_common.h` 统一UI常量定义
- ✅ 所有UI界面适配240×320分辨率（竖屏模式）
- ✅ 创建详细UI布局文档 `docs/UI_LAYOUT_240x320.md`
- ✅ 定义统一颜色方案和字体大小规范
- ✅ 每个UI屏幕添加精确布局说明和像素计算

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

1. **LVGL细化 (部分完成)** ✅
   - ✅ LVGL v8.3已集成，基于ESP LCD API
   - ✅ 所有5个UI界面已实现
   - ✅ 编码器导航已完成 (3-step threshold)
   - ⚠️ 卫星列表滚动需LVGL table/list widget优化
   - ⚠️ 颜色编码需应用到LVGL样式系统
   - ⚠️ GPS轨迹图canvas可视化待实现

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

### 2025-11-13: LVGL v8.3完整集成 (v0.0.3)

**变更原因**: LCD显示实际工作，完成从占位符到完整LVGL系统的迁移

**集成方案**:
- 使用ESP-IDF Component Manager添加LVGL v8.3依赖 (idf_component.yml)
- 基于ESP LCD API实现ST7789驱动 (esp_lcd_panel_io, esp_lcd_panel_ops)
- 双缓冲策略: 2个240×40像素缓冲区，DMA传输
- 刷新回调: lvgl_flush_cb()调用trans_done通知，lv_disp_flush_ready()完成

**关键代码**:
```cpp
// display_driver.cpp
esp_lcd_panel_handle_t panel_handle = NULL;
lv_disp_draw_buf_t disp_buf;
lv_disp_drv_t disp_drv;

// Double buffer allocation
lv_color_t* buf1 = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
lv_disp_draw_buf_init(&disp_buf, buf1, buf2, SCREEN_WIDTH * BUF_HEIGHT);
```

**文件变更**:
- `main/idf_component.yml` - 添加lvgl^8依赖
- `main/hardware/display_driver.cpp` - 完全重写ST7789驱动
- `main/ui/ui_*.cpp` - 所有UI界面从占位符改为实际LVGL widgets
- `main/config.h` - 添加DIAG_SLOW_LOG_INTERVAL_MS=5000

**编译结果**:
- 二进制大小: 660.8KB (0xa1350)
- 分区剩余: 37% (0x5ecb0)
- 零警告编译

---

### 2025-11-13: 关键Bug修复清单 (v0.0.3)

#### Bug #1: Stack Overflow in key_event Task

**问题描述**:
```
***ERROR*** A stack overflow in task key_event has been detected.
Backtrace: 0x4037fc5e:0x3fcaeff0 ...
```

**根本原因**: 
- key_event任务栈仅2048字节
- diagnostics_trigger()函数调用诊断系统需要更多栈空间
- 短按/长按均触发diagnostics_trigger()导致栈溢出

**解决方案**:
```cpp
// encoder_driver.cpp
xTaskCreate(key_event_task, "key_event", 4096, NULL, 3, NULL);  // 2048 → 4096
```

**验证**: 反复短按/长按测试，无重启

---

#### Bug #2: Encoder No Log Output

**问题描述**:
- 旋转编码器物理旋转，但主循环encoder_get_count()始终返回0
- 无任何log输出

**根本原因1 - 自动清零逻辑冲突**:
- encoder_get_count()内部500ms自动清零
- 主循环100ms调用，但清零在读取前发生
- 导致计数被清除后才被读取

**解决方案1**: 
- 移除encoder_get_count()内的自动清零
- 改为主循环手动重置: pcnt_counter_clear(encoder_pcnt_unit)

**根本原因2 - GPIO未配置上拉**:
- PCNT初始化后GPIO未配置pull-up
- 浮空输入导致信号不稳定

**解决方案2**:
```cpp
// encoder_driver.cpp - 在PCNT初始化前
gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << ENC_A_GPIO) | (1ULL << ENC_B_GPIO),
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_DISABLE,
};
gpio_config(&io_conf);
```

**验证**: 旋转编码器log正常输出，计数准确

---

#### Bug #3: Interface Switching Crash

**问题描述**:
```
Guru Meditation Error: Core 0 panic'ed (LoadProhibited)
Exception was unhandled.
Core 0 register dump:
PC: 0x42068927  PS: 0x00060833  A0: 0x8205f5bc  A1: 0x3fcaf990
EXCVADDR: 0x00000008  (Load from invalid address 0x8)
```

**根本原因**:
- 状态栏widgets在模式切换时被多次创建
- 未检查statusbar_cont是否已存在
- 导致野指针访问

**解决方案**:
```cpp
// ui_statusbar.cpp
void ui_statusbar_init(void) {
    if (statusbar_cont != NULL) {
        lv_obj_del(statusbar_cont);  // Auto-cleanup
        statusbar_cont = NULL;
    }
    statusbar_cont = lv_obj_create(lv_scr_act());
    // ... rest of initialization
}
```

**验证**: 反复切换5个界面，无崩溃

---

#### Bug #4: LCD Test Text Not Removed

**问题描述**:
- LCD初始化完成后显示"ESP32-S3 GPS\nMINiMIX\nReady!"
- 用户要求启动直接进入UI，无测试文本

**解决方案**:
```cpp
// display_driver.cpp - display_init()
// 删除以下代码:
// lv_obj_t* label = lv_label_create(lv_scr_act());
// lv_label_set_text(label, "ESP32-S3 GPS\nMINiMIX\nReady!");
// lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);

// 仅保留黑色背景:
lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x000000), LV_PART_MAIN);
```

**验证**: 启动后直接显示状态栏+自行车码表UI

---

#### Bug #5: Encoder Too Sensitive

**问题描述**:
- 旋转编码器每个物理click都触发界面切换
- 用户体验：轻微转动导致误操作

**需求**: "旋转3个step才认为是一次操作，并默认500ms没有操作清零计数器"

**解决方案**:
```cpp
// encoder_driver.cpp
#define ENCODER_STEP_THRESHOLD  3
#define ENCODER_AUTO_CLEAR_MS   500

static int32_t last_encoder_count = 0;
static uint32_t last_encoder_change_time = 0;

int32_t encoder_get_count(void) {
    int32_t count;
    pcnt_get_counter_value(encoder_pcnt_unit, &count);
    
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    
    // 检测变化
    if (count != last_encoder_count) {
        last_encoder_count = count;
        last_encoder_change_time = now;
    }
    
    // 阈值过滤
    if (count >= ENCODER_STEP_THRESHOLD) {
        pcnt_counter_clear(encoder_pcnt_unit);
        last_encoder_count = 0;
        return 1;
    } else if (count <= -ENCODER_STEP_THRESHOLD) {
        pcnt_counter_clear(encoder_pcnt_unit);
        last_encoder_count = 0;
        return -1;
    }
    
    // 500ms自动清零
    if (count != 0 && (now - last_encoder_change_time) >= ENCODER_AUTO_CLEAR_MS) {
        pcnt_counter_clear(encoder_pcnt_unit);
        last_encoder_count = 0;
    }
    
    return 0;
}
```

**验证**: 需旋转3个step才触发，体验改善

---

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
