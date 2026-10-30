# 排错说明

[英文版](../en/troubleshooting.md)

## 背光亮，但没有图像

背光有独立控制引脚，因此背光亮不代表屏幕已收到 SPI 数据。按[接线表](wiring.md)核对 LCD_DIN → IO9、LCD_CLK → IO4、LCD_CS → IO3、LCD_DC → IO7、LCD_RST → EX1。

屏幕应保留原配转接板，通过转接板的 12 芯线连接主板排针。裸屏排线与主板显示插座不兼容。

固件先通过 GPIO 发送屏幕模块的启动寄存器序列，再用 SPI 传输像素。修改驱动时，需要保留这段启动序列，以及 240 × 280 屏幕对应的 20 像素垂直偏移。

## 摄像头离线或报错 ESP_ERR_NOT_SUPPORTED

查看启动日志是否包含 `Detected OV2640 camera` 和 `CAMERA_INIT ESP_OK`。OV2640 的控制地址是 0x30。驱动找不到响应的传感器时也可能返回 `ESP_ERR_NOT_SUPPORTED`，不能仅凭这个错误判断摄像头型号不受支持。

断开 USB 和电池供电，打开摄像头插座锁扣，确认插座金属触片的位置，再将排线触点朝向触片，平直插到底并锁紧。同时核对摄像头是否为采用预期 24 针连接的 OV2640 模块。

## 触摸没有反应

触摸使用共享 I2C 总线。核对 TP_SDA → SDA、TP_SCL → SCL、TP_RST → EX2。正常启动时会打印 `TOUCH_ID` 和 `TOUCH_INIT ESP_OK`。

固件通过轮询读取坐标。如果点击有效但手势无效，请从图像区域开始水平滑动。停留约一秒的长按会切换音效开关。

## 没有声音

先看屏幕右上角是否显示 **MUTE**。长按约一秒或按 K3，切回 **SOUND**。确认扬声器已连接，再点击 EFFECT 或 FREEZE。

正常启动时会打印 `AUDIO_READY speaker=1 microphones=1`。麦克风音量条和灯环亮度独立于音效开关，静音时仍然会响应输入声音。

## 画面停止更新

如果屏幕显示 **FROZEN**，点击 RESUME。实时模式下，串口状态行会报告 `frames`、`fps`、`decode_errors` 和 `capture_errors`。定格时，`frames` 计数停止增加。

串口导出图像会暂时占用帧循环。测量正常帧率前，等待导出完成或停止抓图工具。本项目在所述硬件上测得约每秒八帧。

## 串口工具无法打开端口

关闭 `idf.py monitor` 和其他占用同一端口的程序。重新运行 `python -m serial.tools.list_ports -v`，因为重插开发板后端口名可能改变。应选择 Espressif USB Serial/JTAG 设备，注意区分显示器或扩展坞的串口条目。

打开串口连接可能使开发板复位。等待启动完成后，再查看状态消息。
