#!/usr/bin/env python3
"""
Simple upload script for LoRa Transceiver project using PlatformIO.
Uploads the firmware binary to the ESP32 board.
"""

import os
import subprocess
import sys
import argparse
import json

def run_command(cmd, cwd=None):
    """Run a shell command and return True if successful."""
    try:
        result = subprocess.run(cmd, shell=True, cwd=cwd, check=True, capture_output=True, text=True)
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {cmd}")
        print(f"stdout: {e.stdout}")
        print(f"stderr: {e.stderr}")
        return False

def check_pio():
    """Check if PlatformIO is installed."""
    try:
        subprocess.run(['pio', '--version'], check=True, capture_output=True)
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

def check_binary_exists(project_root):
    """Check if firmware binary exists."""
    bin_dir = os.path.join(project_root, 'bin')
    firmware_bin = os.path.join(bin_dir, 'firmware_esp32dev.bin')
    if os.path.exists(firmware_bin):
        print(f"Firmware binary found: {firmware_bin}")
        return True
    else:
        print(f"Firmware binary not found: {firmware_bin}")
        return False

def check_venv():
    """Check if virtual environment is activated."""
    # Check if python3 is from venv
    try:
        result = subprocess.run(['which', 'python3'], capture_output=True, text=True)
        if 'venv/bin/python3' in result.stdout or 'venv\\Scripts\\python.exe' in result.stdout:
            return True
    except:
        pass
    return False

def activate_venv():
    """Activate virtual environment and restart the script."""
    print("Activating virtual environment and restarting script...")
    os.execlp('bash', 'bash', '-c', 'source venv/bin/activate && python3 upload_simple.py')

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

def main():
    parser = argparse.ArgumentParser(description="Simple upload LoRa Transceiver firmware")
    parser.add_argument('--build', action='store_true', help='Build the project before uploading')
    args = parser.parse_args()

    # Load configuration
    config = load_config()
    print("Loaded configuration:")
    print(json.dumps(config, indent=2))

    # Check virtual environment
    if not check_venv():
        print("Virtual environment not activated.")
        activate_venv()
    else:
        print("Virtual environment is activated.")

    # Get the project root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = script_dir

    print("Uploading LoRa Transceiver firmware...")

    # Check PlatformIO
    if not check_pio():
        print("PlatformIO CLI is not installed.")
        print("Install it with: pip install platformio")
        sys.exit(1)

    # Build if requested
    if args.build:
        print("Building project...")
        build_cmd = 'pio run'
        if not run_command(build_cmd, cwd=project_root):
            print("Build failed!")
            sys.exit(1)
        print("Build completed.")
    else:
        # Check if binary exists
        if not check_binary_exists(project_root):
            print("Binary not found. Use --build to compile first.")
            sys.exit(1)

    # Find serial port
    port = find_serial_port()
    if not port:
        print("No serial port found. Please connect the ESP32 board.")
        sys.exit(1)

    print(f"Using port: {port}")

    # Run upload using PlatformIO
    upload_cmd = 'pio run --target upload'

    if not run_command(upload_cmd, cwd=project_root):
        print("Upload failed!")
        sys.exit(1)

    print("Upload completed successfully!")

if __name__ == "__main__":
    main()
