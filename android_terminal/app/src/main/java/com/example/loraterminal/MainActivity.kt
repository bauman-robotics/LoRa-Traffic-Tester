package com.example.loraterminal

import android.Manifest
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import android.content.IntentFilter
import android.content.pm.PackageManager
import android.hardware.usb.UsbDevice
import android.hardware.usb.UsbManager
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat
import kotlinx.coroutines.*
import java.nio.ByteBuffer

class MainActivity : AppCompatActivity() {

    private lateinit var usbManager: UsbManager
    private var usbDevice: UsbDevice? = null
    private var usbConnection: android.hardware.usb.UsbDeviceConnection? = null
    private var isConnected = false
    private var readJob: Job? = null

    private lateinit var connectBtn: Button
    private lateinit var commandInput: EditText
    private lateinit var sendBtn: Button
    private lateinit var terminalOutput: TextView
    private lateinit var getDebugBtn: Button
    private lateinit var readFlashBtn: Button
    private lateinit var setWifiBtn: Button
    private lateinit var statusBtn: Button

    private val ACTION_USB_PERMISSION = "com.example.loraterminal.USB_PERMISSION"

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        usbManager = getSystemService(Context.USB_SERVICE) as UsbManager

        // Initialize UI elements
        connectBtn = findViewById(R.id.connectBtn)
        commandInput = findViewById(R.id.commandInput)
        sendBtn = findViewById(R.id.sendBtn)
        terminalOutput = findViewById(R.id.terminalOutput)
        getDebugBtn = findViewById(R.id.getDebugBtn)
        readFlashBtn = findViewById(R.id.readFlashBtn)
        setWifiBtn = findViewById(R.id.setWifiBtn)
        statusBtn = findViewById(R.id.statusBtn)

        // Set up button listeners
        connectBtn.setOnClickListener { toggleConnection() }
        sendBtn.setOnClickListener { sendCommand(commandInput.text.toString()) }
        getDebugBtn.setOnClickListener { sendCommand("get debug_info") }
        readFlashBtn.setOnClickListener { sendCommand("flash") }
        setWifiBtn.setOnClickListener { /* Toggle WiFi - implement state */ sendCommand("command set wifi_en 1") }  // Simple on
        statusBtn.setOnClickListener { /* Toggle Status - implement state */ sendCommand("command set status 1") }  // Simple on

        // Register USB permission broadcast receiver
        registerReceiver(usbPermissionReceiver, IntentFilter(ACTION_USB_PERMISSION))

        // Request USB permission on start
        checkAndRequestPermissions()
    }

    private fun checkAndRequestPermissions() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.USB_PERMISSION) != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this, arrayOf(Manifest.permission.USB_PERMISSION), 1)
        } else {
            findAndConnectUSBDevice()
        }
    }

    private fun toggleConnection() {
        if (isConnected) {
            disconnect()
        } else {
            connect()
        }
    }

    private fun connect() {
        if (usbDevice == null) {
            Toast.makeText(this, "No USB device found", Toast.LENGTH_SHORT).show()
            return
        }
        usbConnection = usbManager.openDevice(usbDevice)
        if (usbConnection == null) {
            Toast.makeText(this, "Cannot open USB device", Toast.LENGTH_SHORT).show()
            return
        }
        isConnected = true
        connectBtn.text = "Disconnect"
        appendToTerminal("Connected to USB device\n")
        startReading()
    }

    private fun disconnect() {
        isConnected = false
        readJob?.cancel()
        usbConnection?.close()
        usbConnection = null
        connectBtn.text = "Connect USB Serial"
        appendToTerminal("Disconnected\n")
    }

    private fun sendCommand(command: String) {
        if (!isConnected) {
            Toast.makeText(this, "Not connected", Toast.LENGTH_SHORT).show()
            return
        }
        val data = (command + "\n").toByteArray(Charsets.UTF_8)
        usbConnection?.bulkTransfer(usbDevice?.getInterface(0)?.getEndpoint(1), data, data.size, 1000)
        appendToTerminal("TX: $command\n")
        commandInput.text.clear()
    }

    private fun startReading() {
        readJob = CoroutineScope(Dispatchers.IO).launch {
            val buffer = ByteBuffer.allocate(1024)
            while (isConnected) {
                val bytesRead = usbConnection?.bulkTransfer(usbDevice?.getInterface(0)?.getEndpoint(0), buffer.array(), buffer.capacity(), 100)
                if (bytesRead != null && bytesRead > 0) {
                    val received = String(buffer.array(), 0, bytesRead)
                    runOnUiThread {
                        appendToTerminal("RX: $received")
                    }
                }
                delay(100)
            }
        }
    }

    private fun appendToTerminal(text: String) {
        terminalOutput.append(text)
    }

    private fun findAndConnectUSBDevice() {
        val deviceList = usbManager.deviceList
        for (device in deviceList.values) {
            if (device.productId == 60000 && device.vendorId == 0x10c4) { // Example for CP210x, change as needed
                usbDevice = device
                usbManager.requestPermission(device, PendingIntent.getBroadcast(this, 0, Intent(ACTION_USB_PERMISSION), 0))
                break
            }
        }
        if (usbDevice == null) {
            Toast.makeText(this, "No compatible USB serial device found", Toast.LENGTH_SHORT).show()
        }
    }

    private val usbPermissionReceiver = object : BroadcastReceiver() {
        override fun onReceive(context: Context?, intent: Intent?) {
            if (intent?.action == ACTION_USB_PERMISSION) {
                synchronized(this) {
                    val device = intent.getParcelableExtra<UsbDevice>(UsbManager.EXTRA_DEVICE)
                    if (intent.getBooleanExtra(UsbManager.EXTRA_PERMISSION_GRANTED, false)) {
                        usbDevice = device
                        connect() // Auto connect after permission
                    } else {
                        Toast.makeText(context, "USB permission denied", Toast.LENGTH_SHORT).show()
                    }
                }
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        unregisterReceiver(usbPermissionReceiver)
        disconnect()
    }
}
