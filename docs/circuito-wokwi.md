# Circuito Wokwi: montagem e diagnóstico

A etapa 2 disponibiliza o circuito do ESP32 e um sketch de diagnóstico. Não há irrigação automática nesta versão: NPK, LDR e DHT22 são lidos para conferir as conexões; o relé só recebe comandos manuais de teste.

## Arquivos do projeto

- [diagram.json](../firmware/irrigation/diagram.json): componentes, conexões e Monitor Serial.
- [irrigation.ino](../firmware/irrigation/irrigation.ino): sketch de diagnóstico em C/C++.
- [libraries.txt](../firmware/irrigation/libraries.txt): biblioteca DHT sensor library for ESPx, versão 1.19 (resolvida como 1.19.0 na compilação local).

O nome do sketch coincide com a pasta `irrigation` para permitir compilação pela ferramenta Arduino. No navegador, o conteúdo desse sketch deve ocupar a aba `sketch.ino` do projeto Wokwi.

## Pinagem e alimentação

| Componente / sinal | GPIO no código | Nome no diagrama | Ligação complementar |
| --- | --- | --- | --- |
| Botão verde N, contato 1 | 25 | D25 | Contato 2 no GND; entrada com pull-up interno |
| Botão verde P, contato 1 | 26 | D26 | Contato 2 no GND; entrada com pull-up interno |
| Botão verde K, contato 1 | 27 | D27 | Contato 2 no GND; entrada com pull-up interno |
| LDR, saída AO | 34 | D34 | VCC em 3,3 V e GND comum; DO não utilizado |
| DHT22, SDA | 16 | RX2 | VCC em 3,3 V, GND comum e pull-up de 10 kΩ entre SDA e 3,3 V |
| Relé, entrada IN | 18 | D18 | VCC em VIN (5 V no modelo), GND comum e pull-up de 10 kΩ de IN para 3,3 V |
| Retorno do contato COM | 19 | D19 | Entrada com pull-down interno, apenas para diagnóstico |

Embora a placa rotule o GPIO16 como RX2, ele é usado como GPIO para o DHT22, não como uma segunda porta Serial. TX0/RX0 continuam ligados ao Monitor Serial.

O contato NO do relé recebe 3,3 V. COM alimenta um resistor de 330 Ω e o ânodo de um LED azul; o cátodo vai ao GND. Quando COM/NO fecha, o LED representa a bomba ligada e o GPIO19 permite observar esse contato. NC fica sem ligação.

O módulo de relé padrão do Wokwi representa o relé azul solicitado. O LED adicional não substitui o relé: apenas torna visível o circuito comutado. Não existe motor, bomba real ou rede elétrica nesta montagem.

## Abrir no navegador

