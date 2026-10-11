# FarmTech IoT

Projeto educacional de irrigação inteligente da FarmTech Solutions, desenvolvido para a atividade da FIAP descrita em [CONTEXT.md](CONTEXT.md). A cultura escolhida é o tomateiro. O sistema combina um circuito ESP32 no Wokwi, regras de controle em C++, consulta meteorológica em Python e um painel local para reproduzir cenários.

O DHT22 representa didaticamente a umidade do solo; três botões representam N, P e K; o LDR representa pH; e um relé representa a bomba. Não há sensor agrícola real, bomba física nem modelo de evolução da umidade.

## Estado atual

- O firmware implementa diagnóstico manual e controle automático, ativado explicitamente pelo Monitor Serial.
- O painel local executa as mesmas regras C++ de irrigação e alertas usadas pelo firmware.
- A integração Python consulta o Open-Meteo e gera JSON para transferência manual ao ESP32 pelo Serial.
- Há testes locais de controle, alertas, mensagens meteorológicas e simulação. O diagnóstico e os alertas possuem registros de validação anteriores no Wokwi.
- A suíte ampliada na nuvem, incluindo clima, teve uma execução interrompida por limite de tempo. Isso não comprova a integração completa no Wokwi.
- A análise em R, as imagens reais do circuito, o endereço público do projeto Wokwi e o link do vídeo de entrega continuam pendentes.

Alguns documentos registram etapas anteriores e ainda descrevem o controle automático como proposta. Este guia descreve a implementação atual; as evidências históricas e as pendências não devem ser confundidas com funcionalidades implementadas.

## Estrutura e responsabilidades

```text
farmtech-iot/
├── README.md                 Guia geral e execução
├── CONTEXT.md                Enunciado e requisitos da atividade
├── docs/                     Pesquisa, especificação e guias
├── firmware/
│   └── irrigation/           Sketch ESP32, regras e circuito Wokwi
│       └── build/            Binários, cenários gerados e logs locais
├── tools/                    Painel, executores e integração meteorológica
└── tests/                    Testes C++ e Python
```


### Firmware e circuito

| Arquivo | Responsabilidade |
| --- | --- |
| [irrigation.ino](firmware/irrigation/irrigation.ino) | Inicializar GPIOs e DHT22, ler sensores, receber comandos Serial, selecionar modo, comandar o relé e registrar estados. |
| [controller.h](firmware/irrigation/controller.h) | Manter a demanda de irrigação e decidir a bomba com histerese, previsão, secura crítica, falhas e timeout. Não acessa GPIOs. |
| [alerts.h](firmware/irrigation/alerts.h) | Converter ADC em pH didático e avaliar alertas de NPK/pH, independentemente do acionamento. |
| [weather_command.h](firmware/irrigation/weather_command.h) | Delimitar linhas Serial, validar JSON com ArduinoJson e aceitar ou rejeitar comandos e previsões. |
| [diagram.json](firmware/irrigation/diagram.json) | Descrever componentes e conexões elétricas no Wokwi. |
| [libraries.txt](firmware/irrigation/libraries.txt) | Fixar as bibliotecas DHTesp e ArduinoJson do sketch. |
| [wokwi.toml](firmware/irrigation/wokwi.toml) | Apontar o Wokwi para os arquivos compilados do ESP32. |
| [circuit.test.yaml](firmware/irrigation/circuit.test.yaml) | Automatizar ações e verificações do circuito na nuvem. |

### Ferramentas e testes

