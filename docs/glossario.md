# Glossário: componentes, mensagens e resultados

Este guia explica os termos do projeto em linguagem simples. Pode ser consultado durante o ensaio do [roteiro do vídeo](roteiro.md).

## 1. Como tudo se conecta

**Sensores e botões → ESP32 → decisão do programa → relé → indicação da bomba.**

Os sensores fornecem informações; o programa aplica as regras; o relé liga ou desliga o circuito que representa a bomba. No Wokwi, não existe uma bomba real: usamos um relé e um LED azul para representar seu acionamento.

No painel local, executamos o mesmo controlador C++, mas o desenho e o contato são lógicos. Não existe emulação elétrica dos componentes nesse painel.

## 2. Componentes e termos de eletrônica

| Termo | O que é | Como usamos no projeto |
| --- | --- | --- |
| **ESP32** | Microcontrolador: um pequeno computador que executa o programa e se comunica com componentes | Lê as entradas, decide a irrigação e controla o relé |
| **LDR** | *Light Dependent Resistor*: resistor cuja resistência muda conforme a luz | Sua leitura é convertida em **pH simulado**; ele não mede acidez |
| **DHT22** | Sensor digital de temperatura e umidade relativa **do ar** | A umidade é usada como índice didático representando a condição do solo |
| **Relé** | Interruptor controlado eletricamente | Representa o acionamento da bomba d'água |
| **LED** | Pequeno componente que emite luz quando percorrido por corrente adequada | O LED azul indica o circuito comutado pelo relé |
| **NPK** | Nitrogênio, fósforo e potássio: nutrientes importantes para as plantas | Três botões representam adequado ou insuficiente; não medem concentração |
| **GPIO** | Pino de entrada/saída do microcontrolador | Conecta o programa aos botões, sensores e relé |
| **ADC** | Conversor analógico-digital: transforma uma tensão em um número | Converte o sinal do LDR em uma leitura de 0 a 4095 nesta configuração de 12 bits |
| **AO** | *Analog Output*: saída analógica de um módulo | É a saída do LDR conectada ao GPIO34 |
| **DO** | *Digital Output*: saída digital de um módulo | A saída DO do módulo LDR não é utilizada no projeto |
| **VCC / VIN** | Nomes de conexões de alimentação elétrica, cujo significado exato depende do componente | Alimentam os módulos com a tensão prevista na montagem |
| **GND** | Referência elétrica comum, normalmente chamada de terra do circuito | Permite que os componentes compartilhem a mesma referência de tensão |
| **HIGH / LOW** | Nível lógico alto ou baixo em um pino | Neste relé configurado como `npn`, HIGH liga e LOW desliga; isso não é universal para todo módulo |
| **Pull-up / pull-down** | Configuração com resistor que mantém uma entrada em um nível definido quando ninguém a aciona | Evita que o pino fique sem referência definida; botões usam pull-up e o retorno do contato usa pull-down |

**Analógico** significa que o sinal pode variar dentro de uma faixa. **Digital** significa que o sinal usa níveis discretos; um sensor digital como o DHT22 também pode transmitir números com casas decimais, e não apenas “ligado ou desligado”.

### Por que botão pressionado vira `1` se o pino lê LOW?

Com o pull-up, o botão solto deixa o pino em HIGH; pressioná-lo conecta o pino ao GND, produzindo LOW. O programa interpreta **LOW como botão pressionado** e publica `1`, significando nutriente adequado.

Portanto, `N=1` não significa “tensão alta no pino N”. Significa “o programa reconheceu N como adequado”.

## 3. Relé: COM, NO, NC e contato

| Nome | Significado | Comportamento básico |
| --- | --- | --- |
| **COM** | Contato comum | Terminal que é comutado entre os outros contatos |
| **NO** | *Normally Open*: normalmente aberto | Sem acionamento, não forma uma conexão condutiva com COM; ao acionar, fecha |
| **NC** | *Normally Closed*: normalmente fechado | Sem acionamento, está conectado a COM; ao acionar, abre. Não é utilizado nesta montagem |

