#include <jni.h>

#include "srt/srt.h"
#include "srt/logging_api.h"

#include "log.h"
#include "enums.h"
#include "structs.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TAG "SRTJniGlue"

// SRT Logger callback
void srt_logger(void *opaque, int level, const char *file, int line, const char *area,
                const char *message) {
    int android_log_level = ANDROID_LOG_UNKNOWN;

    switch (level) {
        case LOG_CRIT:
            android_log_level = ANDROID_LOG_FATAL;
            break;
        case LOG_ERR:
            android_log_level = ANDROID_LOG_ERROR;
            break;
        case LOG_WARNING:
            android_log_level = ANDROID_LOG_WARN;
            break;
        case LOG_NOTICE:
            android_log_level = ANDROID_LOG_INFO;
            break;
        case LOG_DEBUG:
            android_log_level = ANDROID_LOG_DEBUG;
            break;
        default:
            LOGE(TAG, "Unknown log level %d", level);
    }

    __android_log_print(android_log_level, "libsrt", "%s@%d:%s %s", file, line, area, message);
}

// Library Initialization
JNIEXPORT jint JNICALL
nativeStartUp(JNIEnv *env, jobject obj) {
    srt_setloghandler(nullptr, srt_logger);
    return srt_startup();
}

JNIEXPORT jint JNICALL
nativeCleanUp(JNIEnv *env, jobject obj) {
    return srt_cleanup();
}

// Creating and configuring sockets
JNIEXPORT jint JNICALL
nativeSocket(JNIEnv *env, jobject obj,
             jobject jaf,
             jint jtype,
             jint jprotocol) {
    int af = address_family_from_java_to_native(env, jaf);
    if (af <= 0) {
        LOGE(TAG, "Bad value for address family");
        return af;
    }

    return srt_socket(af, jtype, jprotocol);
}

JNIEXPORT jint JNICALL
nativeCreateSocket(JNIEnv *env, jobject obj) {
    return srt_create_socket();
}

JNIEXPORT jint JNICALL
nativeBind(JNIEnv *env, jobject ju, jobject inetSocketAddress) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    const struct sockaddr_in *sa = inet_socket_address_from_java_to_native(env,
                                                                           inetSocketAddress);

    int res = srt_bind(u, (struct sockaddr *) sa, sizeof(*sa));

    if (!sa) {
        free((void *) sa);
    }

    return res;
}

JNIEXPORT jobject JNICALL
nativeGetSockState(JNIEnv *env, jobject ju) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    SRT_SOCKSTATUS sock_status = srt_getsockstate((SRTSOCKET) u);

    return srt_sock_status_from_native_to_java(env, sock_status);
}

JNIEXPORT jint JNICALL
nativeClose(JNIEnv *env, jobject ju) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);

    return srt_close((SRTSOCKET) u);
}

// Connecting
JNIEXPORT jint JNICALL
nativeListen(JNIEnv *env, jobject ju, jint backlog) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);

    return srt_listen((SRTSOCKET) u, (int) backlog);
}

JNIEXPORT jobject JNICALL
nativeAccept(JNIEnv *env, jobject ju) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    auto *sa = (struct sockaddr_in *) malloc(sizeof(struct sockaddr));
    int addrlen = 0;
    jobject inetSocketAddress = nullptr;

    SRTSOCKET new_u = srt_accept((SRTSOCKET) u, (struct sockaddr *) &sa, &addrlen);
    if (addrlen != 0) {
        inet_socket_address_from_native_to_java(env, sa, addrlen);
    }
    jobject res = create_java_pair(env, srt_socket_from_native_to_java(env, new_u),
                                   inetSocketAddress);

    if (!sa) {
        free((void *) sa);
    }

    return res;
}

JNIEXPORT jint JNICALL
nativeConnect(JNIEnv *env, jobject ju, jobject inetSocketAddress) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    const struct sockaddr_in *sa = inet_socket_address_from_java_to_native(env,
                                                                           inetSocketAddress);
    int res = srt_connect((SRTSOCKET) u, (struct sockaddr *) sa, sizeof(*sa));

    if (!sa) {
        free((void *) sa);
    }

    return res;
}

// Options and properties
JNIEXPORT jint JNICALL
nativeSetSockOpt(JNIEnv *env,
                 jobject ju,
                 jint level /*ignored*/,
                 jobject jopt,
                 jobject
                 joptval) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    int opt = srt_sockopt_from_java_to_native(env, jopt);
    if (opt <= 0) {
        LOGE(TAG, "Bad value for SRT option");
        return opt;
    }
    int optlen = 0;
    const void *optval = srt_optval_from_java_to_native(env, joptval, &optlen);

    int res = srt_setsockopt((SRTSOCKET) u,
                             level /*ignored*/, (SRT_SOCKOPT) opt, optval, optlen);

    if (!optval) {
        free((void *) optval);
    }

    return
            res;
}

