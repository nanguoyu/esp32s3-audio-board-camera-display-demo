# 依赖与来源

[英文版](../en/dependencies.md)

固件使用 ESP-IDF 及以下 Espressif 组件。版本固定在 `main/idf_component.yml` 中，常规构建时由组件管理器下载。

| 组件 | 版本 | 上游仓库 |
| --- | --- | --- |
| esp32-camera | 2.1.6 | [espressif/esp32-camera](https://github.com/espressif/esp32-camera) |
| esp_jpeg | 1.3.1 | [espressif/idf-extra-components](https://github.com/espressif/idf-extra-components/tree/master/esp_jpeg) |
| led_strip | 3.0.3 | [espressif/idf-extra-components](https://github.com/espressif/idf-extra-components/tree/master/led_strip) |
| esp_codec_dev | 1.5.10 | [espressif/esp-adf](https://github.com/espressif/esp-adf/tree/master/components/esp_codec_dev) |

这些组件版本包含 Apache-2.0 许可证文件，内部第三方代码的授权条款见各自源码和声明。电脑端工具使用 [pySerial](https://github.com/pyserial/pyserial)，通过 `requirements-tools.txt` 单独安装。

LCD 寄存器参数和主板引脚定义参考 Waveshare 的 [1.69 英寸触摸屏资料](https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module)与 [Audio Board 资料](https://docs.waveshare.com/ESP32-S3-AUDIO-Board/Resources-And-Documents)。
