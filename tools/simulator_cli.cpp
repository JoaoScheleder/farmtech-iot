#include <iostream>
#include <limits>
#include "alerts.h"
#include "controller.h"

int main() {
  farmtech::IrrigationController controller;
  farmtech::WeatherForecast weather;
  uint32_t now, ttl;
  float humidity, temperature, rain, probability;
  int adc, nitrogen, phosphorus, potassium, automatic, manual, reset, sample, present;
  bool relay = false;
  bool manualActive = false;
  uint32_t manualStarted = 0;
  int previousMode = -1;
  while (std::cin >> now >> humidity >> temperature >> adc >> nitrogen >> phosphorus
         >> potassium >> automatic >> manual >> reset >> sample >> present >> rain
         >> probability >> ttl) {
    weather.present = present;
    weather.precipitationMm = rain;
    weather.probabilityPct = probability;
    weather.ttlSeconds = ttl;
    if (automatic != previousMode) {
      controller = {};
      relay = false;
      manualActive = false;
      previousMode = automatic;
    }
    if (reset) controller.timedOut = false;
    if (automatic) {
      if (sample) controller.sample(humidity < 0 ? std::numeric_limits<float>::quiet_NaN() : humidity, now);
      controller.evaluate(now, weather);
      relay = controller.pumpOn;
    } else {
      if (manualActive && now - manualStarted >= 5000u) manualActive = false;
      if (manual == 0) manualActive = false;
      if (manual == 1 && !manualActive) {
        manualActive = true;
        manualStarted = now;
      }
      relay = manualActive;
    }
    const float ph = farmtech::simulatedPhFromAdc(adc);
    const auto alerts = farmtech::evaluateCropAlerts(nitrogen, phosphorus, potassium, ph);
    std::cout << "{\"uptime_ms\":" << now << ",\"pump_on\":" << (relay ? "true" : "false")
              << ",\"contact\":\"" << (relay ? "FECHADO" : "ABERTO")
              << "\",\"reason\":\"" << (automatic ? controller.reason : "TESTE_MANUAL")
              << "\",\"weather_status\":\"" << (weather.fresh(now) ? "VALIDO" : "DESCONHECIDO")
              << "\",\"sensor_valid\":" << (humidity >= 0 && humidity <= 100 && (!automatic || controller.sensorValid) ? "true" : "false")
              << ",\"ph\":";
    if (std::isfinite(ph)) std::cout << ph;
    else std::cout << "null";
    std::cout << ",\"alerts\":[";
    bool separator = false;
    const bool active[] = {alerts.nitrogenInsufficient, alerts.phosphorusInsufficient,
      alerts.potassiumInsufficient, alerts.phOutOfRange, alerts.phInvalid};
    const char* names[] = {"N_INSUFICIENTE", "P_INSUFICIENTE", "K_INSUFICIENTE", "PH_FORA_FAIXA", "PH_INVALIDO"};
    for (unsigned index = 0; index < 5; ++index) {
      if (!active[index]) continue;
      if (separator) std::cout << ',';
      std::cout << '"' << names[index] << '"';
      separator = true;
    }
    std::cout << "]}" << std::endl;
  }
  return std::cin.eof() ? 0 : 1;
}