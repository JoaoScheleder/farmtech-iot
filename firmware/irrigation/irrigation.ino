#include <Arduino.h>
#include <DHTesp.h>
#include <math.h>

constexpr uint8_t NITROGEN_PIN = 25;
constexpr uint8_t PHOSPHORUS_PIN = 26;
constexpr uint8_t POTASSIUM_PIN = 27;
constexpr uint8_t PH_PIN = 34;
constexpr uint8_t DHT_PIN = 16;
constexpr uint8_t RELAY_PIN = 18;
constexpr uint8_t RELAY_CONTACT_PIN = 19;
constexpr uint8_t RELAY_OFF_LEVEL = HIGH;
constexpr uint8_t RELAY_ON_LEVEL = LOW;
constexpr uint32_t SAMPLE_INTERVAL_MS = 2500;
constexpr uint32_t RELAY_TEST_DURATION_MS = 5000;

DHTesp moistureSensor;
uint32_t lastSampleMs = 0;
uint32_t relayTestStartedMs = 0;
bool relayTestActive = false;

void stopRelayTest() {
  digitalWrite(RELAY_PIN, RELAY_OFF_LEVEL);
  relayTestActive = false;
  Serial.println("RELE: comando OFF enviado; confira contato e LED.");
}

void processSerialInput(uint32_t nowMs) {
  while (Serial.available() > 0) {
    const int command = Serial.read();
    if (command == '1') {
      if (!relayTestActive) {
        relayTestStartedMs = nowMs;
        relayTestActive = true;
        digitalWrite(RELAY_PIN, RELAY_ON_LEVEL);
        Serial.println("RELE: comando ON por no maximo 5 segundos.");
      }
    } else if (command == '0') {
      stopRelayTest();
    } else if (command != '\r' && command != '\n') {
      Serial.println("Comando invalido. Use 1 para testar ou 0 para desligar.");
    }
  }
}

void printReadings() {
  const TempAndHumidity reading = moistureSensor.getTempAndHumidity();
  const int rawPh = analogRead(PH_PIN);
  const bool humidityValid = isfinite(reading.humidity)
      && reading.humidity >= 0.0f && reading.humidity <= 100.0f;
  Serial.printf("N=%d P=%d K=%d ADC_LDR=%d ",
                digitalRead(NITROGEN_PIN) == LOW,
                digitalRead(PHOSPHORUS_PIN) == LOW,
                digitalRead(POTASSIUM_PIN) == LOW, rawPh);
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
  Serial.printf("RELE_COMANDO=%s CONTATO_NO=%s\n",
                relayTestActive ? "ON" : "OFF",
                digitalRead(RELAY_CONTACT_PIN) == HIGH ? "FECHADO" : "ABERTO");
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
  Serial.println("FarmTech: diagnostico do circuito, sem irrigacao automatica.");
  Serial.println("N/P/K: 1=pressionado; ADC_LDR: leitura bruta, nao pH calibrado.");
  Serial.println("Serial: 1=testar rele por 5s; 0=desligar. Confira LED e CONTATO_NO.");
  lastSampleMs = millis();
}

void loop() {
  const uint32_t nowMs = millis();
  if (relayTestActive && nowMs - relayTestStartedMs >= RELAY_TEST_DURATION_MS) {
    stopRelayTest();
  }
  processSerialInput(nowMs);
  if (nowMs - lastSampleMs >= SAMPLE_INTERVAL_MS) {
    lastSampleMs = nowMs;
    printReadings();
  }
}