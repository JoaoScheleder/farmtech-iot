import argparse
import json
import subprocess
import sys
import threading
import time
import uuid
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

from tools import weather


ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / "firmware/irrigation/build"
BUILD_LOCK = threading.Lock()
JOBS_LOCK = threading.Lock()
JOBS = {}
DEFAULTS = {
    "humidity": 35, "temperature": 24, "adc": 1805,
    "nitrogen": True, "phosphorus": True, "potassium": True,
    "mode": "auto", "manual": False, "sensor": "valid", "reset": False,
}
CASES = {
    "custom": {"label": "Entradas atuais", "steps": [{"seconds": 10}]},
    "hysteresis": {"label": "Histerese 35 / 50 / 65 / 50", "steps": [
        {"humidity": value, "seconds": 5} for value in (35, 50, 65, 50)]},
    "rain": {"label": "Chuva suspende a bomba", "steps": [{"humidity": 35, "seconds": 10}],
             "weather": {"source": "synthetic", "rain": 2, "probability": 60, "ttl": 3600}},
    "critical": {"label": "Seca critica e recuperacao", "steps": [
        {"humidity": value, "seconds": 5} for value in (35, 25, 30, 40)],
        "weather": {"source": "synthetic", "rain": 2, "probability": 60, "ttl": 3600}},
    "expiry": {"label": "Previsao expirada", "steps": [{"humidity": 35, "seconds": 15}],
               "weather": {"source": "synthetic", "rain": 2, "probability": 60, "ttl": 5}},
    "missing": {"label": "Sem previsao", "steps": [{"humidity": 35, "seconds": 10}],
                "weather": {"source": "none"}},
    "fault": {"label": "Falha de sensor e recuperacao", "steps": [
        {"humidity": 35, "seconds": 5}, {"sensor": "invalid", "seconds": 5},
        {"sensor": "valid", "humidity": 35, "seconds": 5}]},
    "stale": {"label": "Sensor sem novas leituras", "steps": [
        {"humidity": 35, "seconds": 0}, {"sensor": "stale", "seconds": 10}]},
    "timeout": {"label": "Limite de 60 s e reset", "steps": [
        {"humidity": 35, "seconds": 65}, {"reset": True, "humidity": 35, "seconds": 5}],
        "weather": {"source": "none"}},
    "manual": {"label": "Teste manual de 5 s", "steps": [
        {"mode": "manual", "manual": True, "seconds": 10}]},
    "alerts": {"label": "Oito combinacoes NPK e pH", "steps": [
        {"nitrogen": bool(mask & 4), "phosphorus": bool(mask & 2),
         "potassium": bool(mask & 1), "adc": 1805 if mask == 7 else 1001,
         "seconds": 2.5} for mask in range(8)]},
}


def build_cli():
    executable = BUILD / "simulator-cli"
    sources = [ROOT / "tools/simulator_cli.cpp", ROOT / "firmware/irrigation/controller.h",
               ROOT / "firmware/irrigation/alerts.h"]
    with BUILD_LOCK:
        if not executable.exists() or any(path.stat().st_mtime > executable.stat().st_mtime for path in sources):
            BUILD.mkdir(parents=True, exist_ok=True)
            subprocess.run(["g++", "-std=c++11", "-Wall", "-Wextra", "-Werror",
                            "-I", str(ROOT / "firmware/irrigation"), str(sources[0]),
                            "-o", str(executable)], check=True, capture_output=True, text=True, timeout=30)
    return executable


def validate_inputs(values):
    if not isinstance(values, dict) or set(values) - set(DEFAULTS):
        raise ValueError("Entradas desconhecidas")
    result = {**DEFAULTS, **values}
    weather.number(result["humidity"], 0, 100, "humidity")
    weather.number(result["temperature"], -40, 80, "temperature")
    if type(result["adc"]) is not int or not 0 <= result["adc"] <= 4095:
        raise ValueError("ADC deve ser inteiro entre 0 e 4095")
    for key in ("nitrogen", "phosphorus", "potassium", "manual", "reset"):
        if type(result[key]) is not bool:
            raise ValueError(f"{key}: booleano obrigatorio")
    if result["mode"] not in ("auto", "manual") or result["sensor"] not in ("valid", "invalid", "stale"):
        raise ValueError("Modo ou estado do sensor invalido")
    return result


def forecast_values(settings):
    if not isinstance(settings, dict):
        raise ValueError("Configuracao meteorologica invalida")
    source = settings.get("source", "none")
    if source == "none":
        return False, 0, 0, 0
    if source == "synthetic":
        rain = weather.number(settings.get("rain"), 0, 6000, "rain")
        probability = weather.number(settings.get("probability"), 0, 100, "probability")
        ttl = settings.get("ttl")
        if type(ttl) is not int or not 1 <= ttl <= 3600:
            raise ValueError("Validade deve ser de 1 a 3600 segundos")
        return True, rain, probability, ttl
    if source == "api":
        command = settings.get("command")
        if command is None:
            return False, 0, 0, 0
        weather.validate_command(command)
        age = int(time.time()) - command["fetched_at_epoch_s"]
        if age < 0 or age >= command["ttl_s"]:
            return False, 0, 0, 0
        return True, command["precip_mm"], command["max_hourly_probability_pct"], command["ttl_s"] - age
    raise ValueError("Fonte meteorologica invalida")


