# Painel local de demonstração

## Executar

Na raiz do repositório, com Python 3 e `g++` disponíveis:

```sh
python3 -m tools.simulator
```

Abra <http://127.0.0.1:8765>. O servidor fica restrito ao endereço de loopback. Em uma sessão remota do VS Code, encaminhe a porta 8765. Para outra porta, use `--port 8766`.

Não é necessário instalar pacotes Python, configurar token ou acessar a internet para a simulação local. O backend compila o [CLI C++](../tools/simulator_cli.cpp) no diretório de build ignorado e o executa como subprocesso. As decisões e os alertas usam diretamente [controller.h](../firmware/irrigation/controller.h) e [alerts.h](../firmware/irrigation/alerts.h), sem reimplementar suas regras em Python.

## Entradas e execução

- Umidade: índice didático 0–100; não é umidade volumétrica do solo.
- Temperatura: entrada exibida no circuito lógico, sem influência na irrigação.
- LDR: ADC 0–4095, convertido em pH simulado com a mesma fórmula do firmware.
- N/P/K: adequado ou insuficiente. Alertas não suspendem a irrigação.
- Sensor: válido, falha injetada ou ausência de novas leituras.
- Controle automático: histerese 40/60, exceção crítica 25 até recuperação em 40, falha/atraso de sensor e timeout contínuo de 60 segundos.
- Controle manual: teste de relé limitado a cinco segundos, independente dos alertas e da previsão, como o diagnóstico do firmware.
- Clima: sem previsão, previsão sintética ou consulta real ao Open-Meteo.

Escolha um cenário ou ajuste as entradas e use **Executar**. A execução começa com um controlador novo; a pausa mantém o histórico e **Continuar** retoma a reprodução. O avanço permite inspecionar uma leitura de cada vez. Alterar entradas invalida a reprodução pendente; a próxima execução começa novamente. O reinício da visualização não envia comandos a hardware.

O controlador é executado antes da animação; a tela reproduz seus resultados. Leituras e avaliações locais têm intervalo de 2,5 segundos. O fator de velocidade altera somente a reprodução, nunca os timestamps usados nas decisões. Cada etapa dos presets inclui suas leituras inicial e final; a etapa seguinte começa no próximo tick. Por isso, a duração total pode exceder a soma das durações nominais das etapas. A duração configurável aplica-se apenas a **Entradas atuais**.

O painel exibe bomba/relé, contato lógico NO, previsão, motivo, pH, alertas e histórico. O botão de download exporta todos os resultados calculados, inclusive leituras ainda não reproduzidas, com entradas e origem do clima.

## Cenários reproduzíveis

| Cenário | Resultado principal |
| --- | --- |
| Entradas atuais | Executa a configuração escolhida |
| Histerese 35 / 50 / 65 / 50 | ON / ON / OFF / OFF |
| Chuva | 2 mm e máxima horária de 60% suspendem a bomba |
| Seca crítica e recuperação | 25 e 30 ignoram a suspensão; em 40 a chuva volta a suspender |
| Previsão expirada | Após cinco segundos, volta ao controle por umidade |
| Sem previsão | A demanda por umidade controla a bomba |
| Falha de sensor e recuperação | Desliga na falha; reavalia após leitura válida |
| Sensor sem novas leituras | Desliga quando a última leitura válida tem mais de 7,5 segundos |
| Limite de 60 s e reset | Desliga e trava; reset explícito na etapa seguinte permite reavaliação |
| Teste manual de 5 s | Desliga ao atingir cinco segundos |
| Oito combinações NPK e pH | Mostra deficiências individuais e caso final sem alertas |

Os presets preenchem os inputs iniciais e, quando necessário, uma previsão sintética ou ausente. A fonte de clima pode ser alterada depois: essa escolha é respeitada na execução. As etapas do cenário continuam alterando os sensores programados; para usar somente inputs fixos, selecione **Entradas atuais**. Um reset de timeout é demonstrado pelo cenário correspondente ou por uma nova execução.

## Open-Meteo

Selecione a API e configure latitude/longitude. As coordenadas iniciais `-23.55, -46.63` são apenas um exemplo na região de São Paulo, não uma fazenda identificada.

O backend executa o [CLI meteorológico](../tools/weather.py), valida seu JSON e disponibiliza o comando para copiar no Serial Wokwi. Uma nova execução no modo API consulta novamente o serviço. Cada consulta realiza no máximo duas tentativas HTTP. A probabilidade exibida é a maior probabilidade **horária**, não a probabilidade acumulada de seis horas.

A validade restante local desconta o tempo decorrido desde a consulta, além de expirar com o tempo simulado. Resposta ausente ou já expirada deixa o clima desconhecido. Falha de API remove a previsão anterior, sem fabricar chuva zero, e a irrigação continua pelas regras de umidade. Use previsões sintéticas para demonstrar chuva determinística: a previsão real depende das condições nas coordenadas consultadas.

## Wokwi e limites

O painel local **não emula o ESP32, o DHT22, o ADC, a transmissão Serial ou os contatos elétricos**. Seu desenho é um circuito lógico, e o contato exibido é calculado a partir do comando da bomba. A umidade não aumenta automaticamente quando a bomba liga.

O botão **Executar suíte do circuito** chama o executor existente `node tools/test-circuit.mjs`, que usa Wokwi **na nuvem**, requer firmware compilado e token no ambiente ou no arquivo local de credenciais e consome quota. Ele executa a suíte fixa de [circuit.test.yaml](../firmware/irrigation/circuit.test.yaml), não os inputs atuais do painel. Uma confirmação antecede a execução. O log e o código de saída ficam disponíveis ao término; não se pode iniciar uma segunda suíte enquanto a primeira está ativa. Credenciais não são enviadas ao navegador.

Consulte o [guia do circuito](circuito-wokwi.md) para compilação e testes elétricos. A implementação do painel não alterou o cenário existente e não repetiu a suíte na nuvem. O timeout observado na execução anterior dessa suíte permanece uma limitação de validação independente.

## Verificação

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
```

Os testes locais cobrem histerese, chuva, exceção crítica, expiração, previsão ausente, falha e atraso de sensor, timeout/reset, manual, NPK/pH, inputs inválidos, duração e alterações explícitas da fonte de clima. O CLI é compilado com C++11 e warnings tratados como erros.

Em 08/10/2026, os 11 cenários também foram executados pela interface em Chromium. Foram verificados pausa/continuação, avanço, exportação JSON, consulta real ao Open-Meteo e fallback com erro de API injetado. O canvas foi confirmado não vazio e os layouts de 390, 768 e 1440 pixels não apresentaram overflow horizontal.