// Transmission
JNIEXPORT jint JNICALL
nativeSend(JNIEnv *env, jobject ju, jbyteArray jbuf) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    int len = env->GetArrayLength(jbuf);
    char *buf = (char *) env->GetByteArrayElements(jbuf, nullptr);

    int res = srt_send(u, buf, len);

    env->ReleaseByteArrayElements(jbuf, (jbyte *) buf, 0);

    return res;
}

JNIEXPORT jint JNICALL
nativeSendMsg(JNIEnv *env,
              jobject ju,
              jbyteArray jbuf,
              jint jttl/* = -1*/,
              jboolean jinorder/* = false*/) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    int len = env->GetArrayLength(jbuf);
    char *buf = (char *) env->GetByteArrayElements(jbuf, nullptr);

    int res = srt_sendmsg(u, buf, len, jttl, jinorder);

    env->ReleaseByteArrayElements(jbuf, (jbyte *) buf, 0);

    return res;
}

JNIEXPORT jint JNICALL
nativeSendMsg2(JNIEnv *env,
               jobject ju,
               jbyteArray jbuf,
               jobject jmsgCtrl) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    SRT_MSGCTRL *msgctrl = srt_msgctrl_from_java_to_native(env, jmsgCtrl);
    int len = env->GetArrayLength(jbuf);
    char *buf = (char *) env->GetByteArrayElements(jbuf, nullptr);

    int res = srt_sendmsg2(u, buf, len, msgctrl);

    env->ReleaseByteArrayElements(jbuf, (jbyte *) buf, 0);
    if (msgctrl != nullptr) {
        free(msgctrl);
    }

    return res;
}

JNIEXPORT jbyteArray JNICALL
nativeRecv(JNIEnv *env, jobject ju, jint len) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    jbyteArray jbuf = nullptr;
    auto *buf = (char *) malloc(sizeof(char));
    int res = srt_recv(u, buf, len);

    if (res > 0) {
        jbuf = env->NewByteArray(res);
        env->SetByteArrayRegion(jbuf, 0, res, (jbyte *) buf);
    }

    if (buf != nullptr) {
        free(buf);
    }

    return jbuf;

}

JNIEXPORT jbyteArray JNICALL
nativeRecvMsg2(JNIEnv *env,
               jobject ju,
               jint len,
               jobject jmsgCtrl) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    SRT_MSGCTRL *msgctrl = srt_msgctrl_from_java_to_native(env, jmsgCtrl);
    jbyteArray jbuf = nullptr;
    auto *buf = (char *) malloc(sizeof(char));
    int res = srt_recvmsg2(u, buf, len, msgctrl);

    if (res > 0) {
        jbuf = env->NewByteArray(res);
        env->SetByteArrayRegion(jbuf, 0, res, (jbyte *) buf);
    }

    if (buf != nullptr) {
        free(buf);
    }
    if (msgctrl != nullptr) {
        free(msgctrl);
    }

    return jbuf;

}

JNIEXPORT jlong JNICALL
nativeSendFile(JNIEnv *env,
               jobject ju,
               jstring jpath,
               jlong joffset,
               jlong jsize,
               jint jblock) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    const char *path = env->GetStringUTFChars(jpath, nullptr);
    auto offset = (int64_t) joffset;
    int64_t res = srt_sendfile(u, path, &offset, (int64_t) jsize, jblock);

    env->ReleaseStringUTFChars(jpath, path);

    return (jlong) res;
}

JNIEXPORT jlong JNICALL
nativeRecvFile(JNIEnv *env,
               jobject ju,
               jstring jpath,
               jlong joffset,
               jlong jsize,
               jint jblock) {
    SRTSOCKET u = srt_socket_from_java_to_native(env, ju);
    const char *path = env->GetStringUTFChars(jpath, nullptr);
    auto offset = (int64_t) joffset;
    int64_t res = srt_recvfile(u, path, &offset, (int64_t) jsize, jblock);

    env->ReleaseStringUTFChars(jpath, path);

    return (jlong) res;
}

// Errors
JNIEXPORT jstring JNICALL
nativeGetLastErrorStr(JNIEnv *env, jobject obj) {
    return env->NewStringUTF(srt_getlasterror_str());
}

JNIEXPORT jobject JNICALL
nativeGetLastError(JNIEnv *env, jobject obj) {
    int err = srt_getlasterror(nullptr);

    return error_from_native_to_java(env, err);
}

JNIEXPORT jstring JNICALL
nativeStrError(JNIEnv *env, jobject obj) {
    int error_type = error_from_java_to_native(env, obj);
    return env->NewStringUTF(srt_strerror(error_type, 0));
}

JNIEXPORT void JNICALL
nativeClearLastError(JNIEnv *env, jobject obj) {
    srt_clearlasterror();
}

