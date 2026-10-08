import argparse
import json
import math
import sys
from datetime import datetime, timezone
from pathlib import Path
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import urlopen


API_URL = "https://api.open-meteo.com/v1/forecast"
MAX_RESPONSE_BYTES = 1_000_000
MAX_EPOCH = 4_294_967_295


def number(value, minimum, maximum, name):
    if type(value) not in (int, float) or not math.isfinite(value):
        raise ValueError(f"{name}: numero finito obrigatorio")
    if not minimum <= value <= maximum:
        raise ValueError(f"{name}: fora da faixa {minimum}..{maximum}")
    return value


def epoch(value, name):
    if type(value) is not int or not 0 < value <= MAX_EPOCH:
        raise ValueError(f"{name}: timestamp inteiro UTC invalido")
    return value


def validate_command(command):
    fields = {
        "type", "schema_version", "fetched_at_epoch_s", "window_start_epoch_s",
        "window_end_epoch_s", "precip_mm", "max_hourly_probability_pct", "ttl_s",
    }
    if not isinstance(command, dict) or set(command) != fields:
        raise ValueError("Campos do comando invalidos")
    if command["type"] != "weather" or type(command["schema_version"]) is not int:
        raise ValueError("Tipo ou versao do comando invalido")
    if command["schema_version"] != 1:
        raise ValueError("Versao do comando nao suportada")
    fetched = epoch(command["fetched_at_epoch_s"], "fetched_at_epoch_s")
    start = epoch(command["window_start_epoch_s"], "window_start_epoch_s")
    end = epoch(command["window_end_epoch_s"], "window_end_epoch_s")
    if start != ((fetched + 3599) // 3600) * 3600 or end - start != 21600:
        raise ValueError("Janela deve conter seis horas completas futuras UTC")
    number(command["precip_mm"], 0, 6000, "precip_mm")
    number(command["max_hourly_probability_pct"], 0, 100, "max_hourly_probability_pct")
    if type(command["ttl_s"]) is not int or not 1 <= command["ttl_s"] <= 3600:
        raise ValueError("ttl_s deve ser inteiro entre 1 e 3600")
    return command


def summarize(payload, fetched_at):
    fetched = epoch(fetched_at, "fetched_at_epoch_s")
    if not isinstance(payload, dict) or payload.get("error"):
        raise ValueError("Resposta da API invalida")
    if type(payload.get("utc_offset_seconds")) is not int or payload["utc_offset_seconds"] != 0:
        raise ValueError("Resposta deve usar UTC")
    units = payload.get("hourly_units")
    expected_units = {"time": "unixtime", "precipitation": "mm", "precipitation_probability": "%"}
    if not isinstance(units, dict) or any(units.get(key) != value for key, value in expected_units.items()):
        raise ValueError("Unidades horarias invalidas")
    hourly = payload.get("hourly")
    if not isinstance(hourly, dict):
        raise ValueError("Series horarias ausentes")
    times = hourly.get("time")
    rain = hourly.get("precipitation")
    probability = hourly.get("precipitation_probability")
    if any(not isinstance(series, list) or not series for series in (times, rain, probability)):
        raise ValueError("Series horarias vazias ou invalidas")
    if not len(times) == len(rain) == len(probability):
        raise ValueError("Series horarias com tamanhos diferentes")
    previous = 0
    for timestamp in times:
        epoch(timestamp, "hourly.time")
        if timestamp % 3600 or timestamp <= previous:
            raise ValueError("Timestamps devem ser horarios UTC estritamente crescentes")
        previous = timestamp
    start = ((fetched + 3599) // 3600) * 3600
    indexes = {timestamp: index for index, timestamp in enumerate(times)}
    total = 0.0
    peak = 0.0
    for end in range(start + 3600, start + 21601, 3600):
        if end not in indexes:
            raise ValueError("Previsao nao cobre seis intervalos completos futuros")
        index = indexes[end]
        total += number(rain[index], 0, 1000, "precipitation")
        peak = max(peak, number(probability[index], 0, 100, "precipitation_probability"))
    return validate_command({
        "type": "weather", "schema_version": 1, "fetched_at_epoch_s": fetched,
        "window_start_epoch_s": start, "window_end_epoch_s": start + 21600,
        "precip_mm": total, "max_hourly_probability_pct": peak, "ttl_s": 3600,
    })


def fetch_forecast(latitude, longitude):
    number(latitude, -90, 90, "latitude")
    number(longitude, -180, 180, "longitude")
    query = urlencode({
        "latitude": latitude, "longitude": longitude,
        "hourly": "precipitation,precipitation_probability", "timezone": "GMT",
        "timeformat": "unixtime", "precipitation_unit": "mm", "forecast_days": 2,
    })
    for attempt in range(2):
        try:
            with urlopen(f"{API_URL}?{query}", timeout=10) as response:
                body = response.read(MAX_RESPONSE_BYTES + 1)
            if len(body) > MAX_RESPONSE_BYTES:
                raise ValueError("Resposta excede o limite de tamanho")
            return json.loads(body)
        except HTTPError as error:
            if attempt == 1 or (error.code != 429 and not 500 <= error.code < 600):
                raise
        except (URLError, TimeoutError):
            if attempt == 1:
                raise
    raise RuntimeError("Consulta meteorologica nao concluida")


def main(argv=None):
    parser = argparse.ArgumentParser(description="Previsao Open-Meteo para colar no Serial do Wokwi.")
    parser.add_argument("--latitude", type=float)
    parser.add_argument("--longitude", type=float)
    parser.add_argument("--save", type=Path, help="Salvar resposta e comando para auditoria, nao para reutilizar.")
    parser.add_argument("--clear", action="store_true", help="Emitir comando para remover a previsao no ESP32.")
    args = parser.parse_args(argv)
    if args.clear:
        if args.latitude is not None or args.longitude is not None or args.save is not None:
            parser.error("--clear nao aceita coordenadas ou --save")
        print('{"type":"weather_clear","schema_version":1}')
        return 0
    if args.latitude is None or args.longitude is None:
        parser.error("Informe --latitude e --longitude")
    try:
        payload = fetch_forecast(args.latitude, args.longitude)
        fetched = int(datetime.now(timezone.utc).timestamp())
        command = summarize(payload, fetched)
        output = json.dumps(command, separators=(",", ":"), allow_nan=False)
        if len(output.encode("ascii")) > 511:
            raise ValueError("Comando excede o buffer Serial")
        if args.save:
            args.save.write_text(json.dumps({
                "source": API_URL, "requested_coordinates": {
                    "latitude": args.latitude, "longitude": args.longitude,
                }, "command": command, "response": payload,
            }, indent=2, allow_nan=False) + "\n", encoding="utf-8")
        print(output)
        return 0
    except (ValueError, OSError) as error:
        print(f"Erro meteorologico: {error}. Nenhuma previsao valida emitida.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())