# Dependencies and credits

The firmware uses ESP-IDF and the following Espressif components. Versions are fixed in `main/idf_component.yml`; the component manager downloads their source during a normal build.

| Component | Version | Upstream |
| --- | --- | --- |
| esp32-camera | 2.1.6 | [espressif/esp32-camera](https://github.com/espressif/esp32-camera) |
| esp_jpeg | 1.3.1 | [espressif/idf-extra-components](https://github.com/espressif/idf-extra-components/tree/master/esp_jpeg) |
| led_strip | 3.0.3 | [espressif/idf-extra-components](https://github.com/espressif/idf-extra-components/tree/master/led_strip) |
| esp_codec_dev | 1.5.10 | [espressif/esp-adf](https://github.com/espressif/esp-adf/tree/master/components/esp_codec_dev) |

These component releases include Apache-2.0 license files. Their own source files and notices describe the terms of included third-party code. The host tools use [pySerial](https://github.com/pyserial/pyserial), installed separately from `requirements-tools.txt`.

The LCD register values and board pin assignments come from Waveshare's [1.69-inch touch LCD resources](https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module) and [Audio Board resources](https://docs.waveshare.com/ESP32-S3-AUDIO-Board/Resources-And-Documents).