def run_simulation(request):
    if not isinstance(request, dict):
        raise ValueError("Requisicao invalida")
    inputs = validate_inputs(request.get("inputs", {}))
    case = request.get("case", "custom")
    if not isinstance(case, str) or case not in CASES:
        raise ValueError("Cenario desconhecido")
    definition = CASES[case]
    steps = definition["steps"]
    if case == "custom":
        duration = weather.number(request.get("seconds", 10), 0, 120, "seconds")
        if duration % 2.5:
            raise ValueError("Duracao deve ser multipla de 2.5 segundos")
        steps = [{"seconds": duration}]
    settings = request.get("weather", definition.get("weather", {"source": "none"}))
    present, rain, probability, ttl = forecast_values(settings)
    rows, snapshots = [], []
    now = 0
    for step in steps:
        state = validate_inputs({**inputs, **{key: value for key, value in step.items() if key != "seconds"}})
        ticks = int(step["seconds"] * 1000) // 2500 + 1
        for tick in range(ticks):
            manual = int(state["manual"]) if tick == 0 else -1
            rows.append(" ".join(map(str, [now, -1 if state["sensor"] == "invalid" else state["humidity"],
                state["temperature"], state["adc"], int(state["nitrogen"]), int(state["phosphorus"]),
                int(state["potassium"]), int(state["mode"] == "auto"), manual,
                int(state["reset"] and tick == 0), int(state["sensor"] != "stale"),
                int(present), rain, probability, ttl])))
            snapshots.append(state)
            now += 2500
    completed = subprocess.run([str(build_cli())], input="\n".join(rows) + "\n",
                               capture_output=True, text=True, check=True, timeout=10)
    frames = [json.loads(line) for line in completed.stdout.splitlines()]
    if len(frames) != len(snapshots):
        raise RuntimeError("Resposta incompleta do controlador")
    for frame, state in zip(frames, snapshots):
        frame["inputs"] = state
    return {"frames": frames, "case": case, "weather": {"source": settings.get("source", "none"),
            "present": present, "rain": rain, "probability": probability, "ttl": ttl},
            "engine": "C++ local: controller.h + alerts.h"}


def fetch_command(latitude, longitude):
    weather.number(latitude, -90, 90, "latitude")
    weather.number(longitude, -180, 180, "longitude")
    completed = subprocess.run([sys.executable, str(ROOT / "tools/weather.py"),
        f"--latitude={latitude}", f"--longitude={longitude}"], capture_output=True, text=True, timeout=30)
    if completed.returncode:
        raise ValueError("Open-Meteo indisponivel. Previsao removida; controle por umidade.")
    return weather.validate_command(json.loads(completed.stdout))


def start_cloud_test():
    with JOBS_LOCK:
        if any(job["status"] == "running" for job in JOBS.values()):
            raise ValueError("Ja existe um teste Wokwi em execucao")
        identifier = uuid.uuid4().hex
        JOBS.clear()
        JOBS[identifier] = {"status": "running", "log": "", "exit_code": None}

    def execute():
        process = None
        try:
            process = subprocess.Popen(["node", str(ROOT / "tools/test-circuit.mjs")],
                cwd=ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
            output, _ = process.communicate(timeout=250)
            status = "passed" if process.returncode == 0 else "failed"
            code = process.returncode
        except subprocess.TimeoutExpired:
            process.terminate()
            try:
                output, _ = process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                output, _ = process.communicate()
            output += "\nTempo limite do teste Wokwi."
            status, code = "failed", -1
        except OSError:
            output, status, code = "Node.js ou Wokwi CLI indisponivel.", "failed", -1
        with JOBS_LOCK:
            JOBS[identifier] = {"status": status, "log": output[-60000:], "exit_code": code}

    threading.Thread(target=execute, daemon=True).start()
    return {"id": identifier, "status": "running"}


class Handler(BaseHTTPRequestHandler):
    def respond(self, status, value):
        body = json.dumps(value, allow_nan=False).encode()
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def do_GET(self):
        if self.path == "/api/cases":
            self.respond(200, {key: {"label": value["label"], "steps": value["steps"],
                                  "weather": value.get("weather")} for key, value in CASES.items()})
        elif self.path.startswith("/api/jobs/"):
            with JOBS_LOCK:
                job = JOBS.get(self.path.rsplit("/", 1)[-1])
                self.respond(200 if job else 404, job or {"error": "Execucao nao encontrada"})
        elif self.path == "/":
            body = (ROOT / "tools/simulator.html").read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)
        else:
            self.respond(404, {"error": "Nao encontrado"})

    def do_POST(self):
        try:
            if self.headers.get("Origin") not in (None, f"http://{self.headers.get('Host')}"):
                self.respond(403, {"error": "Origem nao permitida"})
                return
            length = int(self.headers.get("Content-Length", "0"))
            if not 0 < length <= 16384:
                raise ValueError("Tamanho de requisicao invalido")
            request = json.loads(self.rfile.read(length))
            if self.path == "/api/run":
                self.respond(200, run_simulation(request))
            elif self.path == "/api/weather":
                self.respond(200, fetch_command(request.get("latitude"), request.get("longitude")))
            elif self.path == "/api/cloud":
                self.respond(202, start_cloud_test())
            else:
                self.respond(404, {"error": "Nao encontrado"})
        except (ValueError, TypeError, AttributeError) as error:
            self.respond(400, {"error": str(error)})
        except (OSError, subprocess.SubprocessError, RuntimeError):
            self.respond(500, {"error": "Falha no CLI local. Verifique Python e g++."})


def main():
    parser = argparse.ArgumentParser(description="Painel local FarmTech")
    parser.add_argument("--port", type=int, default=8765)
    args = parser.parse_args()
    build_cli()
    server = ThreadingHTTPServer(("127.0.0.1", args.port), Handler)
    print(f"FarmTech local: http://127.0.0.1:{server.server_port}", flush=True)
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()


if __name__ == "__main__":
    main()