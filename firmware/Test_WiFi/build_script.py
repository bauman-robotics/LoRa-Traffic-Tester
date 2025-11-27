#!/usr/bin/env python3
"""
Build script for WiFi Power Consumption Test project.
Similar to main project's build_script.py but for Test_WiFi.
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

def activate_venv_if_needed():
    """Activate virtual environment in the current process."""
    venv_activate = os.path.join('..', 'venv', 'bin', 'activate')
    venv_bin = os.path.join('..', 'venv', 'bin')
    venv_root = os.path.join('..', 'venv')

    if os.path.exists(venv_activate):
        # Set environment variables for venv
        if venv_bin not in os.environ.get('PATH', ''):
            os.environ['PATH'] = venv_bin + ':' + os.environ.get('PATH', '')

        os.environ['VIRTUAL_ENV'] = venv_root

        print(f"Activated virtual environment: {venv_root}")
        return os.environ.copy()
    else:
        print("Virtual environment not found. Using system paths.")
        return os.environ.copy()

def parse_test_defines():
    """Parse define values from wifi_test_config.hpp."""
    defines = {}
    config_files = ['wifi_test_config.hpp', 'lib/network_definitions/main_network_definitions.h']

    for config_file in config_files:
        if os.path.exists(config_file):
            with open(config_file, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line.startswith('#define'):
                        parts = line.split()
                        if len(parts) >= 3:
                            key = parts[1]
                            value = ' '.join(parts[2:])
                            if '//' in value:
                                value = value.split('//')[0].strip()
                            elif '/*' in value:
                                value = value.split('/*')[0].strip()
                            defines[key] = value
    return defines

def main():
    print("=" * 60)
    print("WiFi Power Consumption Test - Build Script")
    print("=" * 60)

    # Check if we're in Test_WiFi directory
    if not os.path.exists('wifi_test_config.hpp'):
        print("Error: Run this script from Test_WiFi directory!")
        print("Usage: cd Test_WiFi && python build_script.py")
        sys.exit(1)

    # Load venv environment
    env = activate_venv_if_needed()

    # Parse config for this project
    defines = parse_test_defines()
    print("\nWiFi Test Project defines:")
    for key in ['POST_HOT_VALUE', 'POST_COLD_VALUE', 'POST_INTERVAL_MS',
                'DEFAULT_WIFI_SSID', 'DEFAULT_API_KEY', 'DEFAULT_SERVER_IP']:
        value = defines.get(key, 'Not set')
        if value.startswith('"') and value.endswith('"'):
            value = value[1:-1]  # Remove quotes
        if 'PASSWORD' in key.upper():
            value = '****'  # Hide password
        print(f"  {key}={value}")

    # Ensure bin directory exists
    bin_dir = 'bin'
    os.makedirs(bin_dir, exist_ok=True)

    print("\nBuilding WiFi Test project...")

    # Check and install PlatformIO if needed
    if not check_pio(env=env):
        print("PlatformIO CLI is not found. Installing in virtual environment...")
        install_cmd = sys.executable + " -m pip install platformio"
        if not run_command(install_cmd, env=env):
            print("Failed to install PlatformIO. Please install manually:")
            print("source ../venv/bin/activate && pip install platformio")
            sys.exit(1)

        # Check again after installation
        if not check_pio(env=env):
            print("PlatformIO installation failed.")
            sys.exit(1)

    # Build the project
    build_cmd = 'pio run'
    if not run_command(build_cmd, cwd='.', env=env):
        print("Build failed!")
        sys.exit(1)

    # Find and copy firmware binaries
    pio_build_dir = '.pio/build'
    if os.path.exists(pio_build_dir):
        for env_dir in os.listdir(pio_build_dir):
            env_path = os.path.join(pio_build_dir, env_dir)
            if os.path.isdir(env_path):
                firmware_bin = os.path.join(env_path, 'firmware.bin')
                if os.path.exists(firmware_bin):
                    dest_bin = os.path.join(bin_dir, f'firmware_{env_dir}.bin')
                    shutil.copy2(firmware_bin, dest_bin)
                    print(f"Copied firmware to {dest_bin}")

                    # Get file size
                    size = os.path.getsize(dest_bin) / 1024  # KB
                    print(f"Firmware size: {size:.1f} KB")

    print("\n" + "=" * 60)
    print("🚀 Build completed successfully!")
    print(f"📁 Binaries are in: {os.path.abspath(bin_dir)}")
    print("\nNext steps:")
    print("1. Configure network settings in lib/network_definitions/main_network_definitions.h")
    print("2. Run: python ../upload_script.py  (from Test_WiFi dir)")
    print("   Or: python upload.py  (after copying to Test_WiFi)")
    print("=" * 60)

if __name__ == "__main__":
    main()
