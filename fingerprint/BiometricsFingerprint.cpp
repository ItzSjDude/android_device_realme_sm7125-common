#define LOG_TAG "android.hardware.biometrics.fingerprint@2.3-service.realme_sm7125"
#define LOG_VERBOSE "android.hardware.biometrics.fingerprint@2.3-service.realme_sm7125"

#include <android-base/logging.h>
#include <android-base/properties.h>
#include <hardware/hardware.h>
#include <hardware/fingerprint.h>
#include "BiometricsFingerprint.h"
#include <fstream>
#include <cmath>
#include <inttypes.h>
#include <unistd.h>
#include <thread>

#define FP_PRESS_PATH "/sys/kernel/oppo_display/notify_fppress"
#define DIMLAYER_PATH "/sys/kernel/oppo_display/dimlayer_hbm"
#define POWER_STATUS_PATH "/sys/kernel/oppo_display/power_status"
#define NOTIFY_BLANK_PATH "/sys/kernel/oppo_display/notify_panel_blank"
#define PRJNAME_PATH "/proc/oplusVersion/prjName"

namespace android {
namespace hardware {
namespace biometrics {
namespace fingerprint {
namespace V2_3 {
namespace implementation {

template <typename T>
static void set(const std::string& path, const T& value) {
    std::ofstream file(path);
    if (!file.is_open()) {
        LOG(ERROR) << "Failed to open " << path;
        return;
    }
    file << value;
    LOG(INFO) << "wrote path: " << path << ", value: " << value;
}

template <typename T>
static T get(const std::string& path, const T& def) {
    std::ifstream file(path);
    T result;
    file >> result;
    if (file.fail()) return def;
    LOG(INFO) << "read path: " << path << ", value: " << result;
    return result;
}

static std::string get(const std::string& path, const std::string& def) {
    std::ifstream file(path);
    std::string result;
    file >> result;
    if (file.fail()) return def;
    LOG(INFO) << "read path: " << path << ", value: " << result;
    return result;
}

BiometricsFingerprint::BiometricsFingerprint() {
    mOplusBiometricsFingerprint =
        vendor::oplus::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint::getService();
}

/** 
 * UDFPS: Check for Realme 6 Pro (RMX2061 / 206B1)
 */
Return<bool> BiometricsFingerprint::isUdfps(uint32_t) {
    std::string prjName = get(PRJNAME_PATH, "");
    if (prjName == "206B1" || prjName == "206B2") return true;

    std::string device = android::base::GetProperty("ro.product.device", "");
    return (device.find("rmx2061") != std::string::npos);
}

/**
 * Detects doze state
 */
Return<bool> BiometricsFingerprint::isDozeMode() {
    int powerState = get(POWER_STATUS_PATH, 0);
    return (powerState == 1 || powerState == 3);
}

/**
 * Finger down = enable FP press and HBM overlay
 */
Return<void> BiometricsFingerprint::onFingerDown(uint32_t, uint32_t, float, float) {
    if (!isUdfps(0)) return Void();

    LOG(INFO) << "UDFPS: Finger down";
    set(NOTIFY_BLANK_PATH, 1);
    set(FP_PRESS_PATH, 1);

    if (isDozeMode()) {
        set(DIMLAYER_PATH, 1);
    } else {
        // Delay HBM activation slightly in awake mode
        std::thread([]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            set(DIMLAYER_PATH, 1);
        }).detach();
    }
    return Void();
}

/**
 * Finger up = disable FP press and HBM overlay
 */
Return<void> BiometricsFingerprint::onFingerUp() {
    if (!isUdfps(0)) return Void();

    LOG(INFO) << "UDFPS: Finger up";
    set(FP_PRESS_PATH, 0);
    set(DIMLAYER_PATH, 0);
    set(NOTIFY_BLANK_PATH, 0);
    return Void();
}

/**
 * Stub overlay functions (AOSP 15 doesn’t use these anymore)
 */
Return<void> BiometricsFingerprint::onShowUdfpsOverlay() { return Void(); }
Return<void> BiometricsFingerprint::onHideUdfpsOverlay() { return Void(); }

/**
 * The rest of your existing functions (setNotify, preEnroll, etc.)
 * remain unchanged below...
 */

// Keep your existing RequestStatus mapping and enroll/authenticate methods intact.

}  // namespace implementation
}  // namespace V2_3
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace hardware
}  // namespace android
