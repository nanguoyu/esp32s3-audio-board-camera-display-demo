# ESP32-S3 Audio Board Camera and Display Demo

[中文](README.zh-CN.md)

A camera and touchscreen demo for the **Waveshare ESP32-S3-AUDIO-Board**. The firmware displays an OV2640 camera feed with four effects: mirror, pop art, kaleidoscope and a four-color pixel view. Tap the screen to freeze a frame, swipe to change the effect, or hold to mute the sound cues. The LED ring changes color with the selected effect and brightness with sound from the microphones.

The application runs on the board. It needs neither Wi-Fi nor an SD card. A frozen frame stays in RAM until live view resumes or the board restarts.

## Hardware

| Part | Configuration |
| --- | --- |
| Main board | Waveshare ESP32-S3-AUDIO-Board, 16 MB Flash and 8 MB PSRAM |
| Camera | OV2640 with a 24-pin DVP ribbon |
| Display | Waveshare 1.69inch Touch LCD Module, 240 × 280, ST7789V2 and CST816T |
| Audio and LEDs | Onboard ES8311, ES7210, microphones and seven LEDs; a connected speaker |

**Connect the display through its original adapter PCB and 12-wire cable.** The bare panel ribbon and the Audio Board's display socket have different pin assignments. They must not be plugged directly into one another. Follow the [wiring guide](docs/en/wiring.md) before powering the board.

## Build and flash

Use an activated [ESP-IDF 5.5.x environment](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/get-started/index.html). The hardware measurements below used ESP-IDF 5.5.4. The component manager downloads the versions listed in [main/idf_component.yml](main/idf_component.yml).

```sh
git clone https://github.com/nanguoyu/esp32s3-audio-board-camera-display-demo.git
cd esp32s3-audio-board-camera-display-demo
idf.py set-target esp32s3
idf.py build
```

Find the native USB Serial/JTAG port:

```sh
python -m serial.tools.list_ports -v
```

Set `PORT` to the device shown by that command. For example, a macOS port looks like `/dev/cu.usbmodemXXXX`; a Linux port often looks like `/dev/ttyACM0`.

```sh
PORT=/dev/cu.usbmodemXXXX
idf.py -p "$PORT" flash monitor
```

Exit the monitor with **Ctrl+]**. To preserve an existing firmware installation, see [backup and restore](docs/en/development.md#backup-and-restore) before flashing.

## Controls

| Action | Result |
| --- | --- |
| Tap **EFFECT** | Cycle through MIRROR, POP ART, KALEIDO and PIXEL |
| Swipe left / right | Select the next / previous effect |
| Tap the image or **FREEZE** | Freeze or resume the image |
| Tap **RESUME** | Return to live view |
| Hold for about one second | Toggle sound cues; the header shows SOUND or MUTE |
| Press K1 / K2 / K3 | Change effect / freeze or resume / toggle sound |

Effects can also be changed while a frame is frozen. The meter on the right shows microphone level. Switching effects and freezing a frame play short tones.

## Performance

The 240 × 240 camera view ran at about **7.7–7.8 fps** in MIRROR mode on the hardware above. The full 240 × 280 display includes the controls. JPEG decoding and the LCD transfer account for most of each frame's time. See [implementation and timing](docs/en/development.md#frame-pipeline) for the buffer sizes and measurements.

## Serial tools

The firmware accepts serial commands for switching effects and exporting frames. The included tool saves a camera JPEG or a screen capture to the computer:

```sh
python -m pip install -r requirements-tools.txt
python tools/device_check.py --port "$PORT" --snapshot --seconds 45
```

Output goes to `captures/`, which is ignored by Git. Close other serial monitors before running the tool. More commands and an offline build option are described in [development notes](docs/en/development.md).

For a blank screen, an undetected camera or an unresponsive control, start with [troubleshooting](docs/en/troubleshooting.md).

## References

- [Waveshare Audio Board documentation](https://docs.waveshare.com/ESP32-S3-AUDIO-Board)
- [Waveshare 1.69-inch touch LCD documentation](https://www.waveshare.com/wiki/1.69inch_Touch_LCD_Module)
- [Dependencies and credits](docs/en/dependencies.md)