1. Abra o [novo projeto Arduino ESP32 no Wokwi](https://wokwi.com/projects/new/esp32).
2. Carregue o conteúdo dos arquivos locais nas abas correspondentes: `diagram.json`, `sketch.ino` e `libraries.txt`. Se necessário, adicione a biblioteca pelo Library Manager e selecione a versão indicada no manifesto.
3. Inicie a simulação. O Monitor Serial deve abrir automaticamente, configurado para entrada com quebra de linha LF.
4. Aguarde pelo menos 2,5 segundos para a primeira amostragem. O cenário inicial define umidade 50, temperatura 24 °C e os três botões soltos.
5. Após a validação, salve o projeto na sua conta Wokwi e registre seu endereço e as capturas do circuito. Ainda não há um projeto publicado ou imagens de execução neste repositório.

A compilação no navegador precisa ser conferida separadamente: sua versão do core ESP32 pode diferir da versão testada localmente.

## Leituras do diagnóstico

A cada 2,5 segundos, o sketch registra N, P, K, `ADC_LDR`, `UMIDADE_INDICE`, temperatura, comando do relé e estado observado do contato NO.

- NPK: `1` significa botão pressionado; `0`, botão solto. São leituras de diagnóstico, ainda sem o tratamento de repiques da futura lógica de controle.
- LDR: `ADC_LDR` é a leitura bruta de um ADC configurado em 12 bits, com atenuação `ADC_11db`. Não é pH. O valor numérico pertence à escala 0–4095, mas os extremos realmente alcançados precisam ser medidos.
- DHT22: leitura inválida de umidade ou temperatura aparece como `ERRO`, não como zero. A umidade continua sendo uma representação didática, não uma medida real de água no solo.
- Relé: `RELE_COMANDO` indica a intenção do sketch; `CONTATO_NO` indica o retorno lido no GPIO19. Não tratar o comando como prova de que o contato fechou.

## Roteiro de teste no simulador

| Teste | Ação | Resultado esperado | Situação |
| --- | --- | --- | --- |
| Inicialização | Iniciar sem enviar comandos | Comando OFF, contato aberto e LED apagado | Pendente |
| Botão N | Manter N pressionado e aguardar uma amostragem | Somente N passa para 1 | Pendente |
| Botão P | Manter P pressionado e aguardar uma amostragem | Somente P passa para 1 | Pendente |
| Botão K | Manter K pressionado e aguardar uma amostragem | Somente K passa para 1 | Pendente |
| Combinações | Executar as oito combinações da documentação | Leituras correspondem aos botões pressionados | Pendente |
| DHT22 | Ajustar o controle de umidade para 0, 50 e 100 | Índice acompanha cada valor após nova amostragem | Pendente |
| Temperatura | Ajustar o controle do DHT22 para outro valor | Temperatura registrada acompanha o controle | Pendente |
| LDR | Variar a iluminação entre extremos e ponto intermediário | ADC muda; registrar valores e direção antes de converter para pH | Pendente |
| Relé ON | Enviar `1` no Monitor Serial | Contato fechado e LED aceso por até cinco segundos | Pendente |
| Relé OFF | Durante o teste, enviar `0` | Contato aberto e LED apagado | Pendente |
| Limite do teste | Enviar `1` e não enviar mais comandos | Comando volta a OFF ao completar cinco segundos | Pendente |
| Independência | Alterar umidade/NPK/LDR sem comando Serial | Não ligar automaticamente o relé | Pendente |

Com foco no diagrama, as teclas N, P e K controlam os botões. Para manter vários pressionados, use Ctrl+clique em cada botão; um novo clique o libera. Esse recurso é documentado pelo [Wokwi](https://docs.wokwi.com/parts/wokwi-pushbutton#stickiness).

O comando `1` não renova a duração quando o teste já está ativo. Após o desligamento, um novo comando pode iniciar outro teste. Essa é uma ferramenta de bancada do simulador, não o controle de uma bomba real; erros de sensor não bloqueiam esse acionamento manual.

## Polaridade do relé: confirmação obrigatória

O diagrama fixa o atributo `transistor` como `npn`. O sketch adota provisoriamente **HIGH para OFF** e **LOW para ON**, conforme a descrição de operação dos contatos na [referência do relé](https://docs.wokwi.com/parts/wokwi-relay-module). A tabela de atributos dessa página descreve a polaridade de outra forma; por isso, o teste do contato é obrigatório.

Se o LED iniciar aceso ou se o retorno do contato divergir do comando, interrompa a simulação. A polaridade ainda não estará validada: confira a ligação NO/COM e ajuste as constantes `RELAY_OFF_LEVEL` e `RELAY_ON_LEVEL` somente após observar o comportamento. Não continue para o controle automático com essa divergência.

O pull-up externo busca manter IN em nível alto durante a inicialização. Isso não constitui garantia de segurança para um módulo físico ou evidência de ausência de pulsos no simulador.

## Verificações realizadas

Em 8 de outubro de 2026:

- Diagrama analisado como JSON: 11 componentes e 26 conexões.
- Lint oficial Wokwi CLI **0.28.1** executado com `--warnings-as-errors`: sem erros ou avisos. Há uma informação de que o tipo DevKit v1 não possui documentação própria e pode mudar futuramente.
- Sketch compilado com Arduino CLI **1.5.2-rc.1**, core **esp32:esp32 2.0.17**, alvo **esp32:esp32:esp32** e DHTesp **1.19.0**. Uso reportado: 270741 bytes de programa e 21648 bytes de memória global.
- Simulação dinâmica não executada nesta sessão: Wokwi CI requer uma credencial não disponível. Os testes acima, a calibração do LDR e as capturas continuam pendentes; o Wokwi do navegador é a alternativa prevista.

Com ferramentas instaladas, a verificação pode ser reproduzida a partir da raiz do repositório:

```sh
wokwi-cli lint --warnings-as-errors firmware/irrigation/diagram.json
arduino-cli core update-index --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli core install esp32:esp32@2.0.17 --additional-urls https://espressif.github.io/arduino-esp32/package_esp32_index.json
arduino-cli lib install "DHT sensor library for ESPx@1.19.0"
arduino-cli compile --fqbn esp32:esp32:esp32 --warnings all firmware/irrigation
```

Lint e compilação não comprovam leitura correta dos sensores, aparência do circuito ou polaridade do relé. A implementação das regras de irrigação da [especificação](especificacao-simulacao.md) será a próxima etapa, após a conferência manual desta montagem. Os [cenários do sistema](cenarios-validacao.md) permanecem pendentes.