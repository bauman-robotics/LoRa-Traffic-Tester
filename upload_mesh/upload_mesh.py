#!/usr/bin/env python3
"""
Скрипт для прошивки ESP32-C3 Mesh-прошивкой
"""

import os
import sys
import subprocess
import serial.tools.list_ports
import time
import glob

# Конфигурация
FIRMWARE_FILES = {
    'bootloader.bin': '0x0',
    'partitions.bin': '0x8000', 
    'firmware.bin': '0x10000'
}

BAUD_RATE = 921600
CHIP_TYPE = 'esp32c3'
BIN_DIR = 'mesh_bin'  # Папка с бинарными файлами

def check_esptool():
    """Проверка доступности esptool"""
    print("🔧 Проверка esptool...")
    
    # Пробуем разные способы запуска esptool
    commands_to_try = [
        ['esptool.py', 'version'],
        [sys.executable, '-m', 'esptool', 'version'],
    ]
    
    for cmd in commands_to_try:
        try:
            result = subprocess.run(cmd, capture_output=True, text=True, timeout=10)
            if result.returncode == 0:
                print(f"✅ esptool найден: {cmd}")
                return cmd[0] if cmd[0] != sys.executable else [sys.executable, '-m', 'esptool']
        except (FileNotFoundError, subprocess.TimeoutExpired, subprocess.CalledProcessError):
            continue
    
    # Последняя попытка - просто проверим импорт
    try:
        import esptool
        print(f"✅ esptool доступен для импорта: версия {esptool.__version__}")
        return [sys.executable, '-m', 'esptool']
    except ImportError:
        print("❌ esptool не найден. Установите:")
        print("   pip install esptool")
        return None

def find_esp_port():
    """Поиск порта ESP32-C3 в Linux"""
    print("🔍 Поиск ESP32-C3...")
    
    # Список возможных портов для ESP32 в Linux
    possible_ports = []
    
    # Ищем ACM порты (современные ESP32)
    acm_ports = glob.glob('/dev/ttyACM*')
    possible_ports.extend(acm_ports)
    
    # Ищем USB порты (старые версии)
    usb_ports = glob.glob('/dev/ttyUSB*')
    possible_ports.extend(usb_ports)
    
    # Также используем pyserial для поиска
    try:
        ports = serial.tools.list_ports.comports()
        for port in ports:
            if port.device not in possible_ports:
                possible_ports.append(port.device)
    except Exception as e:
        print(f"⚠️  Ошибка при поиске портов: {e}")
    
    # Фильтруем существующие порты
    available_ports = []
    for port in possible_ports:
        if os.path.exists(port):
            available_ports.append(port)
    
    if available_ports:
        print(f"📡 Найдены порты: {available_ports}")
        
        # Пытаемся определить какой порт - ESP32
        for port in available_ports:
            if 'ACM' in port:
                print(f"🎯 Рекомендуемый порт (ESP32-C3): {port}")
                return port
        
        # Если ACM не нашли, возвращаем первый доступный
        return available_ports[0]
    else:
        print("❌ ESP32-C3 не найден.")
        print("   Подключите устройство и проверьте:")
        print("   - Кабель USB")
        print("   - Драйверы CP210x/CH340")
        print("   - Права доступа: sudo usermod -aG dialout $USER")
        return None

def check_files_exist():
    """Проверка наличия необходимых файлов"""
    print("📁 Проверка файлов прошивки...")
    missing_files = []
    
    for file in FIRMWARE_FILES.keys():
        file_path = os.path.join(BIN_DIR, file)
        if not os.path.exists(file_path):
            missing_files.append(file)
        else:
            size = os.path.getsize(file_path)
            print(f"   ✅ {file} - {size} bytes")
    
    if missing_files:
        print(f"❌ Отсутствуют файлы: {missing_files}")
        print(f"   Убедитесь, что папка '{BIN_DIR}' содержит все необходимые файлы")
        return False
    
    print("✅ Все файлы найдены!")
    return True

