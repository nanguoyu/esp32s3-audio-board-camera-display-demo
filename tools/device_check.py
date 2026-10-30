#!/usr/bin/env python3
"""Capture diagnostics and an optional camera JPEG locally, without publishing it."""
import argparse
from pathlib import Path
import re
import time
import serial
from serial.tools import list_ports

parser = argparse.ArgumentParser()
parser.add_argument('--port', required=True)
parser.add_argument('--mac', help='Expected USB serial number / MAC, checked before opening')
parser.add_argument('--seconds', type=float, default=45)
parser.add_argument('--snapshot', action='store_true')
parser.add_argument('--commands', default='', help='Send one of n/b/f/m/p after each status report')
parser.add_argument('--output', type=Path, default=Path(__file__).resolve().parents[1] / 'captures')
args = parser.parse_args()
if any(c not in 'nbfmp' for c in args.commands):
    parser.error('commands may only contain n, b, f, m and p')
if args.mac:
    devices = {p.device: p for p in list_ports.comports()}
    info = devices.get(args.port)
    if not info or (info.serial_number or '').lower() != args.mac.lower():
        raise SystemExit('Port does not match the expected device identity')
args.output.mkdir(parents=True, exist_ok=True)
port = serial.Serial(port=None, baudrate=115200, timeout=0.25)
port.dtr = False
port.rts = False
port.port = args.port
port.open()
deadline = time.monotonic() + args.seconds
snapshot_sent = False
snapshot_received = False
expected_size = 0
frame = bytearray()
capture = False
kind = 'JPEG'
commands = list(args.commands)
buffer = bytearray()
with (args.output / 'serial.log').open('wb') as log:
    try:
        while time.monotonic() < deadline:
            chunk = port.read(min(4096, port.in_waiting or 1))
            log.write(chunk)
            buffer.extend(chunk)
            while b'\n' in buffer:
                raw, _, buffer = buffer.partition(b'\n')
                line = raw.decode('utf-8', errors='replace').strip()
                if line.startswith(('JPEG_BEGIN ', 'UI_BEGIN ')):
                    kind = line.split('_', 1)[0]
                    expected_size = int(line.split()[1])
                    if not 0 < expected_size <= 1024 * 1024:
                        raise RuntimeError('Invalid JPEG size')
                    frame.clear()
                    capture = True
                elif capture and line == kind + '_END':
                    if len(frame) != expected_size:
                        raise RuntimeError('Incomplete frame transfer')
                    if kind == 'JPEG':
                        if not frame.startswith(b'\xff\xd8') or not frame.endswith(b'\xff\xd9'):
                            raise RuntimeError('Invalid JPEG markers')
                        path = args.output / 'camera.jpg'
                        path.write_bytes(frame)
                        snapshot_received = True
                    else:
                        if len(frame) != 240 * 280 * 2:
                            raise RuntimeError('Unexpected UI dimensions')
                        rgb = bytearray()
                        for i in range(0, len(frame), 2):
                            pixel = frame[i] | (frame[i + 1] << 8)
                            rgb.extend((((pixel >> 11) & 31) * 255 // 31,
                                        ((pixel >> 5) & 63) * 255 // 63,
                                        (pixel & 31) * 255 // 31))
                        path = args.output / 'screen.ppm'
                        path.write_bytes(b'P6\n240 280\n255\n' + rgb)
                    print(f'CAPTURE {path} bytes={len(frame)}', flush=True)
                    capture = False
                elif capture:
                    if re.fullmatch(r'[0-9a-f]+', line) and len(line) % 2 == 0:
                        frame.extend(bytes.fromhex(line))
                        if len(frame) > expected_size:
                            raise RuntimeError('JPEG transfer exceeded expected size')
                    else:
                        print(line, flush=True)
                else:
                    print(line, flush=True)
                    status = re.match(r'STATUS frames=(\d+)', line)
                    if args.snapshot and not snapshot_sent and status and int(status[1]) > 0:
                        port.write(b's\n')
                        snapshot_sent = True
                    elif status and commands:
                        command = commands.pop(0)
                        port.write((command + '\n').encode())
                        print('COMMAND', command, flush=True)
    finally:
        port.close()
if capture:
    raise SystemExit('Capture ended before JPEG_END')
if args.snapshot and not snapshot_received:
    raise SystemExit('No camera JPEG received')
