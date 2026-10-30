# Troubleshooting

## Backlight is on, but the screen is blank

The backlight has a separate control pin, so light from the panel does not establish that SPI data is reaching it. Check LCD_DIN → IO9, LCD_CLK → IO4, LCD_CS → IO3, LCD_DC → IO7 and LCD_RST → EX1 against the [wiring table](wiring.md).

Keep the screen connected to its original adapter PCB. Use the adapter's 12-wire cable to the Audio Board header. The bare panel ribbon is not compatible with the main board's display socket.

The firmware sends the module's startup register sequence through GPIO, then transfers pixels through SPI. If changing the driver, retain that startup sequence and the 20-pixel vertical offset for the 240 × 280 panel.

## CAMERA OFFLINE or ESP_ERR_NOT_SUPPORTED

Check the boot log for `Detected OV2640 camera` and `CAMERA_INIT ESP_OK`. The OV2640 control address is 0x30. The driver can report `ESP_ERR_NOT_SUPPORTED` when it does not find a responding sensor; that error alone does not identify an unsupported camera model.

Disconnect USB and battery power. Open the camera socket latch, check which side contains the metal contacts, and align the ribbon contacts with them. Insert the ribbon evenly and close the latch. Also check that the module is an OV2640 with the expected 24-pin connection.

## Touch does not respond

Touch uses the shared I2C bus. Check TP_SDA → SDA, TP_SCL → SCL and TP_RST → EX2. A normal startup prints `TOUCH_ID` and `TOUCH_INIT ESP_OK`.

The firmware polls the controller for coordinates. If taps work but gestures do not, start the swipe on the image and move horizontally. A hold of about one second toggles sound rather than changing the effect.

## No sound

Check whether the header says **MUTE**. Hold the screen for about one second or press K3 to return to **SOUND**. Confirm that a speaker is connected, then tap EFFECT or FREEZE.

The boot log should contain `AUDIO_READY speaker=1 microphones=1`. The right-hand meter and LED brightness respond to microphone input independently of whether sound cues are muted.

## The image stops moving

If the screen says **FROZEN**, tap RESUME. In live mode, the serial status line reports `frames`, `fps`, `decode_errors` and `capture_errors`. The `frames` count stops increasing while frozen.

A serial image export temporarily occupies the frame loop. Wait for it to finish or stop the capture tool before measuring normal frame rate. This project measured roughly eight frames per second on the documented hardware.

## The serial tool cannot open the port

Close `idf.py monitor` and any other program using that port. Run `python -m serial.tools.list_ports -v` again because port names can change after reconnecting the board. Use the Espressif USB Serial/JTAG device, not another USB modem entry belonging to a monitor or dock.

Opening a serial connection can reset the board. Allow it to finish startup before interpreting its status messages.
