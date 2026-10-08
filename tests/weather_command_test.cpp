#include <cassert>
#include <iostream>
#include <string>
#include "weather_command.h"

int main() {
  using farmtech::CommandResult;
  farmtech::WeatherForecast weather;
  const std::string valid = R"({"type":"weather","schema_version":1,"fetched_at_epoch_s":1791463443,"window_start_epoch_s":1791464400,"window_end_epoch_s":1791486000,"precip_mm":2,"max_hourly_probability_pct":60,"ttl_s":10})";
  assert(farmtech::processWeatherCommand(valid.c_str(), 100, weather) == CommandResult::Weather);
  assert(weather.holdsWatering(100) && weather.receivedMs == 100);
  assert(farmtech::processWeatherCommand(valid.c_str(), 200, weather) == CommandResult::Outdated);
  assert(weather.receivedMs == 100);
  for (const std::string& invalid : {std::string("{}"), std::string("[]"), valid + "{}", valid + "garbage",
                                    valid.substr(0, valid.size() - 1), std::string("null")}) {
    assert(farmtech::processWeatherCommand(invalid.c_str(), 300, weather) == CommandResult::Invalid);
    assert(weather.receivedMs == 100 && weather.precipitationMm == 2);
  }
  for (const auto& replacement : {
      std::pair<std::string, std::string>{"\"ttl_s\":10", "\"ttl_s\":3601"},
      {"\"ttl_s\":10", "\"ttl_s\":0"}, {"\"ttl_s\":10", "\"ttl_s\":10.0"},
      {"\"schema_version\":1", "\"schema_version\":true"},
      {"\"schema_version\":1", "\"schema_version\":2"},
      {"\"precip_mm\":2", "\"precip_mm\":\"2\""}, {"\"precip_mm\":2", "\"precip_mm\":true"},
      {"\"precip_mm\":2", "\"precip_mm\":null"}, {"\"precip_mm\":2", "\"precip_mm\":-1"},
      {"\"max_hourly_probability_pct\":60", "\"max_hourly_probability_pct\":101"},
      {"1791464400", "1791464401"}, {"1791463443", "-1"}, {"1791486000", "1791489600"}}) {
    std::string invalid = valid;
    invalid.replace(invalid.find(replacement.first), replacement.first.size(), replacement.second);
    assert(farmtech::processWeatherCommand(invalid.c_str(), 300, weather) == CommandResult::Invalid);
    assert(weather.receivedMs == 100);
  }
  std::string unknown = valid;
  unknown.insert(unknown.size() - 1, ",\"extra\":1");
  assert(farmtech::processWeatherCommand(unknown.c_str(), 300, weather) == CommandResult::Invalid);
  assert(farmtech::processWeatherCommand("{\"type\":\"weather_clear\",\"schema_version\":1}", 400, weather) == CommandResult::Clear);
  assert(!weather.present && weather.fetchedAt == 1791463443);
  assert(farmtech::processWeatherCommand(valid.c_str(), 500, weather) == CommandResult::Outdated);
  assert(farmtech::processWeatherCommand("{\"type\":\"auto\",\"schema_version\":1}", 500, weather) == CommandResult::Auto);
  assert(farmtech::processWeatherCommand("{\"type\":\"reset\",\"schema_version\":1}", 500, weather) == CommandResult::Reset);

  farmtech::SerialLine line;
  for (const char character : valid + "\r") assert(line.feed(character) == farmtech::LineResult::Pending);
  assert(line.feed('\n') == farmtech::LineResult::Ready);
  weather = {};
  assert(farmtech::processWeatherCommand(line.text, 600, weather) == CommandResult::Weather);
  for (int index = 0; index < 600; ++index) line.feed('1');
  assert(line.discarding);
  assert(line.feed('\n') == farmtech::LineResult::Overflow);
  for (const char character : valid) line.feed(character);
  assert(line.feed('\n') == farmtech::LineResult::Ready);
  assert(std::string(line.text) == valid);
  std::cout << "OK: JSON estrito, atualizacao atomica, replay, CRLF e recuperacao apos overflow.\n";
}