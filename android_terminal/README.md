# LoRa Terminal Android App

This is a simple Android terminal application for communicating with a LoRa transceiver device over USB serial. It provides a GUI similar to the desktop `gui.py` script, with buttons for common commands and a text-based terminal for sending custom commands and viewing responses.

## Features

- USB serial connection to LoRa device
- Send custom commands via text input
- Pre-defined buttons for common commands:
  - Get Debug Info
  - Read Flash Logs
  - Toggle WiFi
  - Toggle Status Sending
- Terminal output display for TX/RX data

## Setup

1. Open this project in Android Studio.
2. Ensure your device supports USB host mode (most Android devices do).
3. Install the app on your Android device.
4. Connect your LoRa transceiver (e.g., ESP32) via USB OTG cable.
5. Grant USB permissions when prompted.
6. Use the app to connect and send commands.

## USB Configuration

The app is configured to detect CP210x USB-to-serial chips (common in ESP32 development boards) with VID `0x10C4` and PID `0xEA60`. If your device uses a different chip, modify `MainActivity.kt` in the `findAndConnectUSBDevice()` method to match your device's VID/PID.

## Permissions

The app requires:
- USB permission for hardware access
- USB host feature

## Dependencies

- AndroidX libraries
- Kotlin Coroutines for async operations

## Note

This implementation provides basic USB serial communication. For more robust serial communication, consider using libraries like [usb-serial-for-android](https://github.com/felHR85/UsbSerial).

## Commands Reference

Common commands (based on device firmware):
- `get debug_info`: Get device debug information
- `flash`: Read flash logs
- `command set wifi_en <0|1>`: Enable/disable WiFi
- `command set status <0|1>`: Enable/disable periodic status
- `command update gain <value>`: Set TX gain
- `command update freqMhz <freq>`: Set frequency
- `command update sf <value>`: Set spreading factor
- `command update bwKHz <value>`: Set bandwidth
- `data <payload>`: Send data payload
