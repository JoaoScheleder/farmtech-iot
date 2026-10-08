# Circuito Wokwi: montagem e diagnóstico

A etapa 2 disponibiliza o circuito do ESP32 e um sketch de diagnóstico, agora com alertas de nutrientes e pH. Não há irrigação automática nesta versão: NPK, LDR e DHT22 são lidos e os alertas aparecem no Monitor Serial; o relé só recebe comandos manuais de teste.

## Arquivos do projeto

- [diagram.json](../firmware/irrigation/diagram.json): componentes, conexões e Monitor Serial.
- [irrigation.ino](../firmware/irrigation/irrigation.ino): sketch de diagnóstico em C/C++.
- [alerts.h](../firmware/irrigation/alerts.h): conversão do ADC e avaliação dos alertas, sem dependência do hardware.
- [alerts_test.cpp](../tests/alerts_test.cpp): testes locais da conversão e dos limites.
- [libraries.txt](../firmware/irrigation/libraries.txt): biblioteca DHT sensor library for ESPx, versão 1.19 (resolvida como 1.19.0 na compilação local).
- [wokwi.toml](../firmware/irrigation/wokwi.toml): firmware compilado utilizado pelo simulador local/CI.
- [circuit.test.yaml](../firmware/irrigation/circuit.test.yaml): cenário automatizado de sensores, botões e relé.
- [test-circuit.mjs](../tools/test-circuit.mjs): executor que carrega a credencial local e verifica o registro Serial, sem exibir o token.

O nome do sketch coincide com a pasta `irrigation` para permitir compilação pela ferramenta Arduino. No navegador, o conteúdo desse sketch deve ocupar a aba `sketch.ino` do projeto Wokwi.

## Pinagem e alimentação

| Componente / sinal | GPIO no código | Nome no diagrama | Ligação complementar |
| --- | --- | --- | --- |
| Botão verde N, contato 1 | 25 | D25 | Contato 2 no GND; entrada com pull-up interno |
| Botão verde P, contato 1 | 26 | D26 | Contato 2 no GND; entrada com pull-up interno |
| Botão verde K, contato 1 | 27 | D27 | Contato 2 no GND; entrada com pull-up interno |
| LDR, saída AO | 34 | D34 | VCC em 3,3 V e GND comum; DO não utilizado |
| DHT22, SDA | 16 | RX2 | VCC em 3,3 V, GND comum e pull-up de 10 kΩ entre SDA e 3,3 V |
| Relé, entrada IN | 18 | D18 | VCC em VIN (5 V no modelo), GND comum e pull-down de 10 kΩ de IN para GND |
| Retorno do contato COM | 19 | D19 | Entrada com pull-down interno, apenas para diagnóstico |

Embora a placa rotule o GPIO16 como RX2, ele é usado como GPIO para o DHT22, não como uma segunda porta Serial. TX0/RX0 continuam ligados ao Monitor Serial.

O contato NO do relé recebe 3,3 V. COM alimenta um resistor de 330 Ω e o ânodo de um LED azul; o cátodo vai ao GND. Quando COM/NO fecha, o LED representa a bomba ligada e o GPIO19 permite observar esse contato. NC fica sem ligação.

O módulo de relé padrão do Wokwi representa o relé azul solicitado. O LED adicional não substitui o relé: apenas torna visível o circuito comutado. Não existe motor, bomba real ou rede elétrica nesta montagem.

## Abrir no navegador

