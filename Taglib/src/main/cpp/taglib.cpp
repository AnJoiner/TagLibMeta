#include <jni.h>
#include <string>
#include <android/log.h>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2header.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/unsynchronizedlyricsframe.h>
#include "taglib/privateframe.h"
#include "taglib/synchronizedlyricsframe.h"

#define  LOG_TAG    "taglib"
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define  LOGI(...)  __android_log_机_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define  LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_com_adaiyuns_taglib_NativeLib_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_adaiyuns_taglib_NativeLib_readAudioTag(JNIEnv* env, jobject, jstring filePath) {
    const char* path = env->GetStringUTFChars(filePath, nullptr);
    TagLib::FileRef f(path);
    if (!f.isNull() && f.tag()) {
        auto title = f.tag()->title().toCString(true);
        env->ReleaseStringUTFChars(filePath, path);
        return env->NewStringUTF(title);
    }
    return env->NewStringUTF("Error reading tags");
}

// 辅助函数：安全添加键值对到 HashMap
void safePut(JNIEnv* env, jobject hashMap, const char* key, const TagLib::String& value) {
    jclass hashMapClass = env->GetObjectClass(hashMap);
    jmethodID putMethod = env->GetMethodID(hashMapClass, "put",
                                           "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    // 创建Java字符串对象
    jstring jKey = env->NewStringUTF(key);
    jstring jValue = env->NewStringUTF(value.toCString(true)); // 强制UTF-8转换

    // 添加键值对
    env->CallObjectMethod(hashMap, putMethod, jKey, jValue);

    // 释放局部引用
    env->DeleteLocalRef(jKey);
    env->DeleteLocalRef(jValue);
}


// 处理所有可能的歌词帧
void processLyrics(JNIEnv* env, jobject hashMap, TagLib::ID3v2::Tag* id3v2Tag) {
    if (!id3v2Tag) {
        LOGE("id3v2Tag is null");
        return;
    }

    // 1. 标准非同步歌词 (USLT)
    TagLib::ID3v2::FrameList usltFrames = id3v2Tag->frameList("USLT");
    if (!usltFrames.isEmpty()) {
        for (auto* frame : usltFrames) {
            if (auto* lyricFrame = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(frame)) {
                safePut(env, hashMap, "lyrics_uslt", lyricFrame->text());
                break; // 取第一个有效帧
            }
        }
    } else {
        LOGE("usltFrames is empty");
    }

    // 2. 同步歌词 (SYLT)
    TagLib::ID3v2::FrameList syltFrames = id3v2Tag->frameList("SYLT");
    if (!syltFrames.isEmpty()) {
        for (auto* frame : syltFrames) {
            if (auto* syncFrame = dynamic_cast<TagLib::ID3v2::SynchronizedLyricsFrame*>(frame)) {
                TagLib::String lyrics;
                for (auto it = syncFrame->synchedText().begin(); it != syncFrame->synchedText().end(); ++it) {
                    lyrics += it->text + "\n"; // 合并时间戳和文本
                }
                safePut(env, hashMap, "lyrics_sylt", lyrics);
                break;
            }
        }
    } else {
        LOGE("syltFrames is empty");
    }

    // 3. 私有帧歌词 (如QQ音乐使用的PRIV帧)
    TagLib::ID3v2::FrameList privFrames = id3v2Tag->frameList("PRIV");
    for (auto* frame : privFrames) {
        if (auto* privFrame = dynamic_cast<TagLib::ID3v2::PrivateFrame*>(frame)) {
            if (privFrame->owner().find("LYRICS")) { // 常见私有帧标识
                safePut(env, hashMap, "lyrics_priv",
                        TagLib::String(privFrame->data(), TagLib::String::UTF8));
            }
        }
    }
}


extern "C" JNIEXPORT jobject JNICALL
Java_com_adaiyuns_taglib_NativeLib_getAudioMetadata(
        JNIEnv* env,
        jobject /* this */,
        jstring filePath) {

    // 1. 创建空的 HashMap 对象
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    if (hashMapClass == nullptr) return nullptr;

    jmethodID hashMapInit = env->GetMethodID(hashMapClass, "<init>", "()V");
    if (hashMapInit == nullptr) return nullptr;

    jobject hashMap = env->NewObject(hashMapClass, hashMapInit);
    if (hashMap == nullptr) return nullptr;

    // 2. 处理文件路径
    const char* path = env->GetStringUTFChars(filePath, nullptr);
    if (path == nullptr) return hashMap; // 返回空Map而不是null

    TagLib::FileRef file(path);
    env->ReleaseStringUTFChars(filePath, path);

    // 3. 检查文件有效性
    if(file.isNull() || !file.tag()) {
        return hashMap; // 返回空Map而不是null
    }

    // 4. 添加基础元数据
    TagLib::Tag* tag = file.tag();
    safePut(env, hashMap, "title", tag->title());
    safePut(env, hashMap, "artist", tag->artist());
    safePut(env, hashMap, "album", tag->album());
    safePut(env, hashMap, "year", TagLib::String::number(tag->year()));
    safePut(env, hashMap, "track", TagLib::String::number(tag->track()));
    safePut(env, hashMap, "genre", tag->genre());

    // 5. 处理ID3v2扩展标签（歌词、封面等）
    if (auto* id3v2 = dynamic_cast<TagLib::ID3v2::Tag*>(file.tag())) {
        processLyrics(env, hashMap, id3v2);
    }
    // 6. 返回前确保所有局部引用已清理
    env->DeleteLocalRef(hashMapClass);
    return hashMap;
}