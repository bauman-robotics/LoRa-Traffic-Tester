import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import serial
import serial.tools.list_ports
import threading
import time
import json
import os
import re

class LoRaTransceiverGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("LoRa Transceiver GUI")
        self.root.geometry("800x600")

        # Serial connection
        self.serial_port = None
        self.is_connected = False

        # Load config
        self.config = self.load_config()
        self.defines = self.parse_defines()

        # Create GUI elements
        self.create_widgets()

        # Start thread for reading serial data
        self.read_thread = None
        self.stop_reading = False

    def create_widgets(self):
        # Connection frame
        conn_frame = ttk.LabelFrame(self.root, text="Connection", padding="10")
        conn_frame.pack(fill="x", padx=10, pady=5)

        ttk.Label(conn_frame, text="Port:").grid(row=0, column=0, sticky="w")
        ports = self.get_ports()
        self.port_combo = ttk.Combobox(conn_frame, values=ports)
        self.port_combo.grid(row=0, column=1, padx=5, pady=5, sticky="ew")
        if "/dev/ttyACM0" in ports:
            self.port_combo.set("/dev/ttyACM0")

        # Rescan button
        self.rescan_btn = ttk.Button(conn_frame, text="Rescan", command=self.rescan_ports)
        self.rescan_btn.grid(row=0, column=3, padx=5, pady=5)

        self.connect_btn = ttk.Button(conn_frame, text="Connect", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=4, padx=5, pady=5)

        # Notebook for tabs
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill="both", expand=True, padx=10, pady=5)

        # Configuration tab
        config_frame = ttk.Frame(self.notebook)
        self.notebook.add(config_frame, text="Configuration")

        # Debug tab for network/debug logs
        debug_frame = ttk.Frame(self.notebook)
        self.notebook.add(debug_frame, text="Debug")

        # Debug controls frame
        debug_controls_frame = ttk.Frame(debug_frame)
        debug_controls_frame.pack(fill="x", padx=10, pady=5)

        # Flash log reading
        self.read_flash_btn = ttk.Button(debug_controls_frame, text="Read Flash Logs", command=self.read_flash_logs)
        self.read_flash_btn.pack(side="left", padx=5)

        # Clear debug log button
        self.clear_debug_btn = ttk.Button(debug_controls_frame, text="Clear Debug Log", command=self.clear_debug_log)
        self.clear_debug_btn.pack(side="left", padx=5)

        self.debug_text = scrolledtext.ScrolledText(debug_frame, height=20)
        self.debug_text.pack(fill="both", expand=True, padx=10, pady=10)

        # Main layout: left controls, right log
        left_frame = ttk.Frame(config_frame)
        left_frame.pack(side="left", fill="y", padx=10, pady=5)

        right_frame = ttk.Frame(config_frame)
        right_frame.pack(side="right", fill="both", expand=True, padx=10, pady=5)

        # Update parameters
        update_frame = ttk.LabelFrame(left_frame, text="Update Parameters", padding="10")
        update_frame.pack(fill="x", pady=5)

        ttk.Label(update_frame, text="Gain (dB):").grid(row=0, column=0, sticky="w")
        gain_values = [str(i) for i in range(-9, 23)]  # -9 to 22 dBm
        self.gain_combo = ttk.Combobox(update_frame, values=gain_values, state="readonly")
        self.gain_combo.set(self.config["gain"])
        self.gain_combo.grid(row=0, column=1, padx=5, pady=5, sticky="ew")

        self.set_gain_btn = ttk.Button(update_frame, text="Set Gain", command=self.set_gain)
        self.set_gain_btn.grid(row=0, column=2, padx=5, pady=5)

        ttk.Label(update_frame, text="Frequency (MHz):").grid(row=1, column=0, sticky="w")
        self.freq_entry = ttk.Entry(update_frame)
        self.freq_entry.insert(0, self.config["freq"])
        self.freq_entry.grid(row=1, column=1, padx=5, pady=5, sticky="ew")

        self.set_freq_btn = ttk.Button(update_frame, text="Set Frequency", command=self.set_frequency)
        self.set_freq_btn.grid(row=1, column=2, padx=5, pady=5)

        ttk.Label(update_frame, text="Spreading Factor:").grid(row=2, column=0, sticky="w")
        self.sf_combo = ttk.Combobox(update_frame, values=["5", "6", "7", "8", "9", "10", "11", "12"], state="readonly")
        self.sf_combo.set(self.config["spreading_factor"])
        self.sf_combo.grid(row=2, column=1, padx=5, pady=5, sticky="ew")

        self.set_sf_btn = ttk.Button(update_frame, text="Set SF", command=self.set_spreading_factor)
        self.set_sf_btn.grid(row=2, column=2, padx=5, pady=5)

        ttk.Label(update_frame, text="Bandwidth (kHz):").grid(row=3, column=0, sticky="w")
        self.bw_combo = ttk.Combobox(update_frame, values=["7.8", "10.4", "15.6", "20.8", "31.25", "41.7", "62.5", "125", "250", "500"], state="readonly")
        self.bw_combo.set(self.config["bandwidth"])
        self.bw_combo.grid(row=3, column=1, padx=5, pady=5, sticky="ew")

        self.set_bw_btn = ttk.Button(update_frame, text="Set BW", command=self.set_bandwidth)
        self.set_bw_btn.grid(row=3, column=2, padx=5, pady=5)

            # Log filter checkbox
        self.filter_var = tk.BooleanVar(value=True)
        # Remove, handled above

        # Set parameters - removed as not implemented

        # Send data
        send_frame = ttk.LabelFrame(left_frame, text="Send Data", padding="10")
        send_frame.pack(fill="x", pady=5)

        ttk.Label(send_frame, text="Data to Send:").grid(row=0, column=0, sticky="w")
        self.tx_entry = ttk.Entry(send_frame)
        self.tx_entry.grid(row=0, column=1, padx=5, pady=5, sticky="ew")

        self.send_btn = ttk.Button(send_frame, text="Send Data", command=self.send_data)
        self.send_btn.grid(row=0, column=2, padx=5, pady=5)

        # LoRa status sending
        status_frame = ttk.LabelFrame(left_frame, text="LoRa Status Sending", padding="10")
        status_frame.pack(fill="x", pady=5)

        # Status enable checkbox and interval selector
        status_top_frame = ttk.Frame(status_frame)
        status_top_frame.pack(fill="x", pady=2)

        self.lora_status_var = tk.BooleanVar(value=self.config["lora_status_enabled"])
        self.lora_status_check = ttk.Checkbutton(status_top_frame, text="Enable LoRa periodic status", variable=self.lora_status_var, command=self.toggle_lora_status)
        self.lora_status_check.pack(side="left")

        ttk.Label(status_top_frame, text="Interval (sec):").pack(side="left", padx=(10, 5))
        self.lora_status_interval_combo = ttk.Combobox(status_top_frame, values=["10", "30", "60", "120", "180"], state="readonly", width=5)
        self.lora_status_interval_combo.set(self.config["lora_status_interval"])
        self.lora_status_interval_combo.pack(side="left")
        self.lora_status_interval_combo.bind("<<ComboboxSelected>>", self.change_lora_status_interval)

        # Wifi enable checkbox
        self.wifi_var = tk.BooleanVar(value=self.config["wifi_enabled"])
        self.wifi_check = ttk.Checkbutton(left_frame, text="wifi en", variable=self.wifi_var, command=self.toggle_wifi)
        self.wifi_check.pack(pady=5)

        # POST mode radio buttons
        self.post_mode_var = tk.StringVar(value=self.config["post_mode"])
        self.post_mode_var.trace("w", lambda *args: self.change_post_mode())
        post_mode_frame = ttk.Frame(left_frame)
        post_mode_frame.pack(pady=5)
        ttk.Label(post_mode_frame, text="POST mode:").pack(side="left")
        ttk.Radiobutton(post_mode_frame, text="Periodic", variable=self.post_mode_var, value="time").pack(side="left")
        ttk.Radiobutton(post_mode_frame, text="On LoRa receive", variable=self.post_mode_var, value="lora").pack(side="left")

        # MESH Configuration frame if enabled
        if self.defines.get('MESH_COMPATIBLE', '0') == '1':
            mesh_frame = ttk.LabelFrame(left_frame, text="MESH Configuration", padding="10")
            mesh_frame.pack(fill="x", pady=5)
            ttk.Label(mesh_frame, text=f"Compatible: Yes (BW={self.defines.get('MESH_BANDWIDTH', '250')}, SF={self.defines.get('MESH_SPREADING_FACTOR', '7')}, CR={self.defines.get('MESH_CODING_RATE', '5')}, SW=0x{int(self.defines.get('MESH_SYNC_WORD', '0x2B'), 16):02X}, Freq={self.defines.get('MESH_FREQUENCY', '869.075')} MHz)").grid(row=0, column=0, columnspan=3, sticky="w")
            ttk.Label(mesh_frame, text="GUI displays mesh defines. Recompile to change.").grid(row=1, column=0, sticky="w")

        # WiFi status label
        self.wifi_status_label = ttk.Label(left_frame, text="WiFi: OFF")
        self.wifi_status_label.pack(pady=5)

        # HTTP status label
        self.http_status_label = ttk.Label(left_frame, text="HTTP: No data")
        self.http_status_label.pack(pady=5)

        # Help
        self.help_btn = ttk.Button(left_frame, text="Help", command=self.show_help)
        self.help_btn.pack(pady=5)

        # Log section on right
        log_frame = ttk.LabelFrame(right_frame, text="Log", padding="10")
        log_frame.pack(fill="both", expand=True)

        self.log_text = scrolledtext.ScrolledText(log_frame, height=8)
        self.log_text.pack(fill="both", expand=True)
        # Configure bold tag
        self.log_text.tag_configure("bold", font=("TkDefaultFont", 9, "bold"))

        # Log controls frame
        log_controls_frame = ttk.Frame(log_frame)
        log_controls_frame.pack(fill="x", pady=5)

        # Log filter checkbox
        self.filter_var = tk.BooleanVar(value=True)
        self.filter_check = ttk.Checkbutton(log_controls_frame, text="Short log", variable=self.filter_var)
        self.filter_check.pack(side="left")

        # Clear log button
        self.clear_btn = ttk.Button(log_controls_frame, text="Clear Log", command=self.clear_log)
        self.clear_btn.pack(side="right")

    def get_ports(self):
        ports = [port.device for port in serial.tools.list_ports.comports() if 'ACM' in port.device]
        if "/dev/ttyACM0" in ports:
            default_port = "/dev/ttyACM0"
        else:
            default_port = ports[0] if ports else "No ports found"
        return ports if ports else ["No ports found"]

    def rescan_ports(self):
        ports = self.get_ports()
        self.port_combo['values'] = ports
        if ports and ports[0] != "No ports found":
            self.port_combo.set(ports[0])
        self.log("Ports rescanned")

    def toggle_connection(self):
        if self.is_connected:
            self.disconnect()
        else:
            self.connect()

    def connect(self):
        port = self.port_combo.get()
        if port == "No ports found":
            messagebox.showerror("Error", "No serial port selected.")
            return
        try:
            self.serial_port = serial.Serial(port, 115200, timeout=1)
            self.is_connected = True
            self.connect_btn.config(text="Disconnect")
            self.log("Connected to " + port)
            self.start_reading()
            # Flush serial port and wait for device to be ready
            time.sleep(2)  # Wait for ESP32 boot logs to finish
            self.serial_port.reset_input_buffer()  # Flush input buffer
            self.serial_port.reset_output_buffer()  # Flush output buffer
            self.log("Serial port flushed, device ready")
            # Send command to get debug info
            self.send_command("get debug_info")
            # Also get WiFi status after connection
            self.start_polling()
            # Sync settings with device
            self.sync_settings()
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def disconnect(self):
        self.stop_reading = True
        if self.read_thread:
            self.read_thread.join()
        if self.serial_port:
            self.serial_port.close()
        self.is_connected = False
        self.connect_btn.config(text="Connect")
        self.log("Disconnected")

    def start_reading(self):
        self.stop_reading = False
        self.read_thread = threading.Thread(target=self.read_serial)
        self.read_thread.start()

    def read_serial(self):
        while not self.stop_reading:
            if self.serial_port and self.serial_port.in_waiting:
                data = self.serial_port.readline().decode('utf-8', errors='ignore').strip()
                if data:
                    if data.startswith("wifi_status "):
                        status = data.split(" ")[1].strip()
                        if status == "1":
                            self.wifi_status_label.config(text="WiFi: Connected")
                        else:
                            self.wifi_status_label.config(text="WiFi: Not Connected")
                            self.http_status_label.config(text="HTTP: WiFi disabled")
                        self.debug_log(data)
                    elif data.startswith("http_status "):
                        status = data.split(" ", 1)[1]
                        if status != "WiFi disabled":
                            self.http_status_label.config(text="HTTP: " + status)
                        self.debug_log(data)
                    elif data.startswith("gain "):
                        gain_val = data.split()[1]
                        self.gain_combo.set(gain_val)
                        self.log(f"Synced gain: {gain_val}")
                    elif data.startswith("freq "):
                        freq_val = data.split()[1]
                        self.freq_entry.delete(0, tk.END)
                        self.freq_entry.insert(0, freq_val)
                        self.log(f"Synced freq: {freq_val}")
                    elif data.startswith("sf "):
                        sf_val = data.split()[1]
                        self.sf_combo.set(sf_val)
                        self.log(f"Synced SF: {sf_val}")
                    elif data.startswith("bw "):
                        bw_val = data.split()[1]
                        self.bw_combo.set(bw_val)
                        self.log(f"Synced BW: {bw_val}")
                    elif data.startswith("status "):
                        status_val = data.split()[1]
                        self.lora_status_var.set(status_val == "1")
                        self.log(f"Synced status: {status_val}")
                    elif data.startswith("interval "):
                        interval_val = data.split()[1]
                        self.lora_status_interval_combo.set(interval_val)
                        self.log(f"Synced interval: {interval_val}")
                    elif data.startswith("wifi_en "):
                        wifi_en_val = data.split()[1]
                        self.wifi_var.set(wifi_en_val == "1")
                        self.log(f"Synced wifi_en: {wifi_en_val}")
                    elif data.startswith("post_mode "):
                        post_mode_val = data.split()[1]
                        self.post_mode_var.set(post_mode_val)
                        self.log(f"Synced post_mode: {post_mode_val}")
                    elif data.startswith("post_en "):
                        post_en_val = data.split()[1]
                        self.log(f"Synced post_en define: {post_en_val}")
                    else:
                        self.log("RX: " + data)
                        # Also log to debug tab if network related
                        if any(keyword in data for keyword in ["settings", "defines", "WiFi", "HTTP", "SPIFFS", "Control"]):
                            self.debug_log(data)
            time.sleep(0.1)

    def send_command(self, command):
        if not self.is_connected:
            messagebox.showerror("Error", "Not connected.")
            return
        try:
            full_command = command + "\n"
            self.serial_port.write(full_command.encode())
            self.log("TX: " + command)
        except Exception as e:
            messagebox.showerror("Error", str(e))

    def set_gain(self):
        gain = self.gain_combo.get()
        command = f"command update gain {gain}"
        self.send_command(command)

    def set_frequency(self):
        freq = self.freq_entry.get()
        command = f"command update freqMhz {freq}"
        self.send_command(command)

    def set_spreading_factor(self):
        sf = self.sf_combo.get()
        command = f"command update sf {sf}"
        self.send_command(command)

    def set_bandwidth(self):
        bw = self.bw_combo.get()
        command = f"command update bwKHz {bw}"
        self.send_command(command)

    # def set_output(self):  # Removed as not implemented

    def send_data(self):
        data = self.tx_entry.get()
        command = f"data {data}"
        self.send_command(command)

    def read_flash_logs(self):
        command = "flash"
        self.send_command(command)

    def show_help(self):
        help_text = """Available commands:
- command help: Show command help
- command update gain <int8_t>: Set gain
- command update freqMhz <float>: Set frequency in MHz
- command update sf <uint8_t>: Set spreading factor
- command update bwKHz <float>: Set bandwidth in kHz
- command set status <0|1>: Enable/disable automatic status sending
- command set interval <seconds>: Set status sending interval (10,30,60,120,180)
- command set wifi_en <0|1>: Enable/disable wifi connectivity
- command set post_mode <time|lora>: Set POST mode (periodic or on LoRa receive)
- data <payload>: Send data
- flash: Read flash logs
- help: Show general help"""
        messagebox.showinfo("Help", help_text)

    def log(self, message):
        # Filter out network status messages and apply short log
        if "wifi_status" in message or "http_status" in message or "get wifi_status" in message or "get http_status" in message:
            return  # Skip network status messages in main tab

        # Filter logs if checkbox is enabled
        is_lora_message = "Transmitting" in message or "Received" in message or "Tx done:" in message or "Data sent" in message
        if self.filter_var.get() and not is_lora_message:
            return  # Skip non-LoRa messages when filter is enabled

        timestamp = time.strftime("%H:%M:%S")
        # Highlight LoRa messages (transmitting/receiving over air and transmission times)
        if is_lora_message:
            # Insert with bold tag
            start_idx = self.log_text.index(tk.END + "-1c")
            self.log_text.insert(tk.END, f"[{timestamp}] {message}\n")
            end_idx = self.log_text.index(tk.END + "-1c")
            self.log_text.tag_add("bold", start_idx, end_idx)
        else:
            self.log_text.insert(tk.END, f"[{timestamp}] {message}\n")
        self.log_text.see(tk.END)

    def clear_log(self):
        self.log_text.delete(1.0, tk.END)

    def clear_debug_log(self):
        self.debug_text.delete(1.0, tk.END)

    def debug_log(self, message):
        self.debug_text.insert(tk.END, f"[{time.strftime('%H:%M:%S')}] {message}\n")
        self.debug_text.see(tk.END)

    def toggle_lora_status(self):
        state = "1" if self.lora_status_var.get() else "0"
        command = f"command set status {state}"
        self.send_command(command)

    def change_lora_status_interval(self, event=None):
        interval = self.lora_status_interval_combo.get()
        command = f"command set interval {interval}"
        self.send_command(command)

    def toggle_wifi(self):
        state = "1" if self.wifi_var.get() else "0"
        command = f"command set wifi_en {state}"
        self.send_command(command)
        if state == "1":
            self.wifi_status_label.config(text="WiFi: Connecting...")
        else:
            self.wifi_status_label.config(text="WiFi: OFF")
        # Check status after 3 seconds
        self.root.after(3000, self.get_wifi_status)

    def change_post_mode(self):
        mode = self.post_mode_var.get()
        if self.is_connected:
            command = f"command set post_mode {mode}"
            self.send_command(command)

    def get_wifi_status(self):
        command = "get wifi_status"
        self.send_command(command)
        # After WiFi status, get HTTP status
        self.root.after(0, self.get_http_status)

    def get_http_status(self):
        command = "get http_status"
        self.send_command(command)

    def start_polling(self):
        self.poll_wifi_status()

    def poll_wifi_status(self):
        if self.is_connected:
            self.get_wifi_status()

    def sync_settings(self):
        if self.is_connected:
            commands = [
                "get gain", "get freq", "get sf", "get bw",
                "get status", "get interval", "get wifi_en", "get post_mode", "get post_en"
            ]
            for cmd in commands:
                self.send_command(cmd)
                time.sleep(0.05)  # Small delay to avoid overwhelming

    def set_defaults(self):
        """Set default LoRa settings on connection."""
        time.sleep(1)  # Wait for device to be ready
        self.send_command("command update freqMhz 869.025")
        time.sleep(0.1)
        self.send_command("command update sf 12")
        time.sleep(0.1)
        self.send_command("command update bwKHz 125")
        time.sleep(0.1)
        self.send_command("command update gain 0")
        time.sleep(0.1)
        self.send_command("command set output 1")  # Enable transmitter
        self.log("Default settings applied: Freq 869.025 MHz, SF 12, BW 125 kHz, Gain 0, Output ON")

    def parse_defines(self):
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

    def load_config(self):
        config_file = "gui_config.json"
        default_config = {
            "wifi_enabled": True,
            "post_mode": "time",
            "lora_status_enabled": False,
            "lora_status_interval": "30",
            "log_short": True,
            "gain": "20",
            "freq": "868.075",
            "spreading_factor": "11",
            "bandwidth": "250"
        }
        if os.path.exists(config_file):
            try:
                with open(config_file, 'r') as f:
                    config = json.load(f)
                # Merge with default to ensure all keys
                default_config.update(config)
                return default_config
            except:
                pass
        return default_config

if __name__ == "__main__":
    root = tk.Tk()
    app = LoRaTransceiverGUI(root)
    root.mainloop()