def flash_esp32(port, esptool_cmd):
    """Прошивка ESP32-C3"""
    try:
        # Формируем команду esptool
        if isinstance(esptool_cmd, list):
            cmd = esptool_cmd.copy()
        else:
            cmd = [esptool_cmd]
        
        cmd.extend([
            '--chip', CHIP_TYPE,
            '--port', port,
            '--baud', str(BAUD_RATE),
            'write_flash',
            '-z'
        ])
        
        # Добавляем файлы и адреса
        for file, address in FIRMWARE_FILES.items():
            file_path = os.path.join(BIN_DIR, file)
            cmd.extend([address, file_path])
        
        print("\n🚀 Запуск прошивки...")
        print(f"📋 Команда: {' '.join(cmd)}")
        print("-" * 60)
        
        # Запускаем процесс
        result = subprocess.run(cmd, capture_output=True, text=True)
        
        if result.returncode == 0:
            print("✅ Прошивка успешно завершена!")
            if result.stdout:
                print("Вывод esptool:")
                print(result.stdout)
            return True
        else:
            print("❌ Ошибка при прошивке:")
            if result.stderr:
                print(result.stderr)
            if result.stdout:
                print("Вывод esptool:")
                print(result.stdout)
            return False
            
    except Exception as e:
        print(f"❌ Ошибка выполнения: {e}")
        return False

def main():
    """Основная функция"""
    print("=" * 60)
    print("    ESP32-C3 Mesh Flasher")
    print("=" * 60)
    
    # Показываем структуру
    print(f"📁 Рабочая директория: {os.path.abspath('.')}")
    
    # Проверяем наличие esptool
    esptool_cmd = check_esptool()
    if not esptool_cmd:
        sys.exit(1)
    
    # Проверяем наличие файлов
    if not check_files_exist():
        sys.exit(1)
    
    # Ищем порт
    port = find_esp_port()
    if not port:
        print("\n💡 Введите порт вручную:")
        print("   Например: /dev/ttyACM0, /dev/ttyACM1, /dev/ttyUSB0")
        port = input("📡 Порт: ").strip()
    
    if not port:
        print("❌ Порт не указан")
        sys.exit(1)
    
    print(f"🎯 Используется порт: {port}")
    
    # Подтверждение
    print("\n📋 Файлы для прошивки:")
    for file, address in FIRMWARE_FILES.items():
        file_path = os.path.join(BIN_DIR, file)
        size = os.path.getsize(file_path)
        print(f"   {address}: {file} ({size} bytes)")
    
    print(f"\n⚠️  Убедитесь, что ESP32-C3 находится в режиме загрузки!")
    print("   Для ESP32-C3:")
    print("   1. Зажмите кнопку BOOT")
    print("   2. Нажмите и отпустите кнопку RESET")
    print("   3. Отпустите кнопку BOOT")
    print("   4. Устройство готово к прошивке")
    
    confirm = input("\n🎯 Продолжить прошивку? (y/N): ").strip().lower()
    if confirm not in ['y', 'yes', 'д', 'да']:
        print("⏹️  Отменено")
        sys.exit(0)
    
    # Прошиваем
    print("\n🔄 Начинается прошивка...")
    print("   Это займет несколько секунд...")
    
    if flash_esp32(port, esptool_cmd):
        print("\n🎉 Прошивка завершена успешно!")
        print("💡 Устройство автоматически перезагрузится и запустит Mesh-прошивку")
    else:
        print("\n💥 Прошивка не удалась")
        print("💡 Попробуйте:")
        print("   - Проверить подключение устройства")
        print("   - Убедиться, что устройство в режиме загрузки")
        print("   - Попробовать другой USB порт")
        print("   - Перезагрузить устройство")
        sys.exit(1)

if __name__ == "__main__":
    main()