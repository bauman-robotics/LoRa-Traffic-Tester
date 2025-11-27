#!/usr/bin/env python3
"""
Build script for LoRa Transceiver project using PlatformIO.
Creates build and bin directories, compiles the project, and copies the firmware binary.
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

    print("Building LoRa Transceiver project...")

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

    build_cmd = 'pio run'
    build_cwd = project_root

    # Run build with venv
    if not run_command(build_cmd, cwd=build_cwd, env=env):
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

    print("Build completed successfully!")
    print(f"Binaries are in: {bin_dir}")

if __name__ == "__main__":
    main()
