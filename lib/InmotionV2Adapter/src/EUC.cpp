#include "EUC.h"

static const char* LOG_TAG = "EUC";

void eucClass::debug() {
  ESP_LOGD(LOG_TAG, "Power: %d.%.2dV, (%d.%.2d%%)", (int)(busVoltage / 100), (int)(busVoltage % 100), (int)(batteryPercentage / 100), (int)(batteryPercentage % 100));
  // ESP_LOGD(LOG_TAG, "busCurrent: %d", busCurrent);
  ESP_LOGD(LOG_TAG, "speed: %d", speed);
  // ESP_LOGD(LOG_TAG, "torque: %d", torque);
  // ESP_LOGD(LOG_TAG, "outputRate: %d", outputRate);
  // ESP_LOGD(LOG_TAG, "batteryOutputPower: %d", batteryOutputPower);
  // ESP_LOGD(LOG_TAG, "motorOutputPower: %d", motorOutputPower);
  // ESP_LOGD(LOG_TAG, "reserve14: %d", reserve14);
  // ESP_LOGD(LOG_TAG, "pitchAngle: %d", pitchAngle);
  // ESP_LOGD(LOG_TAG, "rollAngle: %d", rollAngle);
  // ESP_LOGD(LOG_TAG, "aimPitchAngle: %d", aimPitchAngle);
  // ESP_LOGD(LOG_TAG, "speedingBrakingAngle: %d", speedingBrakingAngle);
  // ESP_LOGD(LOG_TAG, "mileage: %d", mileage);
  // ESP_LOGD(LOG_TAG, "reserve26: %d", reserve26);
  // ESP_LOGD(LOG_TAG, "batteryPercentage: %d", batteryPercentage);
  // ESP_LOGD(LOG_TAG, "batteryPercentageForRide: %d", batteryPercentageForRide);
  // ESP_LOGD(LOG_TAG, "estimatedTotalMileage: %d", estimatedTotalMileage);
  // ESP_LOGD(LOG_TAG, "realtimeSpeedLimit: %d", realtimeSpeedLimit);
  // ESP_LOGD(LOG_TAG, "realtimeCurrentLimit: %d", realtimeCurrentLimit);
  // ESP_LOGD(LOG_TAG, "reserve38: %d", reserve38);
  // ESP_LOGD(LOG_TAG, "reserve40: %d", reserve40);
  // ESP_LOGD(LOG_TAG, "mosTemperature: %d", mosTemperature);
  // ESP_LOGD(LOG_TAG, "motorTemperature: %d", motorTemperature);
  // ESP_LOGD(LOG_TAG, "batteryTemperature: %d", batteryTemperature);
  // ESP_LOGD(LOG_TAG, "boardTemperature: %d", boardTemperature);
  // ESP_LOGD(LOG_TAG, "cpuTemperature: %d", cpuTemperature);
  // ESP_LOGD(LOG_TAG, "imuTemperature: %d", imuTemperature);
  // ESP_LOGD(LOG_TAG, "lampTemperature: %d", lampTemperature);
  // ESP_LOGD(LOG_TAG, "envBrightness: %d", envBrightness);
  // ESP_LOGD(LOG_TAG, "lampBrightness: %d", lampBrightness);
  // ESP_LOGD(LOG_TAG, "reserve51: %d", reserve51);
  // ESP_LOGD(LOG_TAG, "reserve52: %d", reserve52);
  // ESP_LOGD(LOG_TAG, "reserve53: %d", reserve53);
  // ESP_LOGD(LOG_TAG, "reserve54: %d", reserve54);
  // ESP_LOGD(LOG_TAG, "reserve55: %d", reserve55);
  // ESP_LOGD(LOG_TAG, "HMICRunMode: %d", HMICRunMode); // lock, drive, shutdown, idle
  // ESP_LOGD(LOG_TAG, "MCRunMode: %d", MCRunMode);
  // ESP_LOGD(LOG_TAG, "motorState: %d", motorState);
  // ESP_LOGD(LOG_TAG, "chargeState: %d", chargeState);
  // ESP_LOGD(LOG_TAG, "backupBatteryState: %d", backupBatteryState);
  ESP_LOGD(LOG_TAG, "lampState: %d", lampState);
  ESP_LOGD(LOG_TAG, "decorativeLightState: %d", decorativeLightState);
  ESP_LOGD(LOG_TAG, "liftedState: %d", liftedState);
  ESP_LOGD(LOG_TAG, "tailLightState: %d", tailLightState); // 3bits; 0 - CLOSED, 1 - LOWLIGHT, 2 - HIGHLIGHT, 3 - BLINKING
  // ESP_LOGD(LOG_TAG, "fanState: %d", fanState);
  ESP_LOGD(LOG_TAG, "brakeState: %d", brakeState);
  ESP_LOGD(LOG_TAG, "slowDownState: %d", slowDownState);
  // ESP_LOGD(LOG_TAG, "DFUState: %d", DFUState);
}

eucClass EUC = eucClass();