“Normalmente” se refere ao estado não acionado do relé. **NO é uma sigla em inglês, não a palavra portuguesa “não”.**

### `RELE_COMANDO` versus `CONTATO_NO`

- `RELE_COMANDO`: o que o programa **mandou** o relé fazer.
- `CONTATO_NO`: o estado **observado do circuito COM/NO**, inferido pela leitura de retorno no GPIO19 no Wokwi.

| Saída | Significado |
| --- | --- |
| `RELE_COMANDO=ON` | O programa enviou comando de ligar |
| `RELE_COMANDO=OFF` | O programa enviou comando de desligar |
| `CONTATO_NO=FECHADO` | O retorno indica que o caminho COM/NO está conduzindo |
| `CONTATO_NO=ABERTO` | O retorno indica que esse caminho não está conduzindo |

**Fechado permite passagem de corrente; aberto interrompe.** É o mesmo princípio de um interruptor: circuito fechado liga a carga, circuito aberto desliga.

Na montagem esperada, ON combina com FECHADO e OFF com ABERTO. Uma divergência persistente exige verificar ligação, polaridade ou simulação. A leitura de contato não comprova vazão de água, funcionamento de uma bomba real ou ausência de pulsos entre amostras.

**No painel local, o contato é calculado a partir do comando.** Ali, FECHADO não é uma segunda evidência elétrica independente.

## 4. Campos das leituras no Monitor Serial

O **Monitor Serial** é a janela de comunicação textual com o ESP32: nela aparecem resultados e nela digitamos comandos. As leituras do sketch são emitidas aproximadamente a cada 2,5 segundos.

| Campo | Valores possíveis nesta implementação | Como interpretar |
| --- | --- | --- |
| `N`, `P`, `K` | `0` ou `1` | 0 = botão solto/nutriente insuficiente; 1 = pressionado/adequado |
| `ADC_LDR` | Inteiro de `0` a `4095` | Leitura bruta do LDR; não é pH diretamente |
| `UMIDADE_INDICE` | Número de `0.0` a `100.0`, ou `ERRO` | Índice didático de umidade, ou leitura inválida |
| `TEMPERATURA_C` | Número em graus Celsius, ou `ERRO` | Temperatura lida; não altera a decisão de irrigação |
| `RELE_COMANDO` | `ON` ou `OFF` | Comando aplicado ao relé |
| `CONTATO_NO` | `FECHADO` ou `ABERTO` | Estado do retorno do contato no circuito Wokwi |
| `PH_SIMULADO` | Número de `0.000` a `14.000`, ou `ERRO` | Conversão didática do ADC; não é medição química |
| `ALERTAS` | `NENHUM` ou lista de alertas separados por vírgula | Condições nutricionais/de pH que precisam de atenção |

**`ERRO` não é zero.** Por exemplo, umidade `0.0` é uma leitura válida muito seca no modelo; `ERRO` significa que não há leitura válida para usar como umidade.

### pH e lux

- **pH:** escala de acidez/alcalinidade; abaixo de 7 é ácido, próximo de 7 é neutro e acima de 7 é alcalino nas condições usuais. Adotamos a faixa de referência **6,0 a 6,5**, incluindo os extremos, para o tomateiro.
- **Lux:** unidade de iluminância, isto é, a quantidade de luz que chega a uma superfície. O ajuste de lux no Wokwi muda o sinal do LDR.
- Na nossa configuração, mais luz tende a reduzir o ADC e o pH simulado. Essa relação é uma escolha da simulação, não uma relação entre luz e acidez do solo.

Exemplo: ADC `1805` produz pH simulado aproximadamente `6.158`, dentro da faixa. ADC `1001` produz aproximadamente `3.365`, fora da faixa. A fórmula está documentada no [guia do circuito](circuito-wokwi.md).

## 5. Alertas possíveis

| Alerta | Tradução prática |
| --- | --- |
| `N_INSUFICIENTE` | Nitrogênio representado como insuficiente |
| `P_INSUFICIENTE` | Fósforo representado como insuficiente |
| `K_INSUFICIENTE` | Potássio representado como insuficiente |
| `PH_FORA_FAIXA` | O pH é válido, mas está abaixo de 6,0 ou acima de 6,5 |
| `PH_INVALIDO` | Não existe um pH válido: por exemplo, valor não finito ou fora da escala 0–14 no avaliador |
| `NENHUM` | Não há alertas nutricionais ou de pH nessa leitura |

