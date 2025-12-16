#!/usr/bin/env python3
"""
Erase, build and upload script for LoRa Transceiver project using PlatformIO.
Erases the entire flash memory, builds the project, and uploads the firmware binary.
WARNING: This will erase all data including LittleFS, WiFi credentials, and configuration!
"""

import os
import subprocess
import shutil
import sys
import json

def run_command(cmd, cwd=None, env=None):
    """Run a shell command and return True if successful."""
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, check=True, capture_output=True, text=True, env=env)
        print(result.stdout)
        if result.stderr:
            print(result.stderr)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {cmd}")
        print(f"stdout: {e.stdout}")
        print(f"stderr: {e.stderr}")
        return False

def check_pio(env=None):
    """Check if PlatformIO is installed."""
    try:
        subprocess.run(['pio', '--version'], check=True, capture_output=True, env=env)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def find_serial_port():
    """Find available serial ports (only ttyACM0)."""
    port = '/dev/ttyACM0'
    if os.path.exists(port):
        print(f"Found ACM port: {port}")
        return port
    else:
        print("ACM port /dev/ttyACM0 not found. Please connect the ESP32 board.")
        return None

def load_config():
    """Load configuration from config.json."""
    config_path = 'config.json'
    if os.path.exists(config_path):
        with open(config_path, 'r') as f:
            return json.load(f)
    else:
        print("config.json not found. Using default paths.")
        return {
            "venv_activate": "venv/bin/activate"
        }

def activate_venv_if_needed(config):
    """Activate virtual environment in the current process."""
    venv_activate = config.get('venv_activate', 'venv/bin/activate')
    venv_bin = os.path.dirname(os.path.abspath(venv_activate))
    venv_root = venv_bin.replace('/bin', '')

    # Set environment variables for venv
    # IMPORTANT: Do NOT set PYTHONHOME when using venv - it causes encoding issues
    if venv_bin not in os.environ.get('PATH', ''):
        os.environ['PATH'] = venv_bin + ':' + os.environ.get('PATH', '')

    os.environ['VIRTUAL_ENV'] = venv_root

    print(f"Activated virtual environment: {venv_root}")
    return os.environ.copy()

def parse_defines():
    """Parse define values from lib/lora_config.hpp."""
    defines = {}
    config_file = 'lib/lora_config.hpp'
    if os.path.exists(config_file):
        with open(config_file, 'r') as f:
            for line in f:
                line = line.strip()
                if line.startswith('#define'):
                    parts = line.split()
                    if len(parts) >= 3:
                        key = parts[1]
                        value = ' '.join(parts[2:])
                        if value.endswith('//'):
                            value = value[:-2].strip()
                        defines[key] = value
    return defines

