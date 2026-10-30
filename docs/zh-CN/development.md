# 开发说明

[英文版](../en/development.md)

## 源码结构

| 文件 | 职责 |
| --- | --- |
| main/main.c | 帧循环、触摸手势、按键与串口命令 |
| main/board.c | I2C、扩展芯片、摄像头、LCD 与灯环 |
| main/effects_ui.c | 图像效果、文字、按钮与音量条 |
| main/audio.c | ES8311 播放、ES7210 采集与短音效 |
| tools/build.py | ESP-IDF 环境设置与组件缓存复用 |
| tools/device_check.py | 串口日志与图像导出 |

## 图像处理流程

摄像头将 240 × 240 的 JPEG 帧写入两个 PSRAM 缓冲区。主循环取得最新一帧，将其解码到内部 SRAM 中的 RGB565 缓冲区，占用 115,200 字节。渲染程序应用当前效果，并把图像与操作控件绘制到 PSRAM 中的屏幕缓冲区，占用 134,400 字节。LCD 驱动每次将 20 行复制到内部 DMA 缓冲区，交换颜色字节顺序后，通过 20 MHz SPI 发送。

ST7789V2 的启动命令先通过 GPIO 发送，随后由 SPI 外设接管像素传输。在本项目使用的屏幕上，仅使用通用初始化序列时曾出现背光亮但无图像的现象，因此保留了这一启动方式。

音频运行在核心 1 的独立任务中。任务以 16 kHz 从 ES7210 采样，去掉当前数据块的直流偏移，再计算音量条和灯环所需的声音强度。界面通过队列向 ES8311 播放任务发送短音效请求。

在 ESP32-S3R8 和 ESP-IDF 5.5.4 上，MIRROR 模式完成首次导出后测得约 7.7–7.8 帧/秒。该配置下的每帧耗时如下：

| 工作 | 耗时 |
| --- | --- |
| 取得最新摄像头缓冲区 | 约 0.03 毫秒 |
| JPEG 解码为 RGB565 | 44–45 毫秒 |
| 渲染图像与控件 | 约 7.6 毫秒 |
| 准备分块数据并传输 LCD 画面 | 69–70 毫秒 |

摄像头与音频同时运行时，内部堆剩余约 164 KiB。画面复杂度和所选效果会影响帧率。串口导出图像时，帧循环会等待传输完成。

## 串口命令

| 命令 | 功能 |
| --- | --- |
| n / b | 下一个或上一个效果 |
| f | 定格或恢复 |
| m | 切换音效开关 |
| s | 导出当前摄像头 JPEG |
| p | 导出渲染后的 RGB565 画面 |
| i | 扫描共享 I2C 总线 |

导出协议在 `JPEG_BEGIN <size>` 与 `JPEG_END` 之间，或 `UI_BEGIN <size>` 与 `UI_END` 之间发送十六进制文本行。电脑端工具检查字节数及 JPEG 起止标记，将摄像头图像保存为 `camera.jpg`，并把界面数据转换为 240 × 280 的 `screen.ppm`。

可以使用 ESP-IDF 自带的 Python 环境运行工具；使用其他 Python 环境时，先安装 `requirements-tools.txt` 中的依赖：

```sh
python tools/device_check.py --port "$PORT" --snapshot --seconds 45
python tools/device_check.py --port "$PORT" --commands nnnnfpfmm --seconds 85
```

第二条命令依次切换效果、定格、导出界面、恢复，再切换两次音效开关。每个操作在收到一条状态报告后发送。可选的 `--mac` 参数会在打开端口前核对 USB 设备序列号，`--output` 可指定保存目录。

## 离线构建

如果电脑上已有完全匹配版本的组件，可以通过构建工具复用缓存：

```sh
python tools/build.py --cache /path/to/managed_components build
```

缓存目录需要包含 `espressif__esp32-camera`、`espressif__esp_jpeg`、`espressif__led_strip` 和 `espressif__esp_codec_dev`，版本应与 `main/idf_component.yml` 一致。构建工具优先使用环境变量 `IDF_PATH`，也可以通过 `--idf /path/to/esp-idf` 指定路径。

## 备份与恢复

以下命令适用于 Audio Board 的 16 MB Flash，需要先激活 ESP-IDF 环境并关闭占用端口的串口监视器。`PORT` 应设为开发板的实际端口。

```sh
mkdir -p backups
python -m esptool --chip esp32s3 --port "$PORT" read_flash 0 0x1000000 backups/original.bin
python -m esptool --chip esp32s3 --port "$PORT" verify_flash 0 backups/original.bin
```

恢复备份：

```sh
python -m esptool --chip esp32s3 --port "$PORT" write_flash 0 backups/original.bin
```

`backups/` 和 `captures/` 目录默认由 Git 忽略。完整 Flash 备份可能含有原固件保存的配置。