`NENHUM` **não** significa que todo o sistema está livre de falhas: o sensor de umidade pode estar com problema, por exemplo. Ele se refere somente a essa lista de alertas.

Vários alertas podem aparecer juntos. `PH_FORA_FAIXA` e `PH_INVALIDO` são situações diferentes: um pH de 3 é válido, mas inadequado para a faixa adotada; uma entrada inválida não deve ser tratada como pH 3 nem pH zero.

Esses alertas **não bloqueiam a bomba** por si só. Regar não corrige automaticamente acidez nem falta de fertilizante. Leituras de LDR desconectado também não são necessariamente reconhecidas como inválidas: elas podem permanecer na faixa numérica do ADC.

## 6. Linha `CONTROLE`: modo, clima, bomba e motivo

Exemplo ilustrativo:

```text
CONTROLE MODO=AUTO CLIMA=VALIDO BOMBA=OFF MOTIVO=RAIN_HOLD
```

Tradução: “Estamos no modo automático, há previsão válida e a bomba está desligada porque a previsão de chuva suspendeu a irrigação.”

| Campo | Valores | Significado |
| --- | --- | --- |
| `MODO` | `AUTO` / `DIAGNOSTICO` | Controle automático / teste manual de bancada |
| `CLIMA` | `VALIDO` / `DESCONHECIDO` | Previsão presente e dentro da validade / previsão ausente, removida ou expirada |
| `BOMBA` | `ON` / `OFF` | Estado comandado para a representação da bomba |
| `MOTIVO` | Um dos códigos abaixo | Por que o controlador está nesse estado |

**Clima válido não significa “vai chover”.** Uma previsão válida pode indicar zero chuva. **Clima desconhecido não significa “não vai chover”.** Significa que não podemos usar uma previsão válida nessa decisão.

### Todos os motivos possíveis

| `MOTIVO` | Significado | Bomba |
| --- | --- | --- |
| `MOISTURE_IDLE` | Não existe demanda de irrigação pelo histórico de umidade | OFF |
| `MOISTURE_DEMAND` | Existe demanda por umidade e não há suspensão meteorológica aplicável | ON |
| `CRITICAL_DRY` | A seca crítica está ativa e tem prioridade sobre a suspensão por chuva | ON |
| `RAIN_HOLD` | Existe demanda, mas uma previsão válida de chuva suspende a irrigação | OFF |
| `SENSOR_FAULT` | Sensor inválido, ainda sem leitura válida ou última leitura válida com mais de 7,5 segundos | OFF |
| `TIMEOUT` | A bomba atingiu o limite de 60 segundos contínuos; bloqueio permanece até reset | OFF |
| `TESTE_MANUAL` | O programa está em diagnóstico, não em decisão automática | ON ou OFF, conforme o teste |

`MOISTURE` significa umidade; `DEMAND`, demanda; `IDLE`, sem demanda; `CRITICAL_DRY`, seca crítica; `RAIN_HOLD`, suspensão por chuva; `SENSOR_FAULT`, falha de sensor; `TIMEOUT`, limite de tempo atingido.

### Histerese: por que umidade 50 pode ligar ou desligar?

**Histerese** é usar dois limites e lembrar o estado anterior, evitando ligar/desligar repetidamente perto de um único limite.

- Em **40 ou menos**, nasce a demanda por água.
- Em **60 ou mais**, a demanda é encerrada.
- Entre **40 e 60**, a demanda anterior é mantida.
- Ao iniciar com 50, sem demanda anterior, a bomba permanece desligada.

Assim, `35 → 50 → 65 → 50`, sem chuva nem falhas, produz **ON → ON → OFF → OFF**. Uma previsão de chuva pode suspender a bomba mesmo que a demanda continue ativa: histerese conserva a demanda, não força a bomba a ficar ligada.

