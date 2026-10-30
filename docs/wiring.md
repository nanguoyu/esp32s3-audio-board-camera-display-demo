# Wiring / 接线

Disconnect USB and battery power before changing a ribbon or jumper wire.
更换排线或接线前，先断开 USB 和电池供电。

## Display / 屏幕

Keep the panel ribbon connected to the display's original adapter PCB. Use that PCB's GH1.25 12-pin cable to connect to the Audio Board header.

屏幕的橙色排线接回原配转接板，再用转接板的 GH1.25 12 芯线连接 Audio Board 排针。

| Display signal / 屏幕信号 | Audio Board label / 主板丝印 | Connection |
| --- | --- | --- |
| VCC | 3V3 | 3.3 V supply |
| GND | GND | Ground |
| LCD_DIN | IO9 | SPI MOSI |
| LCD_CLK | IO4 | SPI clock |
| LCD_CS | IO3 | Chip select |
| LCD_DC | IO7 | Data/command |
| LCD_RST | EX1 | TCA9555 bit 0 |
| LCD_BL | IO5 | Backlight |
| TP_SDA | SDA | GPIO11 |
| TP_SCL | SCL | GPIO10 |
| TP_RST | EX2 | TCA9555 bit 1 |
| TP_IRQ | EX3 | TCA9555 bit 2 |

The header silkscreen uses **EX1, EX2, EX3**. The schematic and firmware number those expander pins **0, 1, 2**. On the header, EX1 is paired with IO5, EX2 with IO6, and EX3 with IO7. These are expander pins, not ESP32 GPIO1, GPIO2 and GPIO3.

排针上的 **EX1、EX2、EX3** 对应固件中的扩展口 **0、1、2**。它们分别与 IO5、IO6、IO7 成对排列，不是 ESP32 的 GPIO1、GPIO2、GPIO3。

The bare screen ribbon does not match the main board's 18-pin display socket. For example, pin 1 is GND on the panel but 3.3 V on the board; pin 3 is the LCD supply on the panel but GND on the board. The connector width alone does not establish compatibility.

裸屏排线不能直接插到主板的 18 针显示座。例如，屏幕第 1 针是 GND，主板第 1 针却是 3.3 V；屏幕第 3 针是 LCD 电源，主板第 3 针却是 GND。

## Camera / 摄像头

Use the 24-pin CAMERA socket near the TF card slot. Align the ribbon's exposed metal contacts with the contacts inside the socket, insert it evenly, and close the latch. The printed side of the ribbon is not a reliable guide to contact orientation.

摄像头接靠近 TF 卡槽的 24 针 CAMERA 插座。让排线末端的金属触点朝向插座内的触片，平直插到底，再锁紧。不要仅凭排线印字判断正反。

| Camera signal | ESP32-S3 connection |
| --- | --- |
| D0–D7 | GPIO2, 17, 18, 39, 45, 46, 47, 48 |
| VSYNC / HREF | GPIO21 / GPIO1 |
| XCLK / PCLK | GPIO43 / GPIO44 |
| SCCB SDA / SCL | GPIO11 / GPIO10 |
| PWDN | TCA9555 bit 5 |

The official demo and pin table name expander bit 6 as the camera-clock selector; the schematic's mux note names bit 7. This firmware holds both high, using GPIO43/44 for the camera while leaving GPIO19/20 available for native USB. This configuration was used for the hardware measurements in this project.

官方示例和引脚表将摄像头时钟切换信号标为扩展位 6，原理图旁注写的是 7。本固件将两位都保持高电平，让摄像头使用 GPIO43/44，并保留 GPIO19/20 的原生 USB 功能。项目中的实测使用了这一配置。

## Sources

- [Audio Board schematic](https://files.waveshare.com/wiki/ESP32-S3-AUDIO-Board/ESP32-S3-AUDIO-Board_1.1.pdf)
- [Display module schematic](https://files.waveshare.com/wiki/1.69inch-Touch-LCD-Module/1.69inch_Touch_LCD_Module.pdf)
- [Audio Board pinout](https://docs.waveshare.com/ESP32-S3-AUDIO-Board)