| Arquivo | Responsabilidade |
| --- | --- |
| [simulator.html](tools/simulator.html) | Interface com entradas, cenários, circuito lógico, reprodução, histórico e exportação JSON. |
| [simulator.py](tools/simulator.py) | Servir o painel em loopback, validar entradas, montar sequências e executar o CLI C++; também intermediar consultas meteorológicas e o executor Wokwi. |
| [simulator_cli.cpp](tools/simulator_cli.cpp) | Executar as regras C++ com entradas de terminal e emitir um resultado JSON por avaliação, sem emular o ESP32. |
| [weather.py](tools/weather.py) | Consultar Open-Meteo, selecionar seis intervalos completos futuros em UTC e emitir um comando meteorológico validado. |
| [test-circuit.mjs](tools/test-circuit.mjs) | Executar a suíte Wokwi, carregar credenciais localmente e verificar os registros Serial. |
| [controller_test.cpp](tests/controller_test.cpp) | Verificar histerese, chuva, secura crítica, expiração, falhas, timeout e rollover de tempo. |
| [alerts_test.cpp](tests/alerts_test.cpp) | Verificar combinações NPK, conversão ADC, limites e entradas inválidas de pH. |
| [weather_command_test.cpp](tests/weather_command_test.cpp) | Verificar o parser JSON, comandos, previsões antigas e limites do buffer Serial. |
| [test_simulator.py](tests/test_simulator.py) | Verificar cenários do executor local, entradas e integração do backend. |
| [test_weather.py](tests/test_weather.py) | Verificar contrato meteorológico, séries horárias, erros e tentativas HTTP. |

### Fluxo dos dados

No Wokwi: sensores → sketch → controlador/alertas → relé e Monitor Serial. O retorno no GPIO19 permite comparar o comando com o contato do relé.

No painel: navegador → backend Python → CLI C++ → resultados JSON → reprodução no navegador. As decisões são calculadas antes da animação; aumentar a velocidade não altera os tempos usados pelas regras.

Na integração climática: Open-Meteo → Python → JSON → cópia manual no Serial → parser C++ → previsão do controlador. Não existe conexão automática entre o painel e o ESP32.

## Regras de operação

| Condição | Comportamento automático |
| --- | --- |
| Inicialização sem leitura válida | Bomba desligada. |
| Umidade ≤ 40 | Ativa a demanda. |
| Umidade ≥ 60 | Encerra a demanda. |
| Umidade entre 40 e 60 | Preserva a demanda anterior; primeira leitura 50 mantém OFF. |
| Previsão válida com precipitação ≥ 2 mm e máxima probabilidade horária ≥ 60% | Suspende a bomba, preservando a demanda. Os dois critérios são necessários. |
| Umidade ≤ 25 | Ativa exceção crítica e ignora a suspensão climática até atingir 40. Não ignora proteções. |
| Umidade inválida ou mais de 7,5 s sem leitura válida | Desliga e limpa demanda e exceção crítica. |
| Bomba ligada continuamente por 60 s | Desliga e bloqueia até reset ou reinicialização do controlador. |
| Previsão ausente ou expirada | Usa a umidade sem suspensão climática; não presume chuva zero. |

O firmware amostra os sensores a cada 2,5 segundos. Temperatura é registrada, mas não participa da decisão. NPK insuficiente e pH fora de 6,0–6,5 geram alertas, sem impedir irrigação necessária. A conversão didática é `limitar((ADC - 32) × 14 / 4031, 0, 14)`; ADC fora de 0–4095 é inválido.

Os limiares de umidade, chuva e tempo são parâmetros didáticos, não recomendações agronômicas ou dimensionamento de segurança para uma bomba real. Consulte a [pesquisa da cultura](docs/cultura-tomate.md) para as referências e suas limitações.

## Executar o painel local

Requisitos: Python 3, `g++` com suporte a C++11 e navegador. Não são necessários pacotes Python externos, token ou internet para cenários locais. Execute todos os comandos deste guia na raiz do repositório.

```sh
python3 -m tools.simulator
```

Abra <http://127.0.0.1:8765>. Em VS Code remoto, encaminhe a porta 8765. Se estiver ocupada:

```sh
python3 -m tools.simulator --port 8766
```