**Exceção crítica:** umidade de 25 ou menos ativa a prioridade sobre a chuva. Essa prioridade permanece até alcançar 40. Por isso, após passar por 25, uma leitura de 30 ainda pode apresentar `CRITICAL_DRY`.

**Prioridade de segurança:** falha de sensor e timeout impedem o acionamento automático mesmo com seca crítica.

## 7. Clima, API e JSON

| Termo ou campo | Significado |
| --- | --- |
| **API** | Interface pela qual um programa solicita dados a outro serviço; usamos a API pública Open-Meteo |
| **JSON** | Formato de texto organizado em campos e valores, usado para transferir a previsão |
| `type` | Tipo da mensagem; `weather` indica uma previsão |
| `schema_version` | Versão do formato de mensagem; atualmente `1` |
| `fetched_at_epoch_s` | Instante da consulta, expresso como timestamp Unix em segundos |
| `window_start_epoch_s` / `window_end_epoch_s` | Início e fim da janela futura de seis horas selecionada |
| `precip_mm` | Soma da precipitação prevista nessa janela, em milímetros |
| `max_hourly_probability_pct` | Maior probabilidade horária de precipitação na janela, em porcentagem |
| `ttl_s` | *Time To Live*: prazo de validade da previsão em segundos, de 1 a 3600 |
| **UTC** | Referência de tempo usada para evitar ambiguidades entre fusos horários |
| **Timestamp Unix / epoch** | Número de segundos desde 1º de janeiro de 1970, usado para representar instantes |

No JSON, `true` significa verdadeiro, `false` significa falso e `null` indica ausência de valor. Um campo `null` não significa número zero.

Exemplo de interpretação, **não um comando completo para colar**:

```text
precip_mm = 2
max_hourly_probability_pct = 60
ttl_s = 10
```

Significa “dois milímetros previstos na janela; a maior probabilidade de uma hora é 60%; validade de dez segundos”. Não significa que a chance acumulada de chuva nas seis horas seja 60%.

A suspensão exige **ambos**: chuva de pelo menos 2 mm **e** máxima probabilidade horária de pelo menos 60%, com previsão ainda válida. Sem previsão válida, vale a regra de umidade; previsão ausente não é convertida em chuva zero.

No ESP32 sem relógio confiável, o TTL começa no recebimento. Ele não prova que a primeira mensagem colada após um reinício foi consultada recentemente. No painel local, também descontamos a idade real da consulta antes de iniciar a simulação.

## 8. Comandos e respostas do Serial

**ACK** vem de *acknowledgment*: confirmação de que o programa aceitou um comando. Não significa automaticamente “bomba ligada”.

| O que enviar | Resposta principal | Efeito |
| --- | --- | --- |
| `1` | `RELE: comando ON por no maximo 5 segundos.` | Entra em diagnóstico e testa o relé por até cinco segundos |
| `0` | `RELE: comando OFF enviado; confira contato e LED.` | Entra em diagnóstico e desliga o relé |
| `{"type":"auto","schema_version":1}` | `ACK AUTO` | Ativa o controle automático; a decisão depende das entradas |
| JSON completo produzido pelo Python, com `type=weather` | `ACK WEATHER fetched_at=... ttl_s=...` | Aceita e armazena a previsão |
| `{"type":"weather_clear","schema_version":1}` | `ACK WEATHER_CLEAR` | Remove a previsão ativa |
| `{"type":"reset","schema_version":1}` | `ACK RESET` | Libera o bloqueio de timeout; não liga a bomba incondicionalmente |

Os comandos JSON precisam terminar com **LF**, a quebra de linha `\n`. **CRLF** é a combinação `\r\n`; o receptor também tolera esse terminador. O buffer aceita até 511 caracteres antes do LF.

O teste manual não aplica as proteções do modo automático. Repetir `1` enquanto o teste já está ativo não estende seus cinco segundos. Colar uma previsão, por si só, não ativa o modo automático.

### Mensagens de erro possíveis