def main():
    # Load configuration
    config = load_config()
    print("Loaded configuration:")
    print(json.dumps(config, indent=2))

    # Activate virtual environment if needed
    env = activate_venv_if_needed(config)

    defines = parse_defines()
    print("Current firmware defines:")
    for key in ['LORA_FREQUENCY', 'LORA_POWER', 'LORA_STATUS_ENABLED', 'LORA_STATUS_INTERVAL_SEC', 'LORA_STATUS_SHORT_PACKETS', 'WIFI_ENABLE', 'WIFI_DEBUG_FIXES', 'FAKE_LORA', 'POST_INTERVAL_EN', 'POST_EN_WHEN_LORA_RECEIVED', 'POST_HOT_AS_RSSI', 'MESH_COMPATIBLE', 'DUTY_CYCLE_RECEPTION']:
        value = defines.get(key, 'Unknown')
        if key == 'LORA_FREQUENCY':
            value = value.replace('f', '') + ' MHz'
        elif key == 'LORA_POWER':
            value += ' dBm'
        print(f"  {key}={value}")

    # If MESH_COMPATIBLE=1, print mesh settings
    if defines.get('MESH_COMPATIBLE', '0') == '1':
        print("MESH settings:")
        for key in ['MESH_SYNC_WORD', 'MESH_FREQUENCY', 'MESH_BANDWIDTH', 'MESH_SPREADING_FACTOR', 'MESH_CODING_RATE']:
            value = defines.get(key, 'Unknown')
            print(f"  {key}={value}")

    # Get the project root directory (current dir has platformio.ini)
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = script_dir

    # Ensure bin directory exists
    bin_dir = os.path.join(script_dir, 'bin')
    os.makedirs(bin_dir, exist_ok=True)

    print("FLASH ERASE, BUILD AND UPLOAD MODE")
    print("WARNING: This will erase ALL data from ESP32 flash memory!")
    print("This includes:")
    print("- LittleFS filesystem (WiFi credentials, configuration)")
    print("- NVS storage (system settings)")
    print("- All user data")
    print("")

    print("Proceeding with flash erase, build and upload...")

    # Use PlatformIO with venv environment
    if not check_pio(env=env):
        print("PlatformIO CLI is not found. Installing in virtual environment...")
        install_cmd = sys.executable + " -m pip install platformio"
        if not run_command(install_cmd, env=env):
            print("Failed to install PlatformIO. Please install it manually:")
            print("source venv/bin/activate && pip install platformio")
            sys.exit(1)

        # Check again after installation
        if not check_pio(env=env):
            print("PlatformIO installation failed or not found in PATH.")
            print("Please install PlatformIO manually in the virtual environment:")
            print("source venv/bin/activate && pip install platformio")
            sys.exit(1)

    # Find serial port
    port = find_serial_port()
    if not port:
        print("No serial port found. Please connect the ESP32 board.")
        sys.exit(1)

    print(f"Using port: {port}")

    # Step 1: Erase flash
    print("Step 1: Erasing flash memory...")
    erase_cmd = 'pio run --target erase'
    if not run_command(erase_cmd, cwd=project_root, env=env):
        print("Flash erase failed!")
        sys.exit(1)
    print("Flash erase completed.")

    # Step 2: Build project
    print("Step 2: Building project...")
    build_cmd = 'pio run'
    if not run_command(build_cmd, cwd=project_root, env=env):
        print("Build failed!")
        sys.exit(1)

    # Find and copy firmware binaries
    pio_build_dir = os.path.join(project_root, '.pio', 'build')
    if os.path.exists(pio_build_dir):
        for env_dir in os.listdir(pio_build_dir):
            env_path = os.path.join(pio_build_dir, env_dir)
            if os.path.isdir(env_path):
                firmware_bin = os.path.join(env_path, 'firmware.bin')
                if os.path.exists(firmware_bin):
                    dest_bin = os.path.join(bin_dir, f'firmware_{env_dir}.bin')
                    shutil.copy2(firmware_bin, dest_bin)
                    print(f"Copied firmware to {dest_bin}")

                # Also copy .elf if exists
                firmware_elf = os.path.join(env_path, 'firmware.elf')
                if os.path.exists(firmware_elf):
                    dest_elf = os.path.join(bin_dir, f'firmware_{env_dir}.elf')
                    shutil.copy2(firmware_elf, dest_elf)
                    print(f"Copied ELF to {dest_elf}")

    # Step 3: Upload firmware
    print("Step 3: Uploading firmware...")
    upload_cmd = f'pio run --target upload --upload-port {port}'
    if not run_command(upload_cmd, cwd=project_root, env=env):
        print("Upload failed!")
        sys.exit(1)

    print("Flash erase, build and upload completed successfully!")
    print("All flash memory has been erased and new firmware uploaded.")
    print("Device will start with factory defaults.")
    print(f"Binaries are in: {bin_dir}")

    # Print diagnostic information
    print("\n" + "="*60)
    print("DIAGNOSTIC INFORMATION FOR NEXT FIRMWARE RUN")
    print("="*60)
    print("At startup, the firmware will log:")
    print("================== DIAGNOSTIC START ==================")
    print("MAC Address: [DEVICE_MAC_ADDRESS]")
    print("Server Protocol: https")
    print("Server IP: 84.252.143.212")
    print("Server Port: 443")
    print("User ID: Guest")
    print("HTTPS Settings: USE_HTTPS=1, USE_INSECURE_HTTPS=1")
    print("LittleFS Status: OK (XXXX/XXXX bytes used)")
    print("Saved WiFi - SSID: [SAVED_SSID], Password: [SET/NOT SET]")
    print("================== DIAGNOSTIC END ==================")
    print("")
    print("For each POST request, the firmware will log:")
    print("================== POST REQUEST ==================")
    print("Protocol: HTTPS")
    print("Certificate Check: DISABLED/SKIPPED")
    print("Server: 84.252.143.212:443")
    print("Request Length: XXX bytes")
    print("Attempt: X/X (direct/queued request)")
    print("Packets Sent: XXX")
    print("Failed Packets: XXX")
    print("Heap Status: XXXX bytes free, XXXX bytes min")
    print("================== POST END ==================")
    print("="*60)

if __name__ == "__main__":
    main()
