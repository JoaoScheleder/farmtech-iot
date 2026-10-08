import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseEnv } from 'node:util';
import { spawn } from 'node:child_process';
import assert from 'node:assert/strict';

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const project = path.join(root, 'firmware', 'irrigation');
let localEnv = {};
try {
  const envFile = path.join(root, '.env');
  if (fs.existsSync(envFile)) localEnv = parseEnv(fs.readFileSync(envFile, 'utf8'));
} catch {
  console.error('Nao foi possivel carregar o arquivo local de credenciais.');
  process.exit(1);
}
const token = process.env.WOKWI_CLI_TOKEN || localEnv.WOKWI_CLI_TOKEN
    || process.env.WOKWI_API_KEY || localEnv.WOKWI_API_KEY;
if (!token) {
  console.error('Defina WOKWI_CLI_TOKEN ou WOKWI_API_KEY no ambiente ou no .env.');
  process.exit(1);
}
if (!fs.existsSync(path.join(project, 'build', 'irrigation.ino.bin'))) {
  console.error('Compile firmware/irrigation com --output-dir firmware/irrigation/build.');
  process.exit(1);
}
const child = spawn('wokwi-cli', [
  project, '--scenario', 'circuit.test.yaml', '--timeout', '65000',
  '--serial-log-file', path.join(project, 'build', 'circuit-serial.log'),
], {
  env: { ...process.env, WOKWI_CLI_TOKEN: token },
  stdio: ['ignore', 'pipe', 'pipe'],
});
for (const stream of [child.stdout, child.stderr]) {
  let pending = '';
  stream.setEncoding('utf8');
  stream.on('data', chunk => {
    pending += chunk;
    const lines = pending.split('\n');
    pending = lines.pop();
    for (const line of lines) process.stdout.write(`${line.replaceAll(token, '[REDACTED]')}\n`);
  });
  stream.on('end', () => {
    if (pending) process.stdout.write(pending.replaceAll(token, '[REDACTED]'));
  });
}
process.once('SIGINT', () => child.kill('SIGINT'));
process.once('SIGTERM', () => child.kill('SIGTERM'));
const result = await new Promise(resolve => {
  let error;
  const deadline = setTimeout(() => {
    error = new Error('Tempo limite de execucao excedido.');
    child.kill('SIGKILL');
  }, 240000);
  child.once('error', () => { error = new Error('Falha ao executar o CLI.'); });
  child.once('close', status => {
    clearTimeout(deadline);
    resolve({ status, error });
  });
});
if (result.error) {
  console.error('Nao foi possivel concluir o Wokwi CLI; confira instalacao, rede e tempo limite.');
}
if (result.status === 0) {
  try {
    const log = fs.readFileSync(path.join(project, 'build', 'circuit-serial.log'), 'utf8');
    const readings = [...log.matchAll(/N=(\d) P=(\d) K=(\d) ADC_LDR=(\d+) UMIDADE_INDICE=([\d.]+) TEMPERATURA_C=([\d.]+) RELE_COMANDO=(ON|OFF) CONTATO_NO=(ABERTO|FECHADO) PH_SIMULADO=([\d.]+) ALERTAS=([A-Z_,]+?)(?=N=[01]|RELE:|\s|$)/g)];
    assert.ok(readings.length > 0, 'Nenhuma leitura completa registrada.');
    assert.equal(new Set(readings.map(reading => reading.slice(1, 4).join(''))).size, 8, 'Combinações de NPK incompletas.');
    for (const humidity of [0, 50, 100]) {
      assert.ok(readings.some(reading => Number(reading[5]) === humidity), `Umidade ${humidity} ausente.`);
    }
    for (const reading of readings) {
      assert.ok(Number(reading[4]) >= 0 && Number(reading[4]) <= 4095, 'ADC fora da escala.');
      assert.equal(reading[8], reading[7] === 'ON' ? 'FECHADO' : 'ABERTO', 'Contato diverge do comando.');
      const ph = Math.max(0, Math.min(14, (Number(reading[4]) - 32) * 14 / (4063 - 32)));
      assert.ok(Math.abs(Number(reading[9]) - ph) <= 0.00051, 'Conversao do pH diverge do ADC.');
      const expectedAlerts = [];
      for (const [position, name] of [[1, 'N_INSUFICIENTE'], [2, 'P_INSUFICIENTE'], [3, 'K_INSUFICIENTE']]) {
        if (reading[position] === '0') expectedAlerts.push(name);
      }
      if (ph < 6 || ph > 6.5) expectedAlerts.push('PH_FORA_FAIXA');
      assert.equal(reading[10], expectedAlerts.join(',') || 'NENHUM', 'Alertas divergem das entradas.');
    }
    assert.ok(readings.some(reading => reading[10] === 'NENHUM'), 'Cenario sem alertas ausente.');
    assert.ok(readings.some(reading => reading[7] === 'ON'
        && reading[10] === 'N_INSUFICIENTE,P_INSUFICIENTE,K_INSUFICIENTE,PH_FORA_FAIXA'),
    'Relé ligado com alertas simultaneos nao foi verificado.');
    const adcAtTemperature = temperature => {
      const reading = readings.find(reading => Number(reading[6]) === temperature);
      assert.ok(reading, `Amostra do LDR com marcador ${temperature} ausente.`);
      return Number(reading[4]);
    };
    const dark = adcAtTemperature(21);
    const middle = adcAtTemperature(22);
    const bright = adcAtTemperature(23);
    assert.ok(dark > middle && middle > bright, 'ADC do LDR nao diminuiu com mais luz.');
    console.log(`OK: oito combinacoes NPK, conversao e alertas de pH, caso sem alertas e rele independente.`);
    console.log(`OK: umidades 0/50/100 e contato do rele consistente.`);
    console.log(`LDR: 0.1 lux=${dark}; 500 lux=${middle}; 100000 lux=${bright}.`);
  } catch (error) {
    console.error(`Falha na verificacao do registro: ${error.message.replaceAll(token, '[REDACTED]')}`);
    process.exit(1);
  }
}
process.exit(result.status ?? 1);