package com.langua.irrigation

import android.annotation.SuppressLint
import android.bluetooth.BluetoothAdapter
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.bluetooth.le.ScanSettings
import android.content.Context
import android.location.LocationManager
import android.os.Build
import android.os.Handler
import android.os.Looper
import java.util.UUID

class BleClient(
    context: Context,
    private val onLink: (String) -> Unit,
    private val onDevice: (DeviceUi) -> Unit,
    private val onReply: (String) -> Unit,
) {
    private val app = context.applicationContext
    private val main = Handler(Looper.getMainLooper())
    private val adapter: BluetoothAdapter? =
        (app.getSystemService(Context.BLUETOOTH_SERVICE) as BluetoothManager).adapter

    private var gatt: BluetoothGatt? = null
    private var cmdChar: BluetoothGattCharacteristic? = null
    private val chunks = HashMap<Int, ByteArray>()
    private var expected = 0
    private var scanning = false
    private var seen = 0
    private val writes = ArrayDeque<String>()
    private var writing = false

    private val cccd = UUID.fromString("00002902-0000-1000-8000-00805f9b34fb")

    fun startScan() {
        val scanner = adapter?.bluetoothLeScanner
        if (adapter == null || !adapter.isEnabled || scanner == null) {
            onLink("请先打开手机蓝牙")
            return
        }
        stopScan()
        if (!locationReady()) {
            onLink("请打开系统定位。这台手机不打开定位就扫不到蓝牙设备")
            return
        }
        onLink("正在搜索 Langua…")
        val settings = ScanSettings.Builder()
            .setScanMode(ScanSettings.SCAN_MODE_LOW_LATENCY)
            .build()
        scanning = true
        seen = 0
        scanner.startScan(emptyList(), settings, scanCb)
        main.postDelayed({
            if (scanning) {
                val n = seen
                stopScan()
                onLink(
                    if (n == 0) {
                        "没扫到任何蓝牙设备。请打开系统定位，并允许本应用使用位置"
                    } else {
                        "附近有 $n 个蓝牙设备，没有 Langua。确认灌溉器已上电"
                    }
                )
            }
        }, 12000)
    }

    private fun locationReady(): Boolean {
        val lm = app.getSystemService(Context.LOCATION_SERVICE) as LocationManager
        return if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) lm.isLocationEnabled else true
    }

    fun disconnect() {
        stopScan()
        gatt?.close()
        gatt = null
        cmdChar = null
        onLink("未连接")
    }

    fun send(line: String) {
        writes.addLast(line)
        pumpWrite()
    }

    @SuppressLint("MissingPermission")
    private fun pumpWrite() {
        if (writing) return
        val line = writes.removeFirstOrNull() ?: return
        val ch = cmdChar ?: return
        val g = gatt ?: return
        writing = true
        ch.writeType = BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
        ch.value = line.toByteArray(Charsets.US_ASCII)
        if (!g.writeCharacteristic(ch)) {
            writing = false
            main.post { onReply("发送失败") }
        }
    }

    private fun isLangua(result: ScanResult): Boolean {
        val record = result.scanRecord
        val name = result.device.name ?: record?.deviceName
        if (name == Protocol.DEVICE_NAME) return true
        val uuids = record?.serviceUuids ?: return false
        return uuids.any { it.uuid.toString().equals(Protocol.SERVICE, ignoreCase = true) }
    }

    @SuppressLint("MissingPermission")
    private fun stopScan() {
        if (!scanning) return
        scanning = false
        adapter?.bluetoothLeScanner?.stopScan(scanCb)
    }

    private val scanCb = object : ScanCallback() {
        override fun onScanResult(callbackType: Int, result: ScanResult) {
            if (!scanning) return
            seen++
            if (!isLangua(result)) return
            stopScan()
            connect(result.device)
        }

        override fun onScanFailed(errorCode: Int) {
            scanning = false
            main.post { onLink("扫描失败 $errorCode") }
        }
    }

    @SuppressLint("MissingPermission")
    private fun connect(device: BluetoothDevice) {
        main.post { onLink("正在连接 ${device.address}") }
        gatt?.close()
        chunks.clear()
        gatt = device.connectGatt(app, false, gattCb, BluetoothDevice.TRANSPORT_LE)
    }

    private val gattCb = object : BluetoothGattCallback() {
        @SuppressLint("MissingPermission")
        override fun onConnectionStateChange(g: BluetoothGatt, status: Int, newState: Int) {
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                g.requestMtu(185)
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                cmdChar = null
                main.post { onLink("已断开") }
                g.close()
                if (gatt == g) gatt = null
            }
        }

        @SuppressLint("MissingPermission")
        override fun onMtuChanged(g: BluetoothGatt, mtu: Int, status: Int) {
            g.discoverServices()
        }

        @SuppressLint("MissingPermission")
        override fun onServicesDiscovered(g: BluetoothGatt, status: Int) {
            val svc = g.getService(UUID.fromString(Protocol.SERVICE))
            if (svc == null) {
                main.post { onLink("设备没有灌溉服务") }
                return
            }
            cmdChar = svc.getCharacteristic(UUID.fromString(Protocol.CMD))
            enableNotify(g, svc.getCharacteristic(UUID.fromString(Protocol.STATUS)))
            main.post { onLink("已连接") }
        }

        override fun onCharacteristicWrite(
            g: BluetoothGatt,
            characteristic: BluetoothGattCharacteristic,
            status: Int,
        ) {
            writing = false
            main.post { pumpWrite() }
        }

        override fun onCharacteristicChanged(g: BluetoothGatt, ch: BluetoothGattCharacteristic) {
            handleChange(ch, ch.value)
        }

        override fun onCharacteristicChanged(
            g: BluetoothGatt,
            ch: BluetoothGattCharacteristic,
            value: ByteArray,
        ) {
            handleChange(ch, value)
        }

        @SuppressLint("MissingPermission")
        override fun onDescriptorWrite(g: BluetoothGatt, descriptor: BluetoothGattDescriptor, status: Int) {
            val uuid = descriptor.characteristic.uuid.toString()
            if (uuid == Protocol.STATUS) {
                val reply = g.getService(UUID.fromString(Protocol.SERVICE))
                    ?.getCharacteristic(UUID.fromString(Protocol.REPLY))
                enableNotify(g, reply)
            } else if (uuid == Protocol.REPLY) {
                send("time ${System.currentTimeMillis() / 1000}")
            }
        }
    }

    @SuppressLint("MissingPermission")
    private fun enableNotify(g: BluetoothGatt, ch: BluetoothGattCharacteristic?) {
        if (ch == null) return
        g.setCharacteristicNotification(ch, true)
        val d = ch.getDescriptor(cccd) ?: return
        d.value = BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE
        g.writeDescriptor(d)
    }

    private fun handleChange(ch: BluetoothGattCharacteristic, value: ByteArray?) {
        if (value == null) return
        when (ch.uuid.toString()) {
            Protocol.STATUS -> onStatusChunk(value)
            Protocol.REPLY -> main.post { onReply(value.toString(Charsets.UTF_8)) }
        }
    }

    private fun onStatusChunk(frame: ByteArray) {
        if (frame.size < 4 || frame[0] != 0xA5.toByte()) return
        val index = frame[1].toInt() and 0xFF
        val total = frame[2].toInt() and 0xFF
        if (total == 0) return
        if (index == 0) {
            chunks.clear()
            expected = total
        }
        chunks[index] = frame.copyOfRange(3, frame.size)
        if (chunks.size < expected) return
        val payload = ByteArray(chunks.entries.sortedBy { it.key }.sumOf { it.value.size })
        var p = 0
        for (i in 0 until expected) {
            val part = chunks[i] ?: return
            part.copyInto(payload, p)
            p += part.size
        }
        val ui = Protocol.parse(payload) ?: return
        main.post { onDevice(ui) }
    }
}