// Logging control
JNIEXPORT void JNICALL
nativeSetLogLevel(JNIEnv *env, jobject obj, jint level) {
    srt_setloglevel((int) level);
}

// Register natives API
static JNINativeMethod srtMethods[] = {
        {"nativeStartUp", "()I", (void *) &nativeStartUp},
        {"nativeCleanUp", "()I", (void *) &nativeCleanUp},
        {"nativeSetLogLevel", "(I)V", (void *) &nativeSetLogLevel}
};

static JNINativeMethod socketMethods[] = {
        {"nativeSocket",       "(Ljava/net/StandardProtocolFamily;II)I",    (void *) &nativeSocket},
        {"nativeCreateSocket", "()I",                                       (void *) &nativeCreateSocket},
        {"nativeBind",         "(L" INETSOCKETADDRESS_CLASS ";)I",          (void *) &nativeBind},
        {"nativeGetSockState", "()L" SOCKSTATUS_CLASS ";",                  (void *) &nativeGetSockState},
        {"nativeClose",        "()I",                                       (void *) &nativeClose},
        {"nativeListen",       "(I)I",                                      (void *) &nativeListen},
        {"nativeAccept",       "()L" PAIR_CLASS ";",                        (void *) &nativeAccept},
        {"nativeConnect",      "(L" INETSOCKETADDRESS_CLASS ";)I",          (void *) &nativeConnect},
        {"nativeSetSockOpt",   "(IL" SOCKOPT_CLASS ";Ljava/lang/Object;)I", (void *) &nativeSetSockOpt},
        {"nativeSend",         "([B)I",                                     (void *) &nativeSend},
        {"nativeSendMsg",      "([BIZ)I",                                   (void *) &nativeSendMsg},
        {"nativeSendMsg2",     "([BL" MSGCTRL_CLASS ";)I",                  (void *) &nativeSendMsg2},
        {"nativeRecv",         "(I)[B",                                     (void *) &nativeRecv},
        {"nativeRecvMsg2",     "(IL" MSGCTRL_CLASS ";)[B",                  (void *) &nativeRecvMsg2},
        {"nativeSendFile",     "(Ljava/lang/String;JJI)J",                  (void *) &nativeSendFile},
        {"nativeRecvFile",     "(Ljava/lang/String;JJI)J",                  (void *) &nativeRecvFile}
};

static JNINativeMethod errorMethods[] = {
        {"nativeGetLastErrorStr", "()Ljava/lang/String;", (void *) &nativeGetLastErrorStr},
        {"nativeGetLastError", "()L" ERRORTYPE_CLASS ";", (void *) &nativeGetLastError},
        {"nativeClearLastError", "()V", (void *) &nativeClearLastError}
};

static JNINativeMethod errorTypeMethods[] = {
        {"nativeStrError", "()Ljava/lang/String;", (void *) &nativeStrError}
};

static int registerNativeForClassName(JNIEnv *env, const char *className,
                                      JNINativeMethod *methods, int methodsSize) {
    jclass clazz = env->FindClass(className);
    if (clazz == nullptr) {
        LOGE(TAG, "Unable to find class '%s'", className);
        return JNI_FALSE;
    }

    if (env->RegisterNatives(clazz, methods, methodsSize) < 0) {
        LOGE(TAG, "RegisterNatives failed for '%s'", className);
        return JNI_FALSE;
    }

    return JNI_TRUE;
}

jint JNI_OnLoad(JavaVM *vm, void * /*reserved*/) {
    JNIEnv *env = nullptr;
    jint result;

    if ((result = vm->GetEnv((void **) &env, JNI_VERSION_1_4)) != JNI_OK) {
        LOGE(TAG, "GetEnv failed");
        return result;
    }

    if ((registerNativeForClassName(env, SRT_CLASS, srtMethods,
                                    sizeof(srtMethods) / sizeof(srtMethods[0])) != JNI_TRUE)) {
        LOGE(TAG, "SRT RegisterNatives failed");
        return -1;
    }

    if ((registerNativeForClassName(env, SRTSOCKET_CLASS, socketMethods,
                                    sizeof(socketMethods) / sizeof(socketMethods[0])) !=
         JNI_TRUE)) {
        LOGE(TAG, "Socket RegisterNatives failed");
        return -1;
    }

    if ((registerNativeForClassName(env, ERROR_CLASS, errorMethods,
                                    sizeof(errorMethods) / sizeof(errorMethods[0])) != JNI_TRUE)) {
        LOGE(TAG, "Error RegisterNatives failed");
        return -1;
    }

    if ((registerNativeForClassName(env, ERRORTYPE_CLASS, errorTypeMethods,
                                    sizeof(errorTypeMethods) / sizeof(errorTypeMethods[0])) != JNI_TRUE)) {
        LOGE(TAG, "ErrorType RegisterNatives failed");
        return -1;
    }

    return JNI_VERSION_1_6;
}

#ifdef __cplusplus
}
#endif