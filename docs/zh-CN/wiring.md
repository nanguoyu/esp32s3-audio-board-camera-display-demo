# 接线

[English](../en/wiring.md)

更换排线或接线前，先断开 USB 和电池供电。

## 屏幕

屏幕的橙色排线接回原配转接板，再用转接板的 GH1.25 12 芯线连接 Audio Board 排针。

| 屏幕信号 | 主板丝印 | 对应连接 |
| --- | --- | --- |
| VCC | 3V3 | 3.3 V 电源 |
| GND | GND | 地 |
| LCD_DIN | IO9 | SPI 数据输出 |
| LCD_CLK | IO4 | SPI 时钟 |
| LCD_CS | IO3 | 片选 |
| LCD_DC | IO7 | 数据与命令选择 |
| LCD_RST | EX1 | TCA9555 第 0 位 |
| LCD_BL | IO5 | 背光 |
| TP_SDA | SDA | GPIO11 |
| TP_SCL | SCL | GPIO10 |
| TP_RST | EX2 | TCA9555 第 1 位 |
| TP_IRQ | EX3 | TCA9555 第 2 位 |

排针上的 **EX1、EX2、EX3** 对应原理图和固件中的扩展口 **0、1、2**。它们分别与 IO5、IO6、IO7 成对排列，不是 ESP32 的 GPIO1、GPIO2、GPIO3。

裸屏排线不能直接插到主板的 18 针显示座。例如，屏幕第 1 针是 GND，主板第 1 针却是 3.3 V；屏幕第 3 针是 LCD 电源，主板第 3 针却是 GND。插座宽度相同不代表引脚兼容。

## 摄像头

摄像头接靠近 TF 卡槽的 24 针 CAMERA 插座。让排线末端的金属触点朝向插座内的触片，平直插到底，再锁紧。不要仅凭排线印字判断正反。

| 摄像头信号 | ESP32-S3 连接 |
| --- | --- |
| D0–D7 | GPIO2、17、18、39、45、46、47、48 |
| VSYNC / HREF | GPIO21 / GPIO1 |
| XCLK / PCLK | GPIO43 / GPIO44 |
| SCCB SDA / SCL | GPIO11 / GPIO10 |
| PWDN | TCA9555 第 5 位 |

官方示例和引脚表将摄像头时钟切换信号标为扩展位 6，原理图旁注写的是 7。本固件将两位都保持高电平，让摄像头使用 GPIO43/44，并保留 GPIO19/20 的原生 USB 功能。项目中的实测使用了这一配置。

## 参考资料

- [Audio Board 原理图](https://files.waveshare.com/wiki/ESP32-S3-AUDIO-Board/ESP32-S3-AUDIO-Board_1.1.pdf)
- [屏幕模块原理图](https://files.waveshare.com/wiki/1.69inch-Touch-LCD-Module/1.69inch_Touch_LCD_Module.pdf)
- [Audio Board 引脚说明](https://docs.waveshare.com/ESP32-S3-AUDIO-Board)
