#!/usr/bin/env python3
"""Run the installed ESP-IDF toolchain with optional offline component reuse."""
import argparse
import os
from pathlib import Path
import subprocess
import sys

parser = argparse.ArgumentParser()
parser.add_argument('--idf', type=Path, default=Path(os.environ.get('IDF_PATH', str(Path.home() / 'esp/esp-idf'))))
parser.add_argument('--cache', type=Path)
parser.add_argument('actions', nargs='*', default=['build'])
args = parser.parse_args()
env = os.environ.copy()
export = subprocess.run(
    [sys.executable, str(args.idf / 'tools/idf_tools.py'), 'export', '--format', 'key-value'],
    capture_output=True, text=True, check=True,
)
for line in export.stdout.splitlines():
    if '=' in line:
        key, value = line.split('=', 1)
        env[key] = value.replace('$PATH', env.get('PATH', ''))
env['IDF_PATH'] = str(args.idf)
if args.cache:
    env['DEMO_COMPONENT_CACHE'] = str(args.cache.resolve())
    # Ninja's bootloader subprocess also needs this setting, outside CMake's scope.
    env['IDF_COMPONENT_MANAGER'] = '0'
result = subprocess.run(
    [sys.executable, str(args.idf / 'tools/idf.py'), *args.actions],
    cwd=Path(__file__).resolve().parents[1], env=env,
)
sys.exit(result.returncode)