| Mensagem | O que ocorreu | O que fazer |
| --- | --- | --- |
| `ERRO JSON_INVALIDO` | JSON malformado, campos/tipos incorretos, versão não suportada ou valores incompatíveis | Use o comando completo no formato esperado |
| `ERRO WEATHER_ANTIGO` | Timestamp da consulta menor ou igual ao último aceito nessa execução | Faça uma nova consulta; não repita a mesma previsão |
| `ERRO LINHA_LONGA` | A linha ultrapassou o buffer permitido | Envie um comando menor e completo em uma nova linha |

Uma mensagem rejeitada **não substitui nem renova** a previsão válida que já estava armazenada. `WEATHER_ANTIGO` compara timestamps entre mensagens aceitas; não é uma checagem independente da hora atual do mundo real. A consulta Python pode exibir outros diagnósticos de rede/validação no terminal, em vez de produzir uma previsão utilizável.

## 9. Exemplo completo de leitura

Exemplo ilustrativo de um estado estável:

```text
N=0 P=1 K=1 ADC_LDR=1001 UMIDADE_INDICE=35.0 TEMPERATURA_C=24.0 RELE_COMANDO=ON CONTATO_NO=FECHADO PH_SIMULADO=3.365 ALERTAS=N_INSUFICIENTE,PH_FORA_FAIXA
CONTROLE MODO=AUTO CLIMA=DESCONHECIDO BOMBA=ON MOTIVO=MOISTURE_DEMAND
```

Leia assim:

1. N está insuficiente; P e K estão adequados.
2. O ADC do LDR é 1001, convertido em pH simulado 3,365, fora da faixa adotada.
3. O índice de umidade é 35, e a temperatura lida é 24 °C.
4. O programa mandou ligar o relé e o retorno do Wokwi indica contato fechado.
5. Não existe previsão utilizável; a demanda por umidade mantém a bomba ligada.
6. Os alertas continuam visíveis; ligar a irrigação não os resolve automaticamente.

**Detalhe de timing:** o sketch imprime a linha de leituras antes de atualizar o controle com a nova amostra. A linha `CONTROLE` seguinte já reflete essa atualização. Durante uma transição, o estado de relé na primeira linha pode ainda ser o anterior; confira a próxima amostra e não conclua imediatamente que houve falha.

## 10. Termos do painel e dos testes

| Termo ou campo | Tradução prática |
| --- | --- |
| **CLI** | Programa executado por comandos no terminal |
| **Firmware / sketch** | Programa C/C++ que roda no ESP32 |
| **Compilar** | Transformar o código-fonte em um programa executável |
| **Suíte de testes** | Conjunto de testes automatizados executados em sequência |
| **Teste unitário** | Verifica uma função ou regra isolada |
| **Teste de integração** | Verifica partes funcionando juntas |
| **Lint** | Análise de estrutura/configuração; não equivale a executar o circuito |
| **Fixture / dados sintéticos** | Entradas preparadas para reproduzir um caso; não são uma consulta real à API |
| **Telemetry / telemetria** | Dados publicados pelo sistema para observar seu funcionamento |
| `uptime_ms` | Tempo desde o início da execução, em milissegundos; 1000 ms = 1 s |
| `pump_on` | `true` = bomba comandada ligada; `false` = desligada |
| `contact` | `FECHADO` / `ABERTO`; no executor local, é calculado |
| `reason` | Código do motivo da decisão, equivalente a `MOTIVO` |
| `weather_status` | `VALIDO` / `DESCONHECIDO`, equivalente a `CLIMA` |
| `sensor_valid` | Se a umidade está utilizável para o controlador; inclui validade/atraso no automático local |
| `ph` | pH simulado numérico; `null` se não houver valor válido na saída do CLI |
| `alerts` | Lista dos alertas; `[]` é uma lista vazia, equivalente à ausência de alertas |
| **5× / 10× / 25×** | Velocidade de reprodução do painel; não muda os tempos das regras |

Um resultado `OK` da suíte significa que os casos executados passaram. Não comprova desempenho de uma lavoura real nem substitui validação elétrica. Consulte o [painel local](simulador-local.md) para reproduzir os cenários e o [roteiro](roteiro.md) para apresentá-los no vídeo.