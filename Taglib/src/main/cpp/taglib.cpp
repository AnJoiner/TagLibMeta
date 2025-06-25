#include <jni.h>
#include <string>
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/id3v2frame.h>
#include <taglib/id3v2header.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/unsynchronizedlyricsframe.h>

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

extern "C" JNIEXPORT jobject JNICALL
Java_com_adaiyuns_taglib_NativeLib_getAudioMetadata(
        JNIEnv* env,
        jobject /* this */,
        jstring filePath) {

    const char* path = env->GetStringUTFChars(filePath, nullptr);
    TagLib::FileRef file(path);
    env->ReleaseStringUTFChars(filePath, path);

    if(file.isNull() || !file.tag()) {
        return nullptr;
    }

    // 获取基础标签
    TagLib::Tag* tag = file.tag();
    jclass hashMapClass = env->FindClass("java/util/HashMap");
    jmethodID init = env->GetMethodID(hashMapClass, "<init>", "()V");
    jobject hashMap = env->NewObject(hashMapClass, init);
    jmethodID put = env->GetMethodID(hashMapClass, "put",
                                     "(Ljava/lang/Object;Ljava/lang/Object;)Ljava/lang/Object;");

    // 添加基础元数据
    auto addField = [&](const char* key, const TagLib::String& value) {
        env->CallObjectMethod(hashMap, put,
                              env->NewStringUTF(key),
                              env->NewStringUTF(value.toCString(true)));
    };

    addField("title", tag->title());
    addField("artist", tag->artist());
    addField("album", tag->album());
    addField("year", TagLib::String::number(tag->year()));
    addField("track", TagLib::String::number(tag->track()));
    addField("genre", tag->genre());

    // 处理 ID3v2 歌词标签
    if(TagLib::ID3v2::Tag* id3v2 = dynamic_cast<TagLib::ID3v2::Tag*>(tag)) {
        TagLib::ID3v2::FrameList lyricsFrames = id3v2->frameList("USLT");
        if(!lyricsFrames.isEmpty()) {
            TagLib::ID3v2::UnsynchronizedLyricsFrame* lyricFrame =
                    dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(lyricsFrames.front());
            if(lyricFrame) {
                addField("lyrics", lyricFrame->text());
            }
        }
    }

    return hashMap;
}