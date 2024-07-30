#include "EUC.h"

static const char* LOG_TAG = "EUC";

void eucClass::debug() {
  ESP_LOGD(LOG_TAG, "Power: %d.%.2dV, (%d.%.2d%%)\n", (int)(busVoltage / 100), (int)(busVoltage % 100), (int)(batteryPercentage / 100), (int)(batteryPercentage % 100));
  // ESP_LOGD(LOG_TAG, "busCurrent: %d\n", busCurrent);
  ESP_LOGD(LOG_TAG, "speed: %d\n", speed);
  // ESP_LOGD(LOG_TAG, "torque: %d\n", torque);
  // ESP_LOGD(LOG_TAG, "outputRate: %d\n", outputRate);
  // ESP_LOGD(LOG_TAG, "batteryOutputPower: %d\n", batteryOutputPower);
  // ESP_LOGD(LOG_TAG, "motorOutputPower: %d\n", motorOutputPower);
  // ESP_LOGD(LOG_TAG, "reserve14: %d\n", reserve14);
  // ESP_LOGD(LOG_TAG, "pitchAngle: %d\n", pitchAngle);
  // ESP_LOGD(LOG_TAG, "rollAngle: %d\n", rollAngle);
  // ESP_LOGD(LOG_TAG, "aimPitchAngle: %d\n", aimPitchAngle);
  // ESP_LOGD(LOG_TAG, "speedingBrakingAngle: %d\n", speedingBrakingAngle);
  // ESP_LOGD(LOG_TAG, "mileage: %d\n", mileage);
  // ESP_LOGD(LOG_TAG, "reserve26: %d\n", reserve26);
  // ESP_LOGD(LOG_TAG, "batteryPercentage: %d\n", batteryPercentage);
  // ESP_LOGD(LOG_TAG, "batteryPercentageForRide: %d\n", batteryPercentageForRide);
  // ESP_LOGD(LOG_TAG, "estimatedTotalMileage: %d\n", estimatedTotalMileage);
  // ESP_LOGD(LOG_TAG, "realtimeSpeedLimit: %d\n", realtimeSpeedLimit);
  // ESP_LOGD(LOG_TAG, "realtimeCurrentLimit: %d\n", realtimeCurrentLimit);
  // ESP_LOGD(LOG_TAG, "reserve38: %d\n", reserve38);
  // ESP_LOGD(LOG_TAG, "reserve40: %d\n", reserve40);
  // ESP_LOGD(LOG_TAG, "mosTemperature: %d\n", mosTemperature);
  // ESP_LOGD(LOG_TAG, "motorTemperature: %d\n", motorTemperature);
  // ESP_LOGD(LOG_TAG, "batteryTemperature: %d\n", batteryTemperature);
  // ESP_LOGD(LOG_TAG, "boardTemperature: %d\n", boardTemperature);
  // ESP_LOGD(LOG_TAG, "cpuTemperature: %d\n", cpuTemperature);
  // ESP_LOGD(LOG_TAG, "imuTemperature: %d\n", imuTemperature);
  // ESP_LOGD(LOG_TAG, "lampTemperature: %d\n", lampTemperature);
  // ESP_LOGD(LOG_TAG, "envBrightness: %d\n", envBrightness);
  // ESP_LOGD(LOG_TAG, "lampBrightness: %d\n", lampBrightness);
  // ESP_LOGD(LOG_TAG, "reserve51: %d\n", reserve51);
  // ESP_LOGD(LOG_TAG, "reserve52: %d\n", reserve52);
  // ESP_LOGD(LOG_TAG, "reserve53: %d\n", reserve53);
  // ESP_LOGD(LOG_TAG, "reserve54: %d\n", reserve54);
  // ESP_LOGD(LOG_TAG, "reserve55: %d\n", reserve55);
  // ESP_LOGD(LOG_TAG, "HMICRunMode: %d\n", HMICRunMode); // lock, drive, shutdown, idle
  // ESP_LOGD(LOG_TAG, "MCRunMode: %d\n", MCRunMode);
  // ESP_LOGD(LOG_TAG, "motorState: %d\n", motorState);
  // ESP_LOGD(LOG_TAG, "chargeState: %d\n", chargeState);
  // ESP_LOGD(LOG_TAG, "backupBatteryState: %d\n", backupBatteryState);
  // ESP_LOGD(LOG_TAG, "lampState: %d\n", lampState);
  // ESP_LOGD(LOG_TAG, "decorativeLightState: %d\n", decorativeLightState);
  // ESP_LOGD(LOG_TAG, "liftedState: %d\n", liftedState);
  ESP_LOGD(LOG_TAG, "tailLightState: %d\n", tailLightState); // 3bits; 0 - CLOSED, 1 - LOWLIGHT, 2 - HIGHLIGHT, 3 - BLINKING
  // ESP_LOGD(LOG_TAG, "fanState: %d\n", fanState);
  ESP_LOGD(LOG_TAG, "brakeState: %d\n", brakeState);
  ESP_LOGD(LOG_TAG, "slowDownState: %d\n", slowDownState);
  // ESP_LOGD(LOG_TAG, "DFUState: %d\n", DFUState);
}

eucClass EUC = eucClass();
