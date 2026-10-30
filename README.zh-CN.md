# ESP32-S3 Audio Board 摄像头与触摸屏示例

[English](README.md)

这是一个运行在 **Waveshare ESP32-S3-AUDIO-Board** 上的触摸相机示例。OV2640 的画面可以切换为镜像、波普色彩、万花筒和四色像素风。点击屏幕可以定格，左右滑动可以切换效果，长按可以关闭或恢复音效。灯环颜色跟随当前效果，亮度跟随麦克风采集到的声音。

应用在开发板上运行，不需要 Wi-Fi 或 SD 卡。定格画面保存在 RAM 中，恢复实时画面或重启后就会被替换。

## 硬件

| 部件 | 配置 |
| --- | --- |
| 主板 | Waveshare ESP32-S3-AUDIO-Board，16 MB Flash、8 MB PSRAM |
| 摄像头 | OV2640，24 针 DVP 排线 |
| 屏幕 | Waveshare 1.69inch Touch LCD Module，240 × 280，ST7789V2、CST816T |
| 音频和灯环 | 板载 ES8311、ES7210、麦克风和七颗 LED，以及接好的扬声器 |

**屏幕必须通过原配转接板和 12 芯线连接。** 裸屏排线与 Audio Board 显示插座的引脚定义不同，不能直接互插。上电前请按[接线说明](docs/zh-CN/wiring.md)连接。

## 编译与烧录

先安装并激活 [ESP-IDF 5.5.x](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5/esp32s3/get-started/index.html)。硬件测试使用的是 ESP-IDF 5.5.4。构建时会自动下载 [main/idf_component.yml](main/idf_component.yml) 中固定版本的组件。

```sh
git clone https://github.com/nanguoyu/esp32s3-audio-board-camera-display-demo.git
cd esp32s3-audio-board-camera-display-demo
idf.py set-target esp32s3
idf.py build
python -m serial.tools.list_ports -v
```

从端口列表中找到开发板的 USB Serial/JTAG 设备，将下面的 `PORT` 换成实际端口。macOS 通常是 `/dev/cu.usbmodemXXXX`，Linux 通常是 `/dev/ttyACM0`。

```sh
PORT=/dev/cu.usbmodemXXXX
idf.py -p "$PORT" flash monitor
```

按 **Ctrl+]** 退出串口监视器。如果要保留板上原有固件，烧录前先按[备份与恢复说明](docs/zh-CN/development.md#备份与恢复)备份。

## 操作

| 操作 | 功能 |
| --- | --- |
| 点击 **EFFECT** | 依次切换 MIRROR、POP ART、KALEIDO、PIXEL |
| 左右滑动 | 切换下一个或上一个效果 |
| 点击画面或 **FREEZE** | 定格或恢复画面 |
| 点击 **RESUME** | 恢复实时画面 |
| 长按约一秒 | 静音或恢复音效，右上角显示 MUTE 或 SOUND |
| 按 K1 / K2 / K3 | 切换效果 / 定格恢复 / 静音切换 |

定格后仍然可以切换效果。右侧音量条显示麦克风的声音强度，切换效果和定格时会播放短音效。

## 刷新率

上述硬件在 MIRROR 模式下，240 × 240 实时画面的刷新率约为 **7.7–7.8 帧/秒**。完整屏幕是 240 × 280，下方留给操作按钮。每帧主要耗时在 JPEG 解码和 LCD 传输，具体内存布局和耗时见[实现说明](docs/zh-CN/development.md#图像处理流程)。

## 抓图与排错

串口工具可以把摄像头 JPEG 保存到电脑：

```sh
python -m pip install -r requirements-tools.txt
python tools/device_check.py --port "$PORT" --snapshot --seconds 45
```

文件保存在 Git 忽略的 `captures/` 目录。运行工具前，请关闭占用同一端口的串口监视器。更多控制命令见[开发说明](docs/zh-CN/development.md)；黑屏、摄像头未识别或触摸异常的处理方法见[排错说明](docs/zh-CN/troubleshooting.md)。

## 参考资料

- [Audio Board 官方文档](https://docs.waveshare.com/ESP32-S3-AUDIO-Board)
- [1.69 英寸触摸屏官方文档](https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module)
- [依赖与来源](docs/zh-CN/dependencies.md)
