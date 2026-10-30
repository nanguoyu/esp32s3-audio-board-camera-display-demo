# Development notes

[Chinese version](../zh-CN/development.md)

## Source layout

| File | Responsibility |
| --- | --- |
| main/main.c | Frame loop, touch gestures, buttons and serial commands |
| main/board.c | I2C, expander, camera, LCD and LED ring |
| main/effects_ui.c | Effects, text, buttons and sound-level meter |
| main/audio.c | ES8311 output, ES7210 input and sound cues |
| tools/build.py | ESP-IDF environment setup and optional component-cache reuse |
| tools/device_check.py | Serial capture and frame export |

## Frame pipeline

The camera produces 240 × 240 JPEG frames into two PSRAM buffers. The main loop takes the latest frame and decodes it to a 115,200-byte RGB565 buffer in internal SRAM. The renderer applies the selected effect and draws the controls into a 134,400-byte screen buffer in PSRAM. The LCD driver copies 20 rows at a time into an internal DMA buffer, swaps the color bytes and sends them over 20 MHz SPI.

The ST7789V2 startup commands are sent through GPIO before the SPI peripheral takes over. The generic initialization sequence alone produced a blank screen on the display module used here. Pixel transfers use the normal ESP-IDF SPI driver.

Audio runs in a separate task on core 1. It samples the ES7210 at 16 kHz, removes the block's DC offset and computes a sound level for the meter and LEDs. A small queue carries sound-cue requests from the UI to the ES8311 playback task.

With ESP-IDF 5.5.4 on an ESP32-S3R8, MIRROR mode measured 7.7–7.8 fps after the first export. The following values are per-frame measurements from that setup:

| Work | Time |
| --- | --- |
| Acquire the latest camera buffer | About 0.03 ms |
| Decode JPEG to RGB565 | 44–45 ms |
| Render image and controls | About 7.6 ms |
| Prepare stripes and transfer the LCD frame | 69–70 ms |

The firmware reported about 164 KiB of free internal heap with camera and audio running. Scene complexity and the selected effect affect the frame rate. Exporting an image over serial pauses the frame loop while the transfer completes.

## Serial commands

| Command | Action |
| --- | --- |
| n / b | Next / previous effect |
| f | Freeze or resume |
| m | Toggle sound cues |
| s | Export the current camera JPEG |
| p | Export the rendered screen as RGB565 |
| i | Scan the shared I2C bus |

The export protocol uses hexadecimal lines between `JPEG_BEGIN <size>` and `JPEG_END`, or between `UI_BEGIN <size>` and `UI_END`. The host tool checks the byte count and JPEG markers. It saves camera frames as `camera.jpg` and converts UI exports into a 240 × 280 `screen.ppm`.

Run the tools with the ESP-IDF Python environment, or install `requirements-tools.txt` in another Python environment:

```sh
python tools/device_check.py --port "$PORT" --snapshot --seconds 45
python tools/device_check.py --port "$PORT" --commands nnnnfpfmm --seconds 85
```

The second command cycles the effects, freezes, exports the screen, resumes and toggles sound twice. Each action follows a status report. The optional `--mac` argument checks the USB device serial number before opening the port. `--output` selects a different capture directory.

## Offline builds

For a machine with the exact component versions already installed, the build helper can reuse that cache:

```sh
python tools/build.py --cache /path/to/managed_components build
```

The directory must contain `espressif__esp32-camera`, `espressif__esp_jpeg`, `espressif__led_strip` and `espressif__esp_codec_dev`. Their versions must match `main/idf_component.yml`. The helper uses `IDF_PATH` when it is set, or accepts `--idf /path/to/esp-idf`.

## Backup and restore

These commands assume the Audio Board's 16 MB Flash and an activated ESP-IDF environment. Close serial monitors first.

```sh
mkdir -p backups
python -m esptool --chip esp32s3 --port "$PORT" read_flash 0 0x1000000 backups/original.bin
python -m esptool --chip esp32s3 --port "$PORT" verify_flash 0 backups/original.bin
```

To restore that image:

```sh
python -m esptool --chip esp32s3 --port "$PORT" write_flash 0 backups/original.bin
```

The `backups/` and `captures/` directories are ignored by Git. A full Flash backup may contain settings from the firmware previously installed on the board.
