#!/usr/bin/env python3
"""
Serial monitor script for WiFi Test Project
Usage: python scripts/monitor.py
"""

import subprocess
import sys
import os

def main():
    print("📊 WiFi Power Consumption Test - Serial Monitor")
    print("="*50)

    # Check if we're in the right directory
    if not os.path.exists("platformio.ini"):
        print("❌ Error: platformio.ini not found. Run from Test_WiFi directory.")
        sys.exit(1)

    print("Starting serial monitor...")
    print("Press Ctrl+C to exit\n")

    # Run platformio device monitor
    try:
        subprocess.run(["pio", "device", "monitor"], cwd=os.getcwd())
    except KeyboardInterrupt:
        print("\nMonitor stopped by user.")
    except Exception as e:
        print(f"Error: {e}")
        print("Make sure:")
        print("1. ESP32 is connected and powered on")
        print("2. Serial port is correct in platformio.ini")
        print("3. PlatformIO is installed")

if __name__ == "__main__":
    main()