1. Abra o [novo projeto Arduino ESP32 no Wokwi](https://wokwi.com/projects/new/esp32).
2. Carregue o conteúdo dos arquivos locais nas abas correspondentes: `diagram.json`, `sketch.ino` e `libraries.txt`. Inclua também [alerts.h](../firmware/irrigation/alerts.h) como arquivo do projeto. Se necessário, adicione a biblioteca pelo Library Manager e selecione a versão indicada no manifesto.
3. Inicie a simulação. O Monitor Serial deve abrir automaticamente, configurado para entrada com quebra de linha LF.
4. Aguarde pelo menos 2,5 segundos para a primeira amostragem. O cenário inicial define umidade 50, temperatura 24 °C e os três botões soltos.
5. Após a validação, salve o projeto na sua conta Wokwi e registre seu endereço e as capturas do circuito. Ainda não há um projeto publicado ou imagens de execução neste repositório.

A compilação no navegador precisa ser conferida separadamente: sua versão do core ESP32 pode diferir da versão testada localmente.

## Leituras do diagnóstico

A cada 2,5 segundos, o sketch registra N, P, K, `ADC_LDR`, `UMIDADE_INDICE`, temperatura, comando do relé, estado observado do contato NO, `PH_SIMULADO` e `ALERTAS`.

- NPK: `1` significa botão pressionado; `0`, botão solto. São leituras de diagnóstico, ainda sem o tratamento de repiques da futura lógica de controle.
- LDR: `ADC_LDR` é a leitura bruta de um ADC configurado em 12 bits, com atenuação `ADC_11db`. Não é pH. `PH_SIMULADO` aplica uma conversão didática dos pontos medidos para 0–14, conforme a seção de alertas abaixo.
- DHT22: leitura inválida de umidade ou temperatura aparece como `ERRO`, não como zero. A umidade continua sendo uma representação didática, não uma medida real de água no solo.
- Relé: `RELE_COMANDO` indica a intenção do sketch; `CONTATO_NO` indica o retorno lido no GPIO19. Não tratar o comando como prova de que o contato fechou.

## Alertas de nutrientes e pH

A conversão implementada é `pH_simulado = limitar((ADC - 32) * 14 / (4063 - 32), 0, 14)`. ADC 32 corresponde a pH 0; ADC 4063, a pH 14. Valores ADC válidos além desses pontos são limitados à escala. Mais luz reduz o ADC e o pH simulado nesta configuração; isso não representa uma relação química entre iluminação e acidez.

O Serial exibe três casas decimais. Os alertas usam o valor calculado antes do arredondamento, e a faixa aceita é **6,0–6,5, inclusive**. A variação por unidade de ADC é aproximadamente 0,00347; os limites exatos são testados na função C++, pois nem todo valor de pH é alcançável por um ADC inteiro.

| Estado | Alerta exibido |
| --- | --- |
| N solto | `N_INSUFICIENTE` |
| P solto | `P_INSUFICIENTE` |
| K solto | `K_INSUFICIENTE` |
| pH válido abaixo de 6,0 ou acima de 6,5 | `PH_FORA_FAIXA` |
| Entrada de pH inválida | `PH_INVALIDO` |
| Nenhuma condição acima | `NENHUM` |

Os alertas ativos aparecem juntos, separados por vírgula, na ordem N, P, K e pH. A conversão rejeita ADC fora de 0–4095; o avaliador rejeita pH não finito ou fora de 0–14. Essas entradas são verificadas em testes locais: não se presume que um LDR desconectado seja detectável, pois sua leitura pode continuar dentro da escala ADC.

Para demonstrar o caso sem alertas, mantenha N, P e K pressionados e ajuste o LDR para **140 lux**. Na execução validada, isso produziu ADC **1805**, pH simulado **6,158** e `ALERTAS=NENHUM`. Em 500 lux, pH **3,365** gera `PH_FORA_FAIXA`; os alertas dos botões soltos continuam sendo exibidos independentemente do LDR.

Nenhum alerta interfere nos comandos manuais do relé. O cenário confirmou contato fechado com os três nutrientes insuficientes e pH simulado 0. Essa independência não substitui os futuros bloqueios de segurança da irrigação automática.

## Roteiro de teste no simulador

| Teste | Ação | Resultado esperado | Situação |
| --- | --- | --- | --- |
| Inicialização | Iniciar sem enviar comandos | Comando OFF, contato aberto na primeira amostra | Aprovado via CLI |
| Botão N | Manter N pressionado e aguardar uma amostragem | Somente N passa para 1 | Aprovado via CLI |
| Botão P | Manter P pressionado e aguardar uma amostragem | Somente P passa para 1 | Aprovado via CLI |
| Botão K | Manter K pressionado e aguardar uma amostragem | Somente K passa para 1 | Aprovado via CLI |
| Combinações | Executar as oito combinações da documentação | Leituras correspondem aos botões pressionados | Aprovado via CLI |
| DHT22 | Ajustar o controle de umidade para 0, 50 e 100 | Índice acompanha cada valor após nova amostragem | Aprovado via CLI |
| Temperatura | Ajustar para 21, 22 e 23 °C, além dos 24 °C iniciais | Temperatura registrada acompanha o controle | Aprovado via CLI |
| LDR | Ajustar para 0,1, 500 e 100000 lux | ADC diminui com aumento da iluminação | Aprovado via CLI |
| Relé ON | Enviar `1` no Monitor Serial | Contato fechado | Aprovado via retorno GPIO19 |
| Relé OFF | Durante o teste, enviar `0` | Contato aberto | Aprovado via retorno GPIO19 |
| Limite do teste | Enviar `1` e não enviar mais comandos | Desligamento automático, programado para cinco segundos | Aprovado via CLI |
| Independência | Alterar umidade/NPK/LDR sem comando Serial | Relé permanece desligado nas amostras | Aprovado via CLI |
| Alertas NPK | Executar as oito combinações | Apenas nutrientes soltos geram seus alertas | Aprovado localmente e via CLI |
| pH e conversão | Testar extremos no Wokwi e limites 6,0/6,5 na função C++ | Conversão coerente; limites inclusivos sem alerta | Aprovado nos testes correspondentes |
| Sem alertas | NPK pressionados e LDR em 140 lux | `PH_SIMULADO=6.158 ALERTAS=NENHUM` | Aprovado via CLI |
| Relé com alertas | Enviar `1` com NPK soltos e pH fora da faixa | Contato fecha; alertas permanecem visíveis | Aprovado via CLI |
| Visual | Capturar o circuito e o LED nos estados ON/OFF | Aparência e sinalização visual coerentes | Pendente |

Os testes verificam estados nas amostras e mensagens do cenário. Não medem pulsos durante a inicialização nem certificam o instante exato de comutação. A aprovação elétrica do contato não substitui a captura visual do LED.

Com foco no diagrama, as teclas N, P e K controlam os botões. Para manter vários pressionados, use Ctrl+clique em cada botão; um novo clique o libera. Esse recurso é documentado pelo [Wokwi](https://docs.wokwi.com/parts/wokwi-pushbutton#stickiness).

O comando `1` não renova a duração quando o teste já está ativo. Após o desligamento, um novo comando pode iniciar outro teste. Essa é uma ferramenta de bancada do simulador, não o controle de uma bomba real; erros de sensor não bloqueiam esse acionamento manual.

## Polaridade do relé: resultado observado

O diagrama fixa o atributo `transistor` como `npn`. O teste na Wokwi Simulation API **1.0.0-20261006-g9494a200** confirmou **LOW para OFF** e **HIGH para ON**, por leitura do contato no GPIO19. A configuração provisória anterior estava invertida: o primeiro teste encontrou contato fechado durante o comando OFF. As constantes e o resistor externo foram corrigidos, e o cenário passou após recompilação.

Esse resultado corresponde à tabela de atributos, mas diverge da descrição textual dos contatos na [referência do relé](https://docs.wokwi.com/parts/wokwi-relay-module). Em caso de mudança de componente ou versão do simulador, repita o teste e interrompa a execução se comando e contato divergirem.

O pull-down externo busca manter IN em nível baixo durante a inicialização. Isso não constitui garantia de segurança para um módulo físico ou evidência de ausência de pulsos no simulador.

## Verificações realizadas

Em 8 de outubro de 2026:

- Diagrama analisado como JSON: 11 componentes e 26 conexões.
- Lint oficial Wokwi CLI **0.28.1** executado com `--warnings-as-errors`: sem erros ou avisos. Há uma informação de que o tipo DevKit v1 não possui documentação própria e pode mudar futuramente.
- Sketch com alertas compilado com Arduino CLI **1.5.2-rc.1**, core **esp32:esp32 2.0.17**, alvo **esp32:esp32:esp32** e DHTesp **1.19.0**. Uso reportado: 271517 bytes de programa e 21648 bytes de memória global.
- Cenário completo executado na nuvem: `Scenario completed successfully`. A checagem local dos registros também passou, incluindo oito combinações NPK, umidades 0/50/100 e contato coerente com cada comando.
- Leituras do LDR: **4063 em 0,1 lux**, **1001 em 500 lux** e **32 em 100000 lux**. São pontos medidos nesta configuração, não extremos universais do ADC nem medições de pH.
- Um primeiro cenário completo foi interrompido. Uma execução seguinte passou na nuvem, mas revelou um defeito no parser local: o log pode agrupar registros sem quebra de linha. O parser foi corrigido e o cenário completo repetido; somente a execução final foi aprovada em ambas as verificações.
- Testes C++ dos alertas aprovados: oito combinações NPK, limites 6,0/6,5, valores fora da faixa, entradas inválidas e monotonicidade da conversão em toda a escala ADC.
- Cenário ampliado na nuvem e verificador local aprovados: pH calculado corresponde ao ADC, alertas correspondem às entradas, há um caso sem alertas e o relé opera com alertas simultâneos. O parser delimita os alertas mesmo quando o Wokwi concatena registros.
- Capturas visuais e cenários de irrigação automática continuam pendentes. A conversão de pH é exclusivamente didática e deverá ser reavaliada se a configuração do ADC ou do circuito mudar.

## Executar os testes sem navegador

É necessário Node.js 22 ou posterior, Wokwi CLI instalado e um token do painel Wokwi CI. O executor aceita `WOKWI_CLI_TOKEN` ou `WOKWI_API_KEY` no ambiente ou no arquivo `.env` da raiz, mapeando a chave para o nome exigido pelo CLI apenas no processo filho. Não imprima a chave e não a inclua no Git.

O cenário usa o simulador na nuvem e consome a cota da conta. O executor transmite o progresso em tempo real, limita cada execução a 65 segundos simulados e impõe um limite de quatro minutos de tempo real. Uma interrupção também encerra o processo filho. O registro é gerado em `firmware/irrigation/build/circuit-serial.log`, dentro da pasta ignorada pelo Git.

Com ferramentas instaladas, a verificação pode ser reproduzida a partir da raiz do repositório:

```sh
g++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/irrigation tests/alerts_test.cpp -o /tmp/farmtech-alerts-test
/tmp/farmtech-alerts-test
wokwi-cli lint --warnings-as-errors firmware/irrigation/diagram.json
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@2.0.17 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "DHT sensor library for ESPx@1.19.0"
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all --output-dir firmware/irrigation/build firmware/irrigation
node tools/test-circuit.mjs
```

O teste valida a montagem de diagnóstico, não as regras de irrigação da [especificação](especificacao-simulacao.md). Os [cenários do sistema](cenarios-validacao.md) permanecem pendentes. A apresentação visual e as imagens para entrega ainda precisam ser conferidas separadamente.