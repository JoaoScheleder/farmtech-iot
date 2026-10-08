#pragma once

#include <cmath>
#include <limits>

namespace farmtech {

constexpr int LDR_ADC_LOW = 32;
constexpr int LDR_ADC_HIGH = 4063;
constexpr float TOMATO_PH_MIN = 6.0f;
constexpr float TOMATO_PH_MAX = 6.5f;

struct CropAlerts {
  bool nitrogenInsufficient;
  bool phosphorusInsufficient;
  bool potassiumInsufficient;
  bool phOutOfRange;
  bool phInvalid;
};

inline float simulatedPhFromAdc(int rawAdc) {
  if (rawAdc < 0 || rawAdc > 4095) {
    return std::numeric_limits<float>::quiet_NaN();
  }
  if (rawAdc <= LDR_ADC_LOW) return 0.0f;
  if (rawAdc >= LDR_ADC_HIGH) return 14.0f;
  return (rawAdc - LDR_ADC_LOW) * 14.0f / (LDR_ADC_HIGH - LDR_ADC_LOW);
}

inline CropAlerts evaluateCropAlerts(bool nitrogenOk, bool phosphorusOk,
                                    bool potassiumOk, float ph) {
  const bool phValid = std::isfinite(ph) && ph >= 0.0f && ph <= 14.0f;
  return { !nitrogenOk, !phosphorusOk, !potassiumOk,
           phValid && (ph < TOMATO_PH_MIN || ph > TOMATO_PH_MAX), !phValid };
}

}