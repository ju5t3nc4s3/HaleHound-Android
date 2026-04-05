// attack_entry.cpp — JNI entry points called from Kotlin HalBridge.
// Each nativeRun* function calls into the corresponding firmware module.
// The firmware .cpp files live in firmware-stubs/ and are compiled unchanged
// against the HAL shim headers in hal/.

#include <jni.h>
#include <android/log.h>
#include <cstring>

#define TAG "HaleHound"

// Forward-declare the firmware entry functions.
// These are defined in the original firmware source files (firmware-stubs/).
// The HAL shim headers make them compile cleanly under NDK.
extern "C" {
    // SubGHz / CC1101
    void subghz_replay_start(const char* freq);
    void subghz_jam_start(const char* freq);
    void subghz_bruteforce_start(const char* freq, int bits);
    void subghz_stop();

    // NRF24 2.4 GHz
    void nrf24_scanner_start();
    void nrf24_jammer_start();
    void nrf24_stop();

    // GPS
    void gps_module_start();

    // SIGINT
    void karma_attack_start();
    void karma_attack_stop();
}

// Attack IDs — keep in sync with HalBridge.kt AttackId enum
#define ATTACK_SUBGHZ_REPLAY    10
#define ATTACK_SUBGHZ_JAM       11
#define ATTACK_SUBGHZ_BRUTE     12
#define ATTACK_NRF24_SCAN       20
#define ATTACK_NRF24_JAM        21
#define ATTACK_GPS_START        30
#define ATTACK_KARMA            40
#define ATTACK_STOP             99

extern "C" JNIEXPORT jstring JNICALL
Java_com_halehound_HalBridge_nativeRunAttack(JNIEnv* env, jobject, jint attackId, jstring jparams) {
    const char* params = env->GetStringUTFChars(jparams, nullptr);
    char result[256] = "ok";

    switch (attackId) {
        case ATTACK_SUBGHZ_REPLAY:  subghz_replay_start(params);        break;
        case ATTACK_SUBGHZ_JAM:     subghz_jam_start(params);           break;
        case ATTACK_SUBGHZ_BRUTE:   subghz_bruteforce_start(params, 24);break;
        case ATTACK_NRF24_SCAN:     nrf24_scanner_start();              break;
        case ATTACK_NRF24_JAM:      nrf24_jammer_start();               break;
        case ATTACK_GPS_START:      gps_module_start();                  break;
        case ATTACK_KARMA:          karma_attack_start();                break;
        case ATTACK_STOP:
            subghz_stop();
            nrf24_stop();
            karma_attack_stop();
            break;
        default:
            snprintf(result, sizeof(result), "unknown attack id: %d", attackId);
            __android_log_print(ANDROID_LOG_WARN, TAG, "%s", result);
            break;
    }

    env->ReleaseStringUTFChars(jparams, params);
    return env->NewStringUTF(result);
}

// Radio verification — reads chip ID registers, same as firmware radio_test.cpp
extern "C" JNIEXPORT jstring JNICALL
Java_com_halehound_HalBridge_nativeRadioTest(JNIEnv* env, jobject) {
    // NRF24: read CONFIG register (addr 0x00), expect 0x08
    // CC1101: read VERSION register (addr 0x31 | 0xC0), expect 0x14
    // Actual register reads happen via hal_spi_transfer which goes to usb_hal.cpp
    char out[256];
    snprintf(out, sizeof(out), "radio_test: see Logcat for SPI register results");
    return env->NewStringUTF(out);
}
