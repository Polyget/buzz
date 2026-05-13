#include <jni.h>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <cmath>
#include <vector>

// Define proprietary ioctl for FM, assuming Qualcomm style
#define FM_SET_FREQ _IOW('F', 1, int)  // Set frequency in kHz
#define FM_GET_RAW_DATA _IOR('F', 2, char*)  // Get raw I/Q data

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_buzz_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_buzz_MainActivity_tuneToFrequency(
        JNIEnv* env,
        jobject,
        jlong freqHz) {
    // Open the FM device, assuming /dev/radio0 for Qualcomm WCN chip
    int fd = open("/dev/radio0", O_RDWR);
    if (fd < 0) {
        // Log error, but since no logging in C++, return
        return;
    }
    // Try standard V4L2 first
    struct v4l2_frequency vf;
    vf.tuner = 0;
    vf.type = V4L2_TUNER_RADIO;
    vf.frequency = freqHz / 62500;  // V4L2 frequency in units of 62.5 kHz
    if (ioctl(fd, VIDIOC_S_FREQUENCY, &vf) < 0) {
        // If fails, try proprietary ioctl to override bounds
        int freqKHz = freqHz / 1000;
        ioctl(fd, FM_SET_FREQ, &freqKHz);  // Force set to 4625 kHz, bypassing factory limits
    }
    close(fd);
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_buzz_MainActivity_readRawData(
        JNIEnv* env,
        jobject) {
    int fd = open("/dev/radio0", O_RDONLY);
    if (fd < 0) return nullptr;
    // Assume driver provides raw I/Q samples
    char buffer[4096];  // Buffer for raw data
    ssize_t len = read(fd, buffer, sizeof(buffer));
    close(fd);
    if (len <= 0) return nullptr;
    jbyteArray arr = env->NewByteArray(len);
    env->SetByteArrayRegion(arr, 0, len, (jbyte*)buffer);
    return arr;
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_com_example_buzz_MainActivity_demodulateAM(
        JNIEnv* env,
        jobject,
        jbyteArray rawData) {
    jsize len = env->GetArrayLength(rawData);
    jbyte* data = env->GetByteArrayElements(rawData, nullptr);
    // Assume data is interleaved I/Q as signed bytes (-128 to 127)
    std::vector<jbyte> demod(len / 2);
    for (jsize i = 0; i < len; i += 2) {
        float I = (float)data[i];
        float Q = (float)data[i + 1];
        // AM envelope detection
        float envelope = std::sqrt(I * I + Q * Q);
        demod[i / 2] = (jbyte)(envelope / std::sqrt(2.0f * 127 * 127) * 127);  // Normalize
    }
    env->ReleaseByteArrayElements(rawData, data, JNI_ABORT);
    jbyteArray arr = env->NewByteArray(demod.size());
    env->SetByteArrayRegion(arr, 0, demod.size(), demod.data());
    return arr;
}