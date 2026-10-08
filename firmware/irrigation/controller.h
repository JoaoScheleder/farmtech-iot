#pragma once

#include <cmath>
#include <cstdint>

namespace farmtech {

struct WeatherForecast {
  bool present = false;
  uint32_t fetchedAt = 0;
  uint32_t windowStart = 0;
  uint32_t windowEnd = 0;
  float precipitationMm = 0;
  float probabilityPct = 0;
  uint32_t ttlSeconds = 0;
  uint32_t receivedMs = 0;

  bool fresh(uint32_t nowMs) const {
    return present && nowMs - receivedMs < ttlSeconds * 1000u;
  }

  bool holdsWatering(uint32_t nowMs) const {
    return fresh(nowMs) && precipitationMm >= 2.0f && probabilityPct >= 60.0f;
  }
};

struct IrrigationController {
  bool sensorValid = false;
  bool demand = false;
  bool critical = false;
  bool timedOut = false;
  bool pumpOn = false;
  uint32_t sampledMs = 0;
  uint32_t pumpStartedMs = 0;
  const char* reason = "SENSOR_FAULT";

  void sample(float humidity, uint32_t nowMs) {
    sensorValid = std::isfinite(humidity) && humidity >= 0 && humidity <= 100;
    if (!sensorValid) {
      demand = false;
      critical = false;
      return;
    }
    sampledMs = nowMs;
    if (humidity <= 40) demand = true;
    else if (humidity >= 60) demand = false;
    if (humidity <= 25) critical = true;
    else if (humidity >= 40) critical = false;
  }

  void evaluate(uint32_t nowMs, const WeatherForecast& weather) {
    if (pumpOn && nowMs - pumpStartedMs >= 60000u) timedOut = true;
    bool nextPump = false;
    if (!sensorValid || nowMs - sampledMs > 7500u) {
      sensorValid = false;
      demand = false;
      critical = false;
      reason = "SENSOR_FAULT";
    } else if (timedOut) {
      reason = "TIMEOUT";
    } else if (!demand) {
      reason = "MOISTURE_IDLE";
    } else if (critical) {
      nextPump = true;
      reason = "CRITICAL_DRY";
    } else if (weather.holdsWatering(nowMs)) {
      reason = "RAIN_HOLD";
    } else {
      nextPump = true;
      reason = "MOISTURE_DEMAND";
    }
    if (nextPump && !pumpOn) pumpStartedMs = nowMs;
    pumpOn = nextPump;
  }
};

}