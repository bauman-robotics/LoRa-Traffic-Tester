#!/usr/bin/env python3
"""
Pre-build script for WiFi Test Project
Displays build configuration and checks for network settings
"""

import os
import sys

def get_define_value(filename, define_name):
    """Extract define value from C++ header file"""
    try:
        with open(filename, 'r') as f:
            for line in f:
                if f'#define {define_name}' in line:
                    parts = line.strip().split()
                    if len(parts) >= 3:
                        return parts[2].strip('"')
                    return "undefined"
    except:
        return "not found"
    return "undefined"

def main():
    print("=== WiFi Power Consumption Test - Build Configuration ===")

    # Network settings
    network_file = "lib/network_definitions/main_network_definitions.h"
    if os.path.exists(network_file):
        print("\nNetwork Settings:")
        print(f"  SSID: {get_define_value(network_file, 'DEFAULT_WIFI_SSID')}")
        print(f"  API Key: {get_define_value(network_file, 'DEFAULT_API_KEY')}")
        print(f"  Server: {get_define_value(network_file, 'DEFAULT_SERVER_IP')}:{get_define_value(network_file, 'DEFAULT_SERVER_PATH')}")

    # Test settings
    config_file = "wifi_test_config.hpp"
    if os.path.exists(config_file):
        print("\nTest Configuration:")
        print(f"  POST Interval: {get_define_value(config_file, 'POST_INTERVAL_MS')} ms")
        print(f"  WiFi TX Power: {get_define_value(config_file, 'WIFI_TX_POWER_VARIANT')}")

    print("\nBuild environment check:")
    print(f"  Python version: {sys.version.split()[0]}")
    print(f"  Current directory: {os.getcwd()}")
    print(f"  Source files present: {os.path.exists('src/main.cpp')}")

    # Warning if network settings not configured
    ssid = get_define_value(network_file, "DEFAULT_WIFI_SSID")
    if ssid == "Your_WIFI_SSID":
        print("\n⚠️  WARNING: Network settings not configured!")
        print("   Please update lib/network_definitions/main_network_definitions.h")
        print("   with your WiFi SSID, password, and server IP.")

    print("\n=== Build Started ===")

if __name__ == "__main__":
    main()
