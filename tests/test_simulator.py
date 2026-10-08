import time
import unittest
from unittest.mock import patch

from tools import simulator


class SimulatorTest(unittest.TestCase):
    def run_case(self, case, **inputs):
        return simulator.run_simulation({"case": case, "inputs": inputs})["frames"]

    def test_hysteresis(self):
        frames = self.run_case("hysteresis")
        self.assertEqual([frame["pump_on"] for frame in frames[::3]], [True, True, False, False])

    def test_rain_critical_and_expiry(self):
        self.assertTrue(all(frame["reason"] == "RAIN_HOLD" for frame in self.run_case("rain")))
        frames = self.run_case("critical")
        self.assertEqual([frame["reason"] for frame in frames[::3]],
                         ["RAIN_HOLD", "CRITICAL_DRY", "CRITICAL_DRY", "RAIN_HOLD"])
        frames = self.run_case("expiry")
        self.assertEqual(frames[1]["reason"], "RAIN_HOLD")
        self.assertEqual(frames[2]["reason"], "MOISTURE_DEMAND")
        self.assertEqual(frames[2]["weather_status"], "DESCONHECIDO")

    def test_missing_fault_stale_timeout(self):
        self.assertEqual(self.run_case("missing")[0]["reason"], "MOISTURE_DEMAND")
        self.assertEqual([frame["reason"] for frame in self.run_case("fault")[::3]],
                         ["MOISTURE_DEMAND", "SENSOR_FAULT", "MOISTURE_DEMAND"])
        self.assertEqual(self.run_case("stale")[-1]["reason"], "SENSOR_FAULT")
        frames = self.run_case("timeout")
        self.assertEqual(frames[24]["reason"], "TIMEOUT")
        self.assertEqual(frames[-1]["reason"], "MOISTURE_DEMAND")

    def test_manual_and_alerts(self):
        frames = self.run_case("manual")
        self.assertEqual([frame["pump_on"] for frame in frames], [True, True, False, False, False])
        frames = self.run_case("alerts")
        self.assertEqual(len({tuple(frame["inputs"][key] for key in ("nitrogen", "phosphorus", "potassium"))
                              for frame in frames}), 8)
        self.assertEqual(frames[-1]["alerts"], [])
        self.assertIn("PH_FORA_FAIXA", frames[0]["alerts"])

    def test_validation(self):
        for inputs in ({"adc": 1.5}, {"nitrogen": 1}, {"humidity": float("nan")},
                       {"mode": "bad"}, {"extra": True}):
            with self.subTest(inputs=inputs), self.assertRaises(ValueError):
                self.run_case("custom", **inputs)
        with self.assertRaises(ValueError):
            simulator.run_simulation({"case": "unknown"})

    def test_api_absent_and_expired_fall_back(self):
        fetched = int(time.time()) - 3601
        start = ((fetched + 3599) // 3600) * 3600
        command = {"type": "weather", "schema_version": 1, "fetched_at_epoch_s": fetched,
                   "window_start_epoch_s": start, "window_end_epoch_s": start + 21600,
                   "precip_mm": 2, "max_hourly_probability_pct": 60, "ttl_s": 3600}
        for value in (None, command):
            result = simulator.run_simulation({"weather": {"source": "api", "command": value}})
            self.assertEqual(result["frames"][0]["reason"], "MOISTURE_DEMAND")
            self.assertFalse(result["weather"]["present"])

    def test_weather_cli_failure(self):
        with patch.object(simulator.subprocess, "run") as run:
            run.return_value.returncode = 1
            with self.assertRaisesRegex(ValueError, "controle por umidade"):
                simulator.fetch_command(0, 0)

    def test_explicit_weather_overrides_preset(self):
        result = simulator.run_simulation({"case": "rain", "weather": {"source": "none"}})
        self.assertEqual(result["frames"][0]["reason"], "MOISTURE_DEMAND")

    def test_custom_duration(self):
        self.assertEqual(len(simulator.run_simulation({"seconds": 5})["frames"]), 3)
        for seconds in (121, -1, 3, True):
            with self.subTest(seconds=seconds), self.assertRaises(ValueError):
                simulator.run_simulation({"seconds": seconds})

    def test_cloud_job_executes_only_existing_runner(self):
        with patch.object(simulator.threading, "Thread") as thread:
            with patch.object(simulator.subprocess, "Popen") as popen:
                process = popen.return_value
                process.communicate.return_value = ("OK: suite mock", None)
                process.returncode = 0
                thread.return_value.start.side_effect = lambda: thread.call_args.kwargs["target"]()
                job = simulator.start_cloud_test()
                self.assertEqual(simulator.JOBS[job["id"]]["status"], "passed")
                self.assertEqual(popen.call_args.args[0], ["node", str(simulator.ROOT / "tools/test-circuit.mjs")])
                self.assertNotIn("shell", popen.call_args.kwargs)


if __name__ == "__main__":
    unittest.main()