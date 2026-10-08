#pragma once

#include <ArduinoJson.h>
#include <cstddef>
#include <cstring>
#include "controller.h"

namespace farmtech {

enum class CommandResult { Invalid, Outdated, Weather, Clear, Auto, Reset };
enum class LineResult { Pending, Ready, Overflow };

struct SerialLine {
  char text[512] = {};
  size_t length = 0;
  bool discarding = false;

  LineResult feed(char character) {
    if (character == '\n') {
      const bool overflowed = discarding;
      text[length] = '\0';
      length = 0;
      discarding = false;
      return overflowed ? LineResult::Overflow : LineResult::Ready;
    }
    if (discarding) return LineResult::Pending;
    if (length == sizeof(text) - 1) {
      discarding = true;
      return LineResult::Pending;
    }
    text[length++] = character;
    return LineResult::Pending;
  }
};

struct JsonReader {
  const char* text;
  size_t length;
  size_t position = 0;

  int read() {
    return position < length ? static_cast<unsigned char>(text[position++]) : -1;
  }

  size_t readBytes(char* buffer, size_t count) {
    const size_t available = length - position;
    if (count > available) count = available;
    std::memcpy(buffer, text + position, count);
    position += count;
    return count;
  }
};

inline CommandResult processWeatherCommand(const char* text, uint32_t nowMs, WeatherForecast& weather) {
  JsonReader reader;
  reader.text = text;
  reader.length = std::strlen(text);
  JsonDocument document;
  const auto error = deserializeJson(document, reader, DeserializationOption::NestingLimit(2));
  if (error || !document.is<JsonObject>()) return CommandResult::Invalid;
  while (reader.position < reader.length) {
    const char character = reader.text[reader.position++];
    if (character != ' ' && character != '\r' && character != '\t') return CommandResult::Invalid;
  }
  if (!document["type"].is<const char*>() || !document["schema_version"].is<uint32_t>()
      || document["schema_version"].as<uint32_t>() != 1) return CommandResult::Invalid;
  const char* type = document["type"];
  if (std::strcmp(type, "weather_clear") == 0 && document.size() == 2) {
    weather.present = false;
    return CommandResult::Clear;
  }
  if (std::strcmp(type, "auto") == 0 && document.size() == 2) return CommandResult::Auto;
  if (std::strcmp(type, "reset") == 0 && document.size() == 2) return CommandResult::Reset;
  if (std::strcmp(type, "weather") != 0 || document.size() != 8) return CommandResult::Invalid;
  for (const char* name : {"fetched_at_epoch_s", "window_start_epoch_s", "window_end_epoch_s", "ttl_s"}) {
    if (!document[name].is<uint32_t>() || document[name].as<uint32_t>() == 0) return CommandResult::Invalid;
  }
  for (const char* name : {"precip_mm", "max_hourly_probability_pct"}) {
    if (!document[name].is<double>()) return CommandResult::Invalid;
  }
  const uint32_t fetched = document["fetched_at_epoch_s"];
  const uint32_t start = document["window_start_epoch_s"];
  const uint32_t end = document["window_end_epoch_s"];
  const uint32_t ttl = document["ttl_s"];
  const double rain = document["precip_mm"];
  const double probability = document["max_hourly_probability_pct"];
  const uint64_t expectedStart = ((static_cast<uint64_t>(fetched) + 3599u) / 3600u) * 3600u;
  if (start != expectedStart || static_cast<uint64_t>(end) != expectedStart + 21600u
      || ttl > 3600 || !std::isfinite(rain) || rain < 0 || rain > 6000
      || !std::isfinite(probability) || probability < 0 || probability > 100) return CommandResult::Invalid;
  if (fetched <= weather.fetchedAt) return CommandResult::Outdated;
  WeatherForecast accepted;
  accepted.present = true;
  accepted.fetchedAt = fetched;
  accepted.windowStart = start;
  accepted.windowEnd = end;
  accepted.precipitationMm = static_cast<float>(rain);
  accepted.probabilityPct = static_cast<float>(probability);
  accepted.ttlSeconds = ttl;
  accepted.receivedMs = nowMs;
  weather = accepted;
  return CommandResult::Weather;
}

}