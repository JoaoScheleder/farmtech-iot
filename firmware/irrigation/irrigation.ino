#include <Arduino.h>
#include <DHTesp.h>
#include <math.h>
#include "alerts.h"
#include "weather_command.h"

constexpr uint8_t NITROGEN_PIN = 25;
constexpr uint8_t PHOSPHORUS_PIN = 26;
constexpr uint8_t POTASSIUM_PIN = 27;
constexpr uint8_t PH_PIN = 34;
constexpr uint8_t DHT_PIN = 16;
constexpr uint8_t RELAY_PIN = 18;
constexpr uint8_t RELAY_CONTACT_PIN = 19;
constexpr uint8_t RELAY_OFF_LEVEL = LOW;
constexpr uint8_t RELAY_ON_LEVEL = HIGH;
constexpr uint32_t SAMPLE_INTERVAL_MS = 2500;
constexpr uint32_t RELAY_TEST_DURATION_MS = 5000;

DHTesp moistureSensor;
uint32_t lastSampleMs = 0;
uint32_t relayTestStartedMs = 0;
bool relayTestActive = false;
bool automaticMode = false;
bool relayOutputOn = false;
farmtech::WeatherForecast weatherForecast;
farmtech::IrrigationController controller;
farmtech::SerialLine serialLine;

void setRelay(bool on) {
  relayOutputOn = on;
  digitalWrite(RELAY_PIN, on ? RELAY_ON_LEVEL : RELAY_OFF_LEVEL);
}

void updateAutomaticControl(uint32_t nowMs) {
  if (!automaticMode) return;
  controller.evaluate(nowMs, weatherForecast);
  setRelay(controller.pumpOn);
}

void printControl(uint32_t nowMs) {
  Serial.printf("CONTROLE MODO=%s CLIMA=%s BOMBA=%s MOTIVO=%s\n",
                automaticMode ? "AUTO" : "DIAGNOSTICO",
                weatherForecast.fresh(nowMs) ? "VALIDO" : "DESCONHECIDO",
                relayOutputOn ? "ON" : "OFF",
                automaticMode ? controller.reason : "TESTE_MANUAL");
}

void stopRelayTest() {
  setRelay(false);
  relayTestActive = false;
  Serial.println("RELE: comando OFF enviado; confira contato e LED.");
}

void processJsonCommand(uint32_t nowMs) {
  const auto result = farmtech::processWeatherCommand(serialLine.text, nowMs, weatherForecast);
  switch (result) {
    case farmtech::CommandResult::Weather:
      Serial.printf("ACK WEATHER fetched_at=%lu ttl_s=%lu\n",
                    static_cast<unsigned long>(weatherForecast.fetchedAt),
                    static_cast<unsigned long>(weatherForecast.ttlSeconds));
      break;
    case farmtech::CommandResult::Clear:
      Serial.println("ACK WEATHER_CLEAR");
      break;
    case farmtech::CommandResult::Auto:
      if (!automaticMode) {
        stopRelayTest();
        controller = {};
        automaticMode = true;
      }
      Serial.println("ACK AUTO");
      break;
    case farmtech::CommandResult::Reset:
      controller.timedOut = false;
      Serial.println("ACK RESET");
      break;
    case farmtech::CommandResult::Outdated:
      Serial.println("ERRO WEATHER_ANTIGO");
      return;
    case farmtech::CommandResult::Invalid:
      Serial.println("ERRO JSON_INVALIDO");
      return;
  }
  updateAutomaticControl(nowMs);
  printControl(nowMs);
}

void processSerialInput(uint32_t nowMs) {
  while (Serial.available() > 0) {
    const int command = Serial.read();
    if (serialLine.length == 0 && !serialLine.discarding && (command == '1' || command == '0')) {
      automaticMode = false;
      controller = {};
      if (command == '0') {
        stopRelayTest();
        continue;
      }
      if (!relayTestActive) {
        relayTestStartedMs = nowMs;
        relayTestActive = true;
        setRelay(true);
        Serial.println("RELE: comando ON por no maximo 5 segundos.");
      }
      continue;
    }
    if (serialLine.length == 0 && !serialLine.discarding && (command == '\r' || command == '\n')) continue;
    const auto status = serialLine.feed(static_cast<char>(command));
    if (status == farmtech::LineResult::Overflow) Serial.println("ERRO LINHA_LONGA");
    else if (status == farmtech::LineResult::Ready) processJsonCommand(nowMs);
  }
}

