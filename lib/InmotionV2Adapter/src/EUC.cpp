#include "EUC.h"

EUC::EUC() {}

EUC& EUC::getInstance() {
  static EUC instance;
  return instance;
}

void EUC::debug() {
  Serial.printf("[DEBUG] mVoltage = %dmV\n", mVoltage);
  Serial.printf("[DEBUG] chargeState = %d\n", chargeState);
  Serial.printf("[DEBUG] speed = %d\n", speed);
}

void EUC::setMVoltage(int val) {
  mVoltage = val;
}

void EUC::setSpeed(int val) {
  speed = val;
}

int EUC::getSpeed() {
  return speed;
}

void EUC::setChargeState(int val) {
  chargeState = (val == 1) ? true : false;
}
