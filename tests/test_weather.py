import copy
import io
import json
import unittest
from contextlib import redirect_stderr, redirect_stdout
from datetime import datetime, timezone
from unittest.mock import patch
from urllib.error import HTTPError, URLError

from tools import weather


class WeatherTest(unittest.TestCase):
    def setUp(self):
        self.hour = int(datetime(2026, 10, 8, 23, tzinfo=timezone.utc).timestamp())
        self.payload = {
            "utc_offset_seconds": 0,
            "hourly_units": {"time": "unixtime", "precipitation": "mm", "precipitation_probability": "%"},
            "hourly": {
                "time": [self.hour + offset * 3600 for offset in range(10)],
                "precipitation": [999, 999, 0, 0.5, 1, 0, 0.5, 0, 999, 999],
                "precipitation_probability": [99, 99, 10, 20, 60, 30, 40, 50, 99, 99],
            },
        }

    def test_future_intervals_cross_midnight(self):
        command = weather.summarize(self.payload, self.hour + 1)
        self.assertEqual(command["window_start_epoch_s"], self.hour + 3600)
        self.assertEqual(command["window_end_epoch_s"], self.hour + 7 * 3600)
        self.assertEqual(command["precip_mm"], 2)
        self.assertEqual(command["max_hourly_probability_pct"], 60)
        self.assertEqual(command["ttl_s"], 3600)
        self.assertLess(len(json.dumps(command)), 512)

    def test_exact_hour_includes_next_hour_end(self):
        command = weather.summarize(self.payload, self.hour)
        self.assertEqual(command["window_start_epoch_s"], self.hour)
        self.assertEqual(command["precip_mm"], 1001)

    def test_invalid_responses(self):
        invalid = []
        for field, value in [("precipitation", None), ("precipitation", -1),
                             ("precipitation", float("nan")), ("precipitation", True),
                             ("precipitation_probability", 101), ("precipitation_probability", "60")]:
            payload = copy.deepcopy(self.payload)
            payload["hourly"][field][2] = value
            invalid.append(payload)
        for field, value in [("time", []), ("time", [self.hour]), ("precipitation", []),
                             ("precipitation_probability", None)]:
            payload = copy.deepcopy(self.payload)
            payload["hourly"][field] = value
            invalid.append(payload)
        for index, value in [(2, self.hour), (2, self.hour + 2 * 3600 + 1), (2, True)]:
            payload = copy.deepcopy(self.payload)
            payload["hourly"]["time"][index] = value
            invalid.append(payload)
        payload = copy.deepcopy(self.payload)
        for series in payload["hourly"].values():
            series.pop(4)
        invalid.append(payload)
        invalid.extend([{}, [], {**self.payload, "utc_offset_seconds": 3600},
                        {**self.payload, "hourly_units": {}}, {**self.payload, "error": True}])
        for payload in invalid:
            with self.subTest(payload=payload), self.assertRaises(ValueError):
                weather.summarize(payload, self.hour + 1)

    def test_command_validation(self):
        valid = weather.summarize(self.payload, self.hour + 1)
        for field, value in [("schema_version", True), ("ttl_s", 0), ("ttl_s", 3601),
                             ("ttl_s", 1.0), ("precip_mm", float("inf")),
                             ("fetched_at_epoch_s", "123"), ("window_end_epoch_s", 1)]:
            with self.subTest(field=field, value=value), self.assertRaises(ValueError):
                weather.validate_command({**valid, field: value})
        with self.assertRaises(ValueError):
            weather.validate_command({**valid, "extra": 1})

    def test_coordinates_rejected_before_network(self):
        with patch.object(weather, "urlopen") as network:
            for latitude, longitude in [(91, 0), (0, -181), (float("nan"), 0), (True, 0)]:
                with self.assertRaises(ValueError):
                    weather.fetch_forecast(latitude, longitude)
            network.assert_not_called()

    def test_http_retries_bounded(self):
        for error, attempts in [(URLError("offline"), 2), (TimeoutError(), 2),
                                (HTTPError("url", 503, "error", None, None), 2),
                                (HTTPError("url", 400, "error", None, None), 1)]:
            with patch.object(weather, "urlopen", side_effect=error) as network:
                with self.assertRaises((OSError, URLError)):
                    weather.fetch_forecast(0, 0)
                self.assertEqual(network.call_count, attempts)
                self.assertEqual(network.call_args.kwargs["timeout"], 10)

    def test_cli_error_has_no_weather_output(self):
        output, errors = io.StringIO(), io.StringIO()
        with patch.object(weather, "fetch_forecast", side_effect=URLError("offline")):
            with redirect_stdout(output), redirect_stderr(errors):
                status = weather.main(["--latitude=-23.55", "--longitude=-46.63"])
        self.assertEqual(status, 1)
        self.assertEqual(output.getvalue(), "")
        self.assertIn("Nenhuma previsao valida", errors.getvalue())

    def test_cli_clear(self):
        output = io.StringIO()
        with redirect_stdout(output):
            self.assertEqual(weather.main(["--clear"]), 0)
        self.assertEqual(json.loads(output.getvalue()), {"type": "weather_clear", "schema_version": 1})


if __name__ == "__main__":
    unittest.main()