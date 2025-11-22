#!/usr/bin/env python3
"""
Build script for WiFi Test Project
Usage: python scripts/build.py
"""

import subprocess
import sys
import os

def run_command(cmd, description):
    """Run shell command and return success status"""
    print(f"\n=== {description} ===")
    print(f"Command: {' '.join(cmd)}")

    try:
        result = subprocess.run(cmd, cwd=os.getcwd(), capture_output=True, text=True)
        if result.returncode == 0:
            print("✅ SUCCESS")
            if result.stdout:
                print(result.stdout[-500:])  # Show last 500 chars of output
        else:
            print(f"❌ FAILED (code: {result.returncode})")
            if result.stderr:
                print("STDERR:", result.stderr[-1000:])  # Show last 1000 chars of error
            return False
    except Exception as e:
        print(f"❌ ERROR: {e}")
        return False

    return True

def main():
    print("🚀 WiFi Power Consumption Test - Build Script")
    print("="*50)

    # Check if we're in the right directory
    if not os.path.exists("platformio.ini"):
        print("❌ Error: platformio.ini not found. Run from Test_WiFi directory.")
        sys.exit(1)

    # Check prerequisites
    try:
        import platformio
        print("✅ PlatformIO is available")
    except ImportError:
        print("❌ PlatformIO not found. Install with: pip install platformio")
        sys.exit(1)

    # Build the project
    if not run_command(["pio", "run"], "Building ESP32 Firmware"):
        print("❌ Build failed!")
        sys.exit(1)

    print("\n🎉 Build completed successfully!")
    print("\nNext steps:")
    print("1. Update network settings in lib/network_definitions/main_network_definitions.h")
    print("2. Run: python scripts/upload.py")
    print("3. Or manually: pio run --target upload")

if __name__ == "__main__":
    main()
