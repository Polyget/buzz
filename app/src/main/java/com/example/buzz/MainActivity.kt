package com.example.buzz

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.compose.foundation.Canvas
import androidx.compose.foundation.layout.*
import androidx.compose.material3.*
import androidx.compose.runtime.*
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.Stroke
import androidx.compose.ui.unit.dp
import java.io.BufferedReader
import java.io.InputStreamReader

class MainActivity : ComponentActivity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContent {
            BuzzApp()
        }
    }

    companion object {
        init {
            System.loadLibrary("native-lib")
        }
    }
}

@Composable
fun BuzzApp() {
    var rootStatus by remember { mutableStateOf(false) }
    var frequency by remember { mutableStateOf("4625 kHz") }
    var tuned by remember { mutableStateOf(false) }
    var log by remember { mutableStateOf("") }
    var audioData by remember { mutableStateOf(byteArrayOf()) }

    LaunchedEffect(Unit) {
        rootStatus = checkRoot()
        log = getLogs()
    }

    Column(modifier = Modifier.fillMaxSize().padding(16.dp)) {
        Text("Root Status: ${if (rootStatus) "Granted" else "Not Granted"}")
        TextField(value = frequency, onValueChange = {}, enabled = false, label = { Text("Frequency") })
        Button(onClick = {
            if (rootStatus) {
                tuneToFrequency(4625000L)
                tuned = !tuned
                val raw = readRawData()
                if (raw != null) {
                    audioData = demodulateAM(raw)
                }
                log = getLogs()
            }
        }) {
            Text(if (tuned) "Tuned" else "Direct Hardware Tune")
        }
        // Audio visualization
        Canvas(modifier = Modifier.fillMaxWidth().height(200.dp)) {
            if (audioData.isNotEmpty()) {
                val path = Path()
                audioData.forEachIndexed { index, byte ->
                    val x = index.toFloat() / audioData.size * size.width
                    val y = size.height / 2 + byte.toFloat() / 128 * size.height / 2
                    if (index == 0) path.moveTo(x, y) else path.lineTo(x, y)
                }
                drawPath(path, Color.Blue, style = Stroke(width = 2f))
            }
        }
        Text("Audio Log / Kernel Logs:")
        Text(log, modifier = Modifier.fillMaxHeight())
    }
}

fun checkRoot(): Boolean {
    return try {
        val process = Runtime.getRuntime().exec(arrayOf("su", "-c", "echo test"))
        process.waitFor() == 0
    } catch (e: Exception) {
        false
    }
}

fun getLogs(): String {
    return try {
        val process = Runtime.getRuntime().exec(arrayOf("su", "-c", "dmesg | tail -20"))
        val reader = BufferedReader(InputStreamReader(process.inputStream))
        val log = reader.readText()
        process.waitFor()
        log
    } catch (e: Exception) {
        "Error reading logs: ${e.message}"
    }
}

external fun tuneToFrequency(freqHz: Long)
external fun readRawData(): ByteArray?
external fun demodulateAM(rawData: ByteArray): ByteArray