O backend compila automaticamente o CLI na pasta de build. Selecione **Histerese 35 / 50 / 65 / 50**, use clima **Sem previsão** e execute: os estados esperados são ON / ON / OFF / OFF. Também estão disponíveis chuva, seca crítica, expiração, falhas, timeout/reset, teste manual e combinações de alertas.

Pausa, continuação e avanço permitem inspecionar os resultados. Cada execução começa com um novo controlador. O download exporta todos os resultados calculados, inclusive os ainda não reproduzidos. Encerre o servidor com `Ctrl+C`.

O painel desenha um circuito lógico: não emula sensores, GPIOs, transmissão Serial ou contatos elétricos. A bomba não aumenta a umidade automaticamente. O botão de suíte do circuito é uma execução separada na nuvem, requer credenciais e consome quota; não testa os inputs atuais do painel.

## Executar o ESP32 no Wokwi

1. Abra um [novo projeto ESP32 no Wokwi](https://wokwi.com/projects/new/esp32).
2. Copie o [diagrama](firmware/irrigation/diagram.json) para a aba `diagram.json` e o [sketch](firmware/irrigation/irrigation.ino) para `sketch.ino`.
3. Inclua os três headers do firmware: [alerts.h](firmware/irrigation/alerts.h), [controller.h](firmware/irrigation/controller.h) e [weather_command.h](firmware/irrigation/weather_command.h).
4. Copie o [manifesto de bibliotecas](firmware/irrigation/libraries.txt) para `libraries.txt`.
5. Inicie a simulação e aguarde a primeira leitura, após aproximadamente 2,5 segundos. O Serial usa 115200 baud; configure a entrada com quebra de linha LF.

O firmware inicia em **diagnóstico**, com o relé desligado. Envie `1` para um teste manual de até cinco segundos e `0` para desligar. Esses comandos saem do modo automático; repetir `1` durante um teste não renova sua duração. O teste manual é independente das proteções de irrigação automática.

Para ativar o modo automático, envie uma linha:

```json
{"type":"auto","schema_version":1}
```

A resposta deve incluir `ACK AUTO`. Ajuste a umidade do DHT22 para 35 e depois para 65, sem previsão bloqueante, e observe os registros `CONTROLE`. Compare também `RELE_COMANDO` com `CONTATO_NO`, considerando o intervalo de amostragem.

Para liberar o bloqueio por tempo máximo, envie:

```json
{"type":"reset","schema_version":1}
```

O reset permite reavaliar as condições, mas não obriga a bomba a ligar. Para remover a previsão:

```json
{"type":"weather_clear","schema_version":1}
```

Botão N/P/K pressionado significa nutriente adequado; solto significa insuficiente. Use Ctrl+clique para manter botões pressionados. Ao alterar NPK, ajuste também o LDR manualmente, conforme o enunciado: o firmware não calcula pH a partir dos botões. Na configuração anteriormente validada, 140 lux produziu pH simulado 6,158.

A pinagem, a alimentação, a polaridade observada do relé e os detalhes elétricos estão no [guia do circuito](docs/circuito-wokwi.md). Ao abrir no navegador, use os arquivos atuais e todos os headers acima; o guia antigo descreve uma versão anterior de diagnóstico.

## Consultar a previsão meteorológica

Requisitos: Python 3 e internet. O Open-Meteo não exige chave para esta consulta; respeite os termos e limites do serviço.

```sh
python3 tools/weather.py --latitude=-23.55 --longitude=-46.63
```

As coordenadas são um exemplo de São Paulo, não uma fazenda identificada. Substitua-as pelo local desejado. Copie o JSON de saída e envie imediatamente no Serial Wokwi com LF. A aceitação gera `ACK WEATHER`; ative o modo automático separadamente se necessário.

O JSON informa versão do contrato, instante da consulta, início/fim da janela UTC, precipitação acumulada, máxima probabilidade horária e validade de 3600 segundos. A máxima probabilidade horária não é a probabilidade acumulada das seis horas. O script faz no máximo duas tentativas HTTP, com timeout de dez segundos por tentativa.

O painel também oferece consulta à API e disponibiliza o comando para copiar. Uma falha da API no painel remove a previsão anterior. No uso manual, se a consulta falhar, nenhum comando válido será emitido: use `weather_clear` se desejar remover uma previsão ainda ativa no ESP32.

Sem relógio confiável no ESP32, a validade é contada desde o recebimento. O parser rejeita mensagens com timestamp menor ou igual ao último aceito, mas não identifica a primeira mensagem antiga após reinício apenas pelo timestamp. Consulte antes de enviar; não reutilize previsões salvas como atuais.

## Executar testes locais

A suíte Python testa previsão e simulação local, compilando o CLI quando necessário. Não aciona a nuvem:

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

Para testar diretamente as regras C++:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/irrigation tests/controller_test.cpp -o /tmp/farmtech-controller-test
/tmp/farmtech-controller-test
g++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/irrigation tests/alerts_test.cpp -o /tmp/farmtech-alerts-test
/tmp/farmtech-alerts-test
```

O teste do parser requer os headers ArduinoJson 7.4.2. Após instalar a biblioteca pelo Arduino CLI, ajuste o caminho se o diretório de bibliotecas não for o padrão:

```sh
ARDUINOJSON_INCLUDE="$HOME/Arduino/libraries/ArduinoJson/src"
g++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/irrigation -I"$ARDUINOJSON_INCLUDE" tests/weather_command_test.cpp -o /tmp/farmtech-weather-command-test
/tmp/farmtech-weather-command-test
```

Sucesso nos testes locais comprova os comportamentos cobertos, não a montagem elétrica nem toda a integração do ESP32.

## Compilar e testar o circuito na nuvem

Requisitos adicionais: Arduino CLI, Node.js 22 ou posterior, Wokwi CLI, internet e token Wokwi CI. O ambiente local de referência usa core ESP32 2.0.17, DHTesp 1.19.0 e ArduinoJson 7.4.2.

```sh
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@2.0.17 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "DHT sensor library for ESPx@1.19.0"
arduino-cli lib install "ArduinoJson@7.4.2"
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all --output-dir firmware/irrigation/build firmware/irrigation
wokwi-cli lint --warnings-as-errors firmware/irrigation/diagram.json
node tools/test-circuit.mjs
```

Antes do último comando, configure `WOKWI_CLI_TOKEN` ou `WOKWI_API_KEY` no ambiente ou no arquivo local `.env` da raiz. Não versionar, imprimir ou compartilhar credenciais. O executor usa a nuvem e consome a quota da conta; o lint apenas verifica o diagrama. O registro Serial é gravado na pasta de build.

Não declare a suíte completa aprovada caso ela termine por timeout ou erro. A compilação no navegador também precisa ser conferida, pois o core do Wokwi pode diferir do ambiente local.

## Documentação complementar

| Documento | Conteúdo |
| --- | --- |
| [Enunciado](CONTEXT.md) | Objetivo, componentes exigidos, extensões opcionais e entregáveis. |
| [Cultura do tomateiro](docs/cultura-tomate.md) | Pesquisa de pH, nutrientes, água e limites das referências. |
| [Especificação](docs/especificacao-simulacao.md) | Regras propostas na etapa inicial e justificativas. |
| [Cenários de validação](docs/cenarios-validacao.md) | Matriz de resultados esperados e rastreabilidade da etapa inicial. |
| [Circuito Wokwi](docs/circuito-wokwi.md) | Montagem, pinagem e evidências históricas de diagnóstico. |
| [Simulador local](docs/simulador-local.md) | Operação detalhada do painel, cenários e verificações da interface. |

Para concluir a entrega, o grupo ainda deve registrar imagens reais do circuito, o link do projeto Wokwi salvo e o vídeo não listado no YouTube. Não há evidência de economia real de água ou aumento de produtividade em campo.