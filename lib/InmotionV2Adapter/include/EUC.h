#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>

class eucClass {
 public:
  void debug();

  NimBLEAdvertisedDevice* bleDevice;
  int16_t busVoltage;
  int16_t busCurrent;
  int16_t speed;
  int16_t torque;
  int16_t outputRate;
  int16_t batteryOutputPower;
  int16_t motorOutputPower;
  uint16_t reserve14;
  int16_t pitchAngle;
  int16_t rollAngle;
  int16_t aimPitchAngle;
  int16_t speedingBrakingAngle;
  uint16_t mileage;
  uint16_t reserve26;
  uint16_t batteryPercentage;
  uint16_t batteryPercentageForRide;
  uint16_t estimatedTotalMileage;
  int16_t realtimeSpeedLimit;
  int16_t realtimeCurrentLimit;
  uint16_t reserve38;
  uint16_t reserve40;
  int8_t mosTemperature;
  int8_t motorTemperature;
  int8_t batteryTemperature;
  int8_t boardTemperature;
  int8_t cpuTemperature;
  int8_t imuTemperature;
  int8_t lampTemperature;
  uint8_t envBrightness;
  uint8_t lampBrightness;
  uint8_t reserve51;
  uint8_t reserve52;
  uint8_t reserve53;
  uint8_t reserve54;
  uint8_t reserve55;

  uint8_t HMICRunMode;
  bool MCRunMode;
  bool motorState;
  bool chargeState;
  bool backupBatteryState;
  bool lampState;
  bool decorativeLightState;
  bool liftedState;
  uint8_t tailLightState;
  bool fanState;
  bool brakeState;
  bool slowDownState;
  bool DFUState;
};

extern eucClass EUC;
