#!/usr/bin/env python3
"""
Upload script for WiFi Test Project
Usage: python scripts/upload.py
"""

import subprocess
import sys
import os
import serial.tools.list_ports

def run_command(cmd, description):
    """Run shell command and return success status"""
    print(f"\n=== {description} ===")
    print(f"Command: {' '.join(cmd) if isinstance(cmd, list) else cmd}")

    try:
        result = subprocess.run(cmd if isinstance(cmd, list) else cmd.split(),
                              cwd=os.getcwd(), capture_output=False, text=True)
        if result.returncode == 0:
            print("✅ SUCCESS")
            return True
        else:
            print(f"❌ FAILED (code: {result.returncode})")
            return False
    except Exception as e:
        print(f"❌ ERROR: {e}")
        return False

def list_serial_ports():
    """List available serial ports"""
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("No serial ports found!")
        return []

    print("Available serial ports:")
    for i, port in enumerate(ports):
        print(f"  {i+1}. {port.device} - {port.description}")

    return [port.device for port in ports]

def main():
    print("🚀 WiFi Power Consumption Test - Upload Script")
    print("="*50)

    # Check if we're in the right directory
    if not os.path.exists("platformio.ini"):
        print("❌ Error: platformio.ini not found. Run from Test_WiFi directory.")
        sys.exit(1)

    # Check build exists
    firmware_path = ".pio/build/esp32dev/firmware.bin"
    if not os.path.exists(firmware_path):
        print("❌ Error: Firmware not built. Run 'python scripts/build.py' first.")
        sys.exit(1)

    # Show available ports
    available_ports = list_serial_ports()

    # Get upload port from platformio.ini or ask user
    port = None
    try:
        with open("platformio.ini", "r") as f:
            for line in f:
                if "upload_port" in line and not line.strip().startswith(";"):
                    port = line.split("=")[1].strip()
                    break
    except:
        pass

    if port and port in available_ports:
        print(f"Using configured port: {port}")
    elif available_ports:
        print(f"Using first available port: {available_ports[0]}")
        port = available_ports[0]
        # Update platformio.ini
        try:
            with open("platformio.ini", "r") as f:
                content = f.read()
            content = content.replace("upload_port = /dev/ttyUSB0", f"upload_port = {port}")
            with open("platformio.ini", "w") as f:
                f.write(content)
            print(f"Updated platformio.ini with port: {port}")
        except Exception as e:
            print(f"Could not update platformio.ini: {e}")
    else:
        print("❌ No serial ports available. Check USB connection.")
        sys.exit(1)

    print(f"\n📡 Uploading to: {port}")
    print("⚠️  Make sure ESP32 is in boot mode (BOOT button pressed)")

    # Ask for confirmation
    try:
        response = input("\nContinue with upload? [y/N]: ").lower().strip()
        if response not in ['y', 'yes']:
            print("Upload cancelled.")
            return
    except KeyboardInterrupt:
        print("\nUpload cancelled.")
        return

    # Upload the firmware
    if run_command(["pio", "run", "--target", "upload"], "Uploading Firmware to ESP32"):
        print("\n🎉 Upload completed successfully!")
        print("\nNext steps:")
        print("1. Release BOOT button on ESP32")
        print("2. Monitor serial output: pio device monitor")
        print("   Or run: python scripts/monitor.py")
        print("\nDevice should connect to WiFi and start sending POST requests every 10 seconds")
    else:
        print("❌ Upload failed!")
        print("\nTroubleshooting tips:")
        print("1. Check USB connection")
        print("2. Press and hold BOOT button while uploading")
        print("3. Try different USB port or cable")
        print("4. Check serial port permissions")

if __name__ == "__main__":
    # Check if pyserial is available
    try:
        import serial.tools.list_ports
    except ImportError:
        print("❌ pyserial not installed. Install with: pip install pyserial")
        sys.exit(1)

    main()
