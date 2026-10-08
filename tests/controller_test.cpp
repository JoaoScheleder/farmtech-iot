#include <cassert>
#include <cstring>
#include <iostream>
#include <limits>
#include "controller.h"

int main() {
  using farmtech::IrrigationController;
  using farmtech::WeatherForecast;
  WeatherForecast weather;
  IrrigationController control;
  control.evaluate(0, weather);
  assert(!control.pumpOn);
  control.sample(50, 1);
  control.evaluate(1, weather);
  assert(!control.pumpOn);
  for (const float humidity : {40.0f, 50.0f, 59.9f}) {
    control.sample(humidity, 2);
    control.evaluate(2, weather);
    assert(control.pumpOn);
  }
  control.sample(60, 3);
  control.evaluate(3, weather);
  assert(!control.pumpOn);
  control.sample(50, 4);
  control.evaluate(4, weather);
  assert(!control.pumpOn);

  weather = {true, 1, 3600, 25200, 2, 60, 10, 100};
  control.sample(35, 100);
  control.evaluate(100, weather);
  assert(control.demand && !control.pumpOn);
  assert(std::strcmp(control.reason, "RAIN_HOLD") == 0);
  for (const float humidity : {25.0f, 26.0f, 39.9f}) {
    control.sample(humidity, 101);
    control.evaluate(101, weather);
    assert(control.critical && control.pumpOn);
  }
  control.sample(40, 102);
  control.evaluate(102, weather);
  assert(!control.critical && control.demand && !control.pumpOn);
  weather.probabilityPct = 59.9f;
  control.evaluate(103, weather);
  assert(control.pumpOn);
  weather.probabilityPct = 60;
  weather.precipitationMm = 1.9f;
  control.evaluate(104, weather);
  assert(control.pumpOn);
  weather.precipitationMm = 2;
  control.sample(50, 10099);
  control.evaluate(10099, weather);
  assert(!control.pumpOn && weather.fresh(10099));
  control.evaluate(10100, weather);
  assert(control.pumpOn && !weather.fresh(10100));
  weather.present = false;
  control.evaluate(10101, weather);
  assert(control.pumpOn);

  for (const float invalid : {-1.0f, 101.0f, std::numeric_limits<float>::quiet_NaN(),
                              std::numeric_limits<float>::infinity()}) {
    control.sample(invalid, 10102);
    control.evaluate(10102, weather);
    assert(!control.pumpOn && !control.demand && !control.critical);
    control.sample(50, 10103);
    control.evaluate(10103, weather);
    assert(!control.pumpOn);
  }
  control.sample(25, 20000);
  control.evaluate(27500, weather);
  assert(control.pumpOn);
  control.evaluate(27501, weather);
  assert(!control.pumpOn && !control.demand && !control.critical);

  control = {};
  for (uint32_t nowMs = 0; nowMs <= 60000; nowMs += 2500) {
    control.sample(25, nowMs);
    control.evaluate(nowMs, weather);
    assert(control.pumpOn == (nowMs < 60000));
  }
  assert(control.timedOut && std::strcmp(control.reason, "TIMEOUT") == 0);
  control.sample(25, 62500);
  control.evaluate(62500, weather);
  assert(!control.pumpOn);
  control.timedOut = false;
  control.evaluate(62500, weather);
  assert(control.pumpOn);

  const uint32_t nearWrap = UINT32_MAX - 2000;
  weather = {true, 1, 3600, 25200, 2, 60, 5, nearWrap};
  assert(weather.fresh(nearWrap + 4999u));
  assert(!weather.fresh(nearWrap + 5000u));
  control = {};
  control.sample(35, nearWrap);
  control.evaluate(nearWrap, weather);
  assert(!control.pumpOn);
  control.evaluate(nearWrap + 5000u, weather);
  assert(control.pumpOn);
  control.evaluate(nearWrap + 7501u, weather);
  assert(!control.pumpOn);
  std::cout << "OK: histerese, chuva, secura critica, expiracao, falhas, timeout e rollover.\n";
}