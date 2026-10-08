import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseEnv } from 'node:util';
import { spawnSync } from 'node:child_process';
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
const result = spawnSync('wokwi-cli', [
  project, '--scenario', 'circuit.test.yaml', '--timeout', '65000',
  '--serial-log-file', path.join(project, 'build', 'circuit-serial.log'),
], {
  env: { ...process.env, WOKWI_CLI_TOKEN: token },
  encoding: 'utf8', timeout: 240000, maxBuffer: 4 * 1024 * 1024,
});
for (const output of [result.stdout, result.stderr]) {
  if (output) process.stdout.write(output.replaceAll(token, '[REDACTED]'));
}
if (result.error) {
  console.error('Nao foi possivel concluir o Wokwi CLI; confira instalacao, rede e tempo limite.');
}
if (result.status === 0) {
  try {
    const log = fs.readFileSync(path.join(project, 'build', 'circuit-serial.log'), 'utf8');
    const readings = [...log.matchAll(/^N=(\d) P=(\d) K=(\d) ADC_LDR=(\d+) UMIDADE_INDICE=([\d.]+) TEMPERATURA_C=([\d.]+) RELE_COMANDO=(ON|OFF) CONTATO_NO=(ABERTO|FECHADO)/gm)];
    assert.ok(readings.length > 0, 'Nenhuma leitura completa registrada.');
    assert.equal(new Set(readings.map(reading => reading.slice(1, 4).join(''))).size, 8, 'Combinações de NPK incompletas.');
    for (const humidity of [0, 50, 100]) {
      assert.ok(readings.some(reading => Number(reading[5]) === humidity), `Umidade ${humidity} ausente.`);
    }
    for (const reading of readings) {
      assert.ok(Number(reading[4]) >= 0 && Number(reading[4]) <= 4095, 'ADC fora da escala.');
      assert.equal(reading[8], reading[7] === 'ON' ? 'FECHADO' : 'ABERTO', 'Contato diverge do comando.');
    }
    const adcAtTemperature = temperature => {
      const reading = readings.find(reading => Number(reading[6]) === temperature);
      assert.ok(reading, `Amostra do LDR com marcador ${temperature} ausente.`);
      return Number(reading[4]);
    };
    const dark = adcAtTemperature(21);
    const middle = adcAtTemperature(22);
    const bright = adcAtTemperature(23);
    assert.ok(dark > middle && middle > bright, 'ADC do LDR nao diminuiu com mais luz.');
    console.log(`OK: oito combinacoes NPK, umidades 0/50/100 e contato do rele consistente.`);
    console.log(`LDR: 0.1 lux=${dark}; 500 lux=${middle}; 100000 lux=${bright}.`);
  } catch (error) {
    console.error(`Falha na verificacao do registro: ${error.message.replaceAll(token, '[REDACTED]')}`);
    process.exit(1);
  }
}
process.exit(result.status ?? 1);