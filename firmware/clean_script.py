#!/usr/bin/env python3
"""
Clean script for LoRa Transceiver project.
Removes build artifacts, binaries, and temporary files.
"""

import os
import shutil
import sys

def remove_directory(path):
    """Remove a directory if it exists."""
    if os.path.exists(path):
        print(f"Removing directory: {path}")
        shutil.rmtree(path)
    else:
        print(f"Directory not found: {path}")

def remove_file(path):
    """Remove a file if it exists."""
    if os.path.exists(path):
        print(f"Removing file: {path}")
        os.remove(path)
    else:
        print(f"File not found: {path}")

def main():
    print("Cleaning LoRa Transceiver project...")

    # Get the project root directory
    script_dir = os.path.dirname(os.path.abspath(__file__))
    project_root = script_dir

    # Directories to remove
    dirs_to_remove = [
        '.pio',  # PlatformIO build directory
        'bin',   # Binary files directory
    ]

    # Files to remove
    files_to_remove = [
        # Add any specific files if needed
    ]

    # Remove directories
    for dir_path in dirs_to_remove:
        full_path = os.path.join(project_root, dir_path)
        remove_directory(full_path)

    # Remove files
    for file_path in files_to_remove:
        full_path = os.path.join(project_root, file_path)
        remove_file(full_path)

    print("Project cleaned successfully!")

if __name__ == "__main__":
    main()
