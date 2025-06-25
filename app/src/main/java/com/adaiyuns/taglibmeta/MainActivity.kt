package com.adaiyuns.taglibmeta

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.material3.Scaffold
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.ui.Modifier
import androidx.compose.ui.tooling.preview.Preview
import com.adaiyuns.taglib.NativeLib
import com.adaiyuns.taglibmeta.ui.theme.TagLibMetaTheme
import com.adaiyuns.taglibmeta.utils.FileUtils
import java.io.File

class MainActivity : ComponentActivity() {

    companion object {
        private const val TAG = "MainActivity"
    }

    private lateinit var nativeLib: NativeLib

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        nativeLib = NativeLib()
        val mp3Path = copyMp3()
        Log.d(TAG, "mp3=$mp3Path")
//        Log.d(TAG, "nativeLib, readAudio=${nativeLib.readAudioTag(mp3Path)}")
        // 使用示例
        val metadata = nativeLib.getAudioMetadata(mp3Path)
        metadata?.forEach { (key, value) ->
            Log.d(TAG, "$key: $value")
        }
        enableEdgeToEdge()
        setContent {
            TagLibMetaTheme {
                Scaffold(modifier = Modifier.fillMaxSize()) { innerPadding ->
                    Greeting(
                        name = "Android",
                        modifier = Modifier.padding(innerPadding)
                    )
                }
            }
        }
    }

    private fun copyMp3(): String {
        FileUtils.copy2Memory(this, "test.mp3")
        return File(externalCacheDir, "test.mp3").absolutePath
    }

}

@Composable
fun Greeting(name: String, modifier: Modifier = Modifier) {
    Text(
        text = "Hello $name!",
        modifier = modifier
    )
}

@Preview(showBackground = true)
@Composable
fun GreetingPreview() {
    TagLibMetaTheme {
        Greeting("Android")
    }
}