#!/usr/bin/env python3
import subprocess
import sys

cmd = [
    'esptool.py', '--chip', 'esp32c3', '--port', '/dev/ttyACM0', '--baud', '921600',
    'write_flash', '-z',
    '0x0', 'mesh_bin/bootloader.bin',
    '0x8000', 'mesh_bin/partitions.bin', 
    '0x10000', 'mesh_bin/firmware.bin'
]

print(f"📋 Команда: {' '.join(cmd)}")
print("-" * 80)

try:
    subprocess.run(cmd, check=True)
    print("✅ Прошивка завершена успешно!")
except Exception as e:
    print(f"❌ Ошибка: {e}")
    sys.exit(1)