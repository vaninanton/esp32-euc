#ifndef SAMPLE_MANAGER_H
#define SAMPLE_MANAGER_H
#include <Arduino.h>

class EUC {
 public:
  static EUC& getInstance();
  void debug();
  void setMVoltage(int val);
  void setChargeState(int val);

 private:
  EUC();

  EUC(const EUC&) = delete;
  EUC& operator=(const EUC&) = delete;

  int mVoltage;
  bool chargeState;
};

#endif
