package com.adaiyuns.taglib

class NativeLib {

    /**
     * A native method that is implemented by the 'taglib' native library,
     * which is packaged with this application.
     */
    external fun stringFromJNI(): String

    external fun readAudioTag(filePath: String): String

    external fun getAudioMetadata(filePath: String): Map<String, String>?

    companion object {
        // Used to load the 'taglib' library on application startup.
        init {
            System.loadLibrary("taglib")
        }
    }
}