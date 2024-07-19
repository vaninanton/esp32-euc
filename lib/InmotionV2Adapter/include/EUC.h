#pragma once
#include <Arduino.h>
#include <NimBLEDevice.h>

class eucClass {
 public:
  void debug();

  NimBLEAdvertisedDevice* bleDevice;
  int16_t busVoltage;                 // 0
  int16_t busCurrent;                 // 2
  int16_t speed;                      // 4
  int16_t torque;                     // 6
  int16_t outputRate;                 // 8
  int16_t batteryOutputPower;         // 10
  int16_t motorOutputPower;           // 12
  uint16_t reserve14;                 // 14
  int16_t pitchAngle;                 // 16
  int16_t rollAngle;                  // 18
  int16_t aimPitchAngle;              // 20
  int16_t speedingBrakingAngle;       // 22
  uint16_t mileage;                   // 24
  uint16_t reserve26;                 // 26
  uint16_t batteryPercentage;         // 28
  uint16_t batteryPercentageForRide;  // 30
  uint16_t estimatedTotalMileage;     // 32
  int16_t realtimeSpeedLimit;         // 34
  int16_t realtimeCurrentLimit;       // 36
  uint16_t reserve38;                 // 38
  uint16_t reserve40;                 // 40
  int8_t mosTemperature;              // 42
  int8_t motorTemperature;            // 43
  int8_t batteryTemperature;          // 44
  int8_t boardTemperature;            // 45
  int8_t cpuTemperature;              // 46
  int8_t imuTemperature;              // 47
  int8_t lampTemperature;             // 48
  uint8_t envBrightness;              // 49
  uint8_t lampBrightness;             // 50
  uint8_t reserve51;                  // 51
  uint8_t reserve52;                  // 52
  uint8_t reserve53;                  // 53
  uint8_t reserve54;                  // 54
  uint8_t reserve55;                  // 55

  int HMICRunMode;
  int MCRunMode;
  int motorState;
  int chargeState;
  int backupBatteryState;
  int lampState;
  int decorativeLightState;
  int liftedState;
  int tailLightState;
  int fanState;
  int brakeState;
  int slowDownState;
  int DFUState;
};

extern eucClass EUC;