void printAlerts(const farmtech::CropAlerts& alerts) {
  const bool active[] = { alerts.nitrogenInsufficient, alerts.phosphorusInsufficient,
                          alerts.potassiumInsufficient, alerts.phOutOfRange, alerts.phInvalid };
  const char* names[] = { "N_INSUFICIENTE", "P_INSUFICIENTE", "K_INSUFICIENTE",
                          "PH_FORA_FAIXA", "PH_INVALIDO" };
  bool hasAlerts = false;
  Serial.print(" ALERTAS=");
  for (unsigned int alertIndex = 0; alertIndex < sizeof(active) / sizeof(active[0]); ++alertIndex) {
    if (!active[alertIndex]) continue;
    if (hasAlerts) Serial.print(',');
    Serial.print(names[alertIndex]);
    hasAlerts = true;
  }
  if (!hasAlerts) Serial.print("NENHUM");
  Serial.println();
}

void printReadings(uint32_t nowMs) {
  const TempAndHumidity reading = moistureSensor.getTempAndHumidity();
  const int rawPh = analogRead(PH_PIN);
  const float simulatedPh = farmtech::simulatedPhFromAdc(rawPh);
  const bool nitrogenOk = digitalRead(NITROGEN_PIN) == LOW;
  const bool phosphorusOk = digitalRead(PHOSPHORUS_PIN) == LOW;
  const bool potassiumOk = digitalRead(POTASSIUM_PIN) == LOW;
  const auto alerts = farmtech::evaluateCropAlerts(nitrogenOk, phosphorusOk, potassiumOk, simulatedPh);
  const bool humidityValid = isfinite(reading.humidity)
      && reading.humidity >= 0.0f && reading.humidity <= 100.0f;
  if (automaticMode) controller.sample(reading.humidity, nowMs);
  Serial.printf("N=%d P=%d K=%d ADC_LDR=%d ",
                nitrogenOk, phosphorusOk, potassiumOk, rawPh);
  if (humidityValid) {
    Serial.printf("UMIDADE_INDICE=%.1f ", reading.humidity);
  } else {
    Serial.print("UMIDADE_INDICE=ERRO ");
  }
  if (isfinite(reading.temperature)) {
    Serial.printf("TEMPERATURA_C=%.1f ", reading.temperature);
  } else {
    Serial.print("TEMPERATURA_C=ERRO ");
  }
  Serial.printf("RELE_COMANDO=%s CONTATO_NO=%s",
                relayOutputOn ? "ON" : "OFF",
                digitalRead(RELAY_CONTACT_PIN) == HIGH ? "FECHADO" : "ABERTO");
  if (isfinite(simulatedPh)) {
    Serial.printf(" PH_SIMULADO=%.3f", simulatedPh);
  } else {
    Serial.print(" PH_SIMULADO=ERRO");
  }
  printAlerts(alerts);
  updateAutomaticControl(nowMs);
  printControl(nowMs);
}

void setup() {
  digitalWrite(RELAY_PIN, RELAY_OFF_LEVEL);
  pinMode(RELAY_PIN, OUTPUT);
  pinMode(RELAY_CONTACT_PIN, INPUT_PULLDOWN);
  pinMode(NITROGEN_PIN, INPUT_PULLUP);
  pinMode(PHOSPHORUS_PIN, INPUT_PULLUP);
  pinMode(POTASSIUM_PIN, INPUT_PULLUP);
  pinMode(PH_PIN, INPUT);
  analogReadResolution(12);
  analogSetPinAttenuation(PH_PIN, ADC_11db);
  moistureSensor.setup(DHT_PIN, DHTesp::DHT22);
  Serial.begin(115200);
  Serial.println("FarmTech: inicio em diagnostico; irrigacao automatica por comando JSON auto.");
  Serial.println("N/P/K: 1=adequado; PH_SIMULADO: escala didatica 0-14, faixa 6.0-6.5.");
  Serial.println("Alertas nutricionais e de pH nao bloqueiam o teste manual do rele.");
  Serial.println("Serial: 1=testar rele por 5s; 0=desligar. Confira LED e CONTATO_NO.");
  Serial.println("JSON com LF: weather, weather_clear, auto ou reset; schema_version=1.");
  lastSampleMs = millis();
}

void loop() {
  const uint32_t nowMs = millis();
  if (relayTestActive && nowMs - relayTestStartedMs >= RELAY_TEST_DURATION_MS) {
    stopRelayTest();
  }
  updateAutomaticControl(nowMs);
  processSerialInput(nowMs);
  if (nowMs - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = nowMs;
    printReadings(nowMs);
  }
}