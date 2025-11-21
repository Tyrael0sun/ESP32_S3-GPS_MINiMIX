# ESP32-S3 多功能 GPS 性能分析设备开发文档

## 1. 项目概述
本项目基于 **ESP32-S3FH4R2** 微控制器，开发一套集自行车码表、GPS 轨迹记录和汽车性能测试盒（P-Box）于一体的多功能固件。系统利用 FreeRTOS 进行任务管理，LVGL 进行图形界面显示，并集成多传感器数据融合算法。

- **开发环境**：ESP-IDF v6.1
- **构建系统**：CMake
- **GUI 框架**：LVGL v8.3

---

## 2. 硬件配置与引脚定义

### 2.1 核心控制器
- **芯片**：ESP32-S3FH4R2 (Flash 4MB / PSRAM 2MB)

### 2.2 传感器系统 (I2C Bus 0)
- **I2C 配置**：
  - SCL: GPIO 39
  - SDA: GPIO 40
  - 频率: 1 MHz
  - **注意**：内部上拉电阻禁用（依赖外部上拉）。
- **IMU (6轴惯性测量单元)**
  - 型号：LSM6DSR
  - 地址：0x6A
  - **轴向配置**：X轴反向，Z轴反向，Y轴不变。
- **磁力计**
  - 型号：LIS2MDL
  - 地址：0x1E
  - **轴向配置**：X/Y轴交换，然后Y轴反向，Z轴反向。
  - 温度读取：寄存器 0x6E (L), 0x6F (H)。
- **气压计**
  - 型号：BMP388
  - 地址：0x76
  - 补偿算法：需使用浮点校准参数进行温度和气压补偿，计算海拔。

### 2.3 GNSS 定位系统 (UART 1)
- **型号**：U-Blox MAX-F10S (或同类 UBX 协议模块)
- **引脚**：
  - ESP_TX (接模块 RX): GPIO 18
  - ESP_RX (接模块 TX): GPIO 17
  - LDO_EN: GPIO 14
- **波特率配置**：
  - 默认：9600 bps
  - 工作：115200 bps (启动时通过 UBX `CFG-VALSET` 命令切换)。

### 2.4 显示系统 (SPI 3)
- **屏幕**：ST7789 IPS LCD (240x320)
- **引脚**：
  - SCK: 5, MOSI: 8, CS: 7, DC: 6, RST: 4, BL: 9
- **配置**：
  - 颜色格式：RGB565
  - 旋转：180度 (通过软件 Mirror X & Y 实现)。
  - 驱动：`esp_lcd` (使用 `rgb_ele_order` 适配 IDF v5+)。

### 2.5 输入设备
- **旋转编码器**：
  - A相: GPIO 1 (内部上拉)
  - B相: GPIO 3 (内部上拉)
- **主按键**：
  - KEY: GPIO 2 (内部上拉，低电平有效)
- **逻辑**：支持短按、中按(>500ms)、长按(>2000ms)、双击检测。

### 2.6 电源管理
- **电池电压检测**：
  - 引脚: GPIO 12 (ADC2 Channel 1)
  - 分压比: 1:1 (V_bat = V_adc * 2)
  - 充电状态: GPIO 21

### 2.7 存储 (SDIO 4-bit) - *待完善*
- CMD: 35, CLK: 36, D0: 37, D1: 38, D2: 33, D3: 34

---

## 3. 软件架构

### 3.1 任务分配 (FreeRTOS)
| 任务名称 | 优先级 | 栈大小 | 职责 |
| --- | --- | --- | --- |
| `gnss_task` | 5 | 4096 | 串口数据流解析 (NMEA/UBX)，位置更新。 |
| `ui_task` | 5 | 8192 | LVGL 渲染循环，显示刷新 (互斥锁保护)。 |
| `logger_task` | 4 | 4096 | SD 卡数据写入 (GPX 格式)。 |
| `diagnostics_task` | 3 | 4096 | 传感器数据采集、融合计算、系统心跳日志。 |
| `input_task` | 5 | 2048 | GPIO 轮询 (10ms)，按键状态机，事件上报。 |

### 3.2 关键模块实现
- **Sensor Fusion**:
  - 重力分离：低通滤波器 (Alpha=0.2)。
  - 线性加速度：原始加速度 - 重力分量。
  - 航向角 (Heading)：`atan2(My, Mx)`。
  - 海拔：基于 BMP388 气压的 Hypsometric 公式。
- **Watchdog (WDT) 修复**:
  - 输入任务使用非零 `vTaskDelay` 防止 IDLE 任务饥饿。
- **I2C 驱动**:
  - 迁移至 `driver/i2c_master.h` 以兼容 ESP-IDF v6.0+。

---

## 4. 调试与日志规范

### 4.1 启动自检 (0-5s)
每秒输出一次完整传感器状态，检查通信是否正常。

### 4.2 运行时心跳 (每 5s)
格式严格统一，包含以下字段：
```text
I (4468) MAIN: IMU: ACC(x,y,z) GRAV(x,y,z) LIN(x,y,z)
I (4468) MAIN: GYRO: (x,y,z) dps
I (4468) MAIN: MAG: (x,y,z) Heading=deg
I (4468) MAIN: BARO: P=hPa Alt=m
I (4478) MAIN: TEMP: IMU=C, MAG=C, BARO=C
I (4478) MAIN: BAT: mV
```

### 4.3 事件响应
按键或编码器操作需立即触发日志：
`[EVENT] KEY: SHORT PRESS` / `[EVENT] ENC: CW` 等。

---

## 5. 待办事项 (TODO)
1.  **SD 卡驱动**：初始化 SDIO 总线并挂载 FATFS 文件系统。
2.  **GPX 记录**：实现将 GNSS 和传感器数据格式化为 GPX 文件写入 SD 卡。
3.  **UI 逻辑**：完善 LVGL 界面，实现不同模式（码表、记录、P-Box）的页面切换和数据显示。
4.  **P-Box 算法**：实现 0-100km/h 自动计时逻辑。
