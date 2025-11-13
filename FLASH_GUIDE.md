# ESP32-S3 GPS MINiMIX 固件烧录指南

## 固件文件说明

### 完整固件（推荐）
- **文件**: `build/esp32s3_gps_minimix_full.bin`
- **大小**: 501 KB
- **包含**: Bootloader + 分区表 + 应用程序
- **烧录地址**: 0x0

### 分离固件（开发用）
1. **Bootloader**: `build/bootloader/bootloader.bin` (地址: 0x0)
2. **分区表**: `build/partition_table/partition-table.bin` (地址: 0x8000)
3. **应用程序**: `build/esp32s3_gps_minimix.bin` (地址: 0x10000)

## 烧录方法

### 方法 1: 使用 esptool（命令行）

#### 烧录完整固件（最简单）
```bash
esptool --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 \
  write_flash 0x0 build/esp32s3_gps_minimix_full.bin
```

#### 烧录分离固件
```bash
esptool --chip esp32s3 --port /dev/ttyUSB0 --baud 460800 \
  --before default-reset --after hard-reset write_flash \
  --flash-mode dio --flash-freq 80m --flash-size 4MB \
  0x0 build/bootloader/bootloader.bin \
  0x8000 build/partition_table/partition-table.bin \
  0x10000 build/esp32s3_gps_minimix.bin
```

### 方法 2: 使用 idf.py（推荐开发时使用）
```bash
idf.py -p /dev/ttyUSB0 flash monitor
```

### 方法 3: 使用 Flash Download Tool（Windows 图形界面）

1. 下载 [ESP Flash Download Tool](https://www.espressif.com/en/support/download/other-tools)
2. 选择芯片类型: ESP32-S3
3. 配置文件和地址:
   - **完整固件模式**:
     - `esp32s3_gps_minimix_full.bin` @ 0x0
   - **分离固件模式**:
     - `bootloader.bin` @ 0x0
     - `partition-table.bin` @ 0x8000
     - `esp32s3_gps_minimix.bin` @ 0x10000
4. 配置 Flash 参数:
   - SPI Speed: 80MHz
   - SPI Mode: DIO
   - Flash Size: 4MB
5. 选择 COM 口
6. 点击 START 开始烧录

## Windows 串口号查找

在 Windows 设备管理器中查看:
- 展开 "端口 (COM 和 LPT)"
- 找到 USB-SERIAL CH340 或类似设备
- 记下 COM 端口号（例如: COM3）

将上述命令中的 `/dev/ttyUSB0` 替换为 `COM3`（或你的实际端口号）

## 硬件配置

- **芯片**: ESP32-S3
- **Flash**: 4MB
- **PSRAM**: 2MB (八线模式，80MHz)
- **Flash 模式**: DIO
- **Flash 频率**: 80MHz

## 分区布局

| 名称       | 类型      | 子类型  | 偏移地址 | 大小  |
|-----------|----------|---------|---------|------|
| nvs       | data     | nvs     | 0x9000  | 24KB |
| phy_init  | data     | phy     | 0xf000  | 4KB  |
| factory   | app      | factory | 0x10000 | 3MB  |
| storage   | data     | fat     | auto    | 1MB  |

## 故障排查

### 烧录失败
1. 确保 USB 线缆支持数据传输（非仅充电线）
2. 检查 CH340 驱动是否已安装
3. 尝试降低波特率: `--baud 115200`
4. 按住 BOOT 按钮，点击 RST 按钮进入下载模式

### 运行异常
1. 检查 PSRAM 连接（GPIO30=CLK, GPIO26=CS）
2. 使用串口监视器查看日志: `idf.py monitor`
3. 擦除 Flash 后重新烧录: `esptool --chip esp32s3 erase_flash`

## 监视串口输出

```bash
# 使用 idf.py
idf.py -p /dev/ttyUSB0 monitor

# 或使用其他工具
screen /dev/ttyUSB0 115200
# 或
minicom -D /dev/ttyUSB0 -b 115200
```

## 更新说明

- **完整固件**: 每次编译后自动生成在 `build/esp32s3_gps_minimix_full.bin`
- **版本**: 基于 Git commit ID (当前: 76d46f4-dirty)
- **ESP-IDF 版本**: v6.1-dev-450-g286b8cb76d
