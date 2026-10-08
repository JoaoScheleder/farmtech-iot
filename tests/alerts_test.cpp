#include "alerts.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

int main() {
  for (int combination = 0; combination < 8; ++combination) {
    const bool nitrogenOk = (combination & 4) != 0;
    const bool phosphorusOk = (combination & 2) != 0;
    const bool potassiumOk = (combination & 1) != 0;
    const auto alerts = farmtech::evaluateCropAlerts(nitrogenOk, phosphorusOk, potassiumOk, 6.2f);
    assert(alerts.nitrogenInsufficient == !nitrogenOk);
    assert(alerts.phosphorusInsufficient == !phosphorusOk);
    assert(alerts.potassiumInsufficient == !potassiumOk);
    assert(!alerts.phOutOfRange && !alerts.phInvalid);
  }

  for (const float ph : { 0.0f, 5.9f, 6.6f, 14.0f }) {
    const auto alerts = farmtech::evaluateCropAlerts(true, true, true, ph);
    assert(alerts.phOutOfRange && !alerts.phInvalid);
  }
  for (const float ph : { 6.0f, 6.2f, 6.5f }) {
    const auto alerts = farmtech::evaluateCropAlerts(true, true, true, ph);
    assert(!alerts.phOutOfRange && !alerts.phInvalid);
  }
  for (const float ph : { -1.0f, 15.0f, std::numeric_limits<float>::quiet_NaN(),
                          std::numeric_limits<float>::infinity() }) {
    const auto alerts = farmtech::evaluateCropAlerts(true, true, true, ph);
    assert(alerts.phInvalid && !alerts.phOutOfRange);
  }

  assert(std::isnan(farmtech::simulatedPhFromAdc(-1)));
  assert(std::isnan(farmtech::simulatedPhFromAdc(4096)));
  assert(farmtech::simulatedPhFromAdc(0) == 0.0f);
  assert(farmtech::simulatedPhFromAdc(32) == 0.0f);
  assert(farmtech::simulatedPhFromAdc(4063) == 14.0f);
  assert(farmtech::simulatedPhFromAdc(4095) == 14.0f);
  assert(std::fabs(farmtech::simulatedPhFromAdc(1832) - 6.25155f) < 0.0001f);

  float previousPh = 0.0f;
  for (int rawAdc = 0; rawAdc <= 4095; ++rawAdc) {
    const float ph = farmtech::simulatedPhFromAdc(rawAdc);
    assert(ph >= previousPh && ph <= 14.0f);
    previousPh = ph;
  }
  std::cout << "OK: NPK, limites inclusivos de pH, entradas invalidas e conversao ADC.\n";
}