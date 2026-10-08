# Especificação proposta da simulação

Esta especificação traduz o [enunciado](../CONTEXT.md) e a [pesquisa do tomateiro](cultura-tomate.md) em regras verificáveis. **As regras de irrigação abaixo continuam propostas, ainda sem controle automático implementado.** O [circuito e o sketch de diagnóstico](circuito-wokwi.md) já implementam conversão didática de pH e alertas NPK/pH, verificados localmente e na nuvem; isso não valida as regras de irrigação abaixo.

## Componentes e significado das entradas

| Requisito | Componente no Wokwi | Significado didático |
| --- | --- | --- |
| Nitrogênio | Botão verde N | Pressionado: adequado (`true`); solto: insuficiente (`false`) |
| Fósforo | Botão verde P | Pressionado: adequado (`true`); solto: insuficiente (`false`) |
| Potássio | Botão verde K | Pressionado: adequado (`true`); solto: insuficiente (`false`) |
| pH do solo | Saída analógica AO do LDR | Valor convertido para pH artificial de 0 a 14 |
| Umidade do solo | DHT22 | Umidade relativa do ar usada como índice simulado de 0 a 100 |
| Bomba de irrigação | Relé azul | Estado lógico de irrigação ligada ou desligada |
| Processamento | ESP32 com C/C++ | Leitura, decisão, acionamento e registro dos estados |

Os botões representarão estados momentâneos, sem alternância automática a cada clique. Para demonstrar combinações, será necessário manter os botões desejados pressionados; o Wokwi permite Ctrl+clique para mantê-los pressionados. O futuro controle deverá tratar repiques sem alterar esse significado; o sketch de diagnóstico ainda mostra as leituras brutas.

O relé deve iniciar desligado. Para o componente `npn` desta montagem, o teste do contato confirmou LOW para OFF e HIGH para ON. A pinagem, a alimentação e os resultados estão registrados no [guia do circuito](circuito-wokwi.md). Mudanças no modelo exigem nova verificação; os testes não comprovam ausência de pulsos antes da primeira amostra.

## Conversão do LDR

A leitura analógica é convertida pela função [simulatedPhFromAdc](../firmware/irrigation/alerts.h), usando os pontos observados ADC 32 e 4063 para pH 0 e 14, respectivamente. A fórmula é `limitar((ADC - 32) * 14 / 4031, 0, 14)`. A configuração é ADC de 12 bits com atenuação `ADC_11db`; mais iluminação produz menor pH simulado neste circuito. ADC fora de 0–4095 é rejeitado, não convertido em um pH válido.

Essa configuração é uma calibração **do cenário simulado**, não uma calibração química. A variação por unidade ADC é aproximadamente 0,00347 na região não limitada. O Serial apresenta três casas decimais, mas a comparação usa o valor antes do arredondamento. Os limites exatos 6,0 e 6,5 passaram nos testes da função C++; em 140 lux, a execução na nuvem registrou pH 6,158 dentro da faixa. Uma alteração no ADC ou na montagem exige nova verificação dos pontos de referência.

## Parâmetros iniciais

| Parâmetro | Valor proposto | Natureza |
| --- | --- | --- |
| pH de referência | 6,0 a 6,5, inclusive | Referência da pesquisa, com limitações regionais |
| Início da demanda de irrigação | Índice de umidade ≤ 40 | Parâmetro didático |
| Encerramento da demanda | Índice de umidade ≥ 60 | Parâmetro didático |
| Secura crítica | Índice de umidade ≤ 25 | Parâmetro didático para a extensão climática |
| Intervalo de leitura do DHT22 | 2,5 segundos | Proposta técnica a validar |
| Leitura de umidade desatualizada | Mais de 7,5 segundos sem sucesso | Proposta técnica a validar |
| Tempo máximo ligado continuamente | 60 segundos de simulação | Proteção didática, não dimensionamento de bomba real |

O índice de umidade não é teor de água do solo. Nenhum desses limiares de umidade foi obtido como recomendação agronômica das fontes consultadas.

## Regras do controle básico

1. Na inicialização, manter a bomba desligada até receber uma leitura válida. Uma primeira leitura entre 40 e 60 mantém a demanda desligada.
2. Com leitura válida ≤ 40, ativar a demanda de irrigação. Com leitura válida ≥ 60, encerrar a demanda.
3. Entre os dois limites, conservar a demanda anterior. Essa histerese evita alternar o relé quando o índice oscila perto de um único limite.
4. Sem bloqueios de segurança ou climáticos, o estado da bomba acompanha a demanda.
5. Uma leitura ausente, não finita ou fora de 0 a 100 desliga a bomba imediatamente. Uma leitura desatualizada também bloqueia o acionamento. Nunca interpretar erro de leitura como umidade zero.
6. Uma falha limpa a demanda e o estado de secura crítica. Após recuperação, uma leitura nova reavalia a demanda; no intervalo intermediário, a bomba permanece desligada.
7. Ao completar 60 segundos ligada continuamente, desligar e manter um bloqueio por tempo máximo. O bloqueio exige reinicialização ou comando explícito de reset, cujo formato será definido no firmware. O reset apenas permite reavaliar as condições; não ordena ligar a bomba.

## Alertas nutricionais e de pH

Cada botão solto gera `N_INSUFICIENTE`, `P_INSUFICIENTE` ou `K_INSUFICIENTE`. Um pH válido abaixo de 6,0 ou acima de 6,5 gera `PH_FORA_FAIXA`; os limites pertencem à faixa adotada. Valores não finitos ou fora de 0–14 geram `PH_INVALIDO`, não um alerta de faixa agronômica. Essas verificações estão implementadas e testadas. Sem alertas, o Serial registra `ALERTAS=NENHUM`.

Esses alertas já são registrados junto ao estado do relé e **não bloqueiam seu teste manual**, como confirmado na nuvem. No futuro controle automático, também não deverão impedir uma irrigação necessária. Trata-se de uma decisão do grupo: uma deficiência nutricional não elimina a necessidade de água e irrigar não resolve, por si só, a deficiência ou o pH inadequado. Falhas de sensor e outros bloqueios de segurança serão tratados separadamente na etapa de controle.

Quando o demonstrador alterar NPK, deverá também ajustar o LDR para representar uma mudança de cenário, conforme o enunciado. O firmware não deverá inventar um pH a partir dos botões nem substituir a leitura do LDR.

## Extensões previstas

**Python e previsão meteorológica:** consultar uma API pública e transferir o resultado ao ESP32 pelo Monitor Serial, manualmente, como permitido pelo enunciado. A proposta inicial é suspender a bomba quando uma previsão válida apresentar precipitação acumulada ≥ 2 mm e máxima probabilidade horária ≥ 60% em seis intervalos horários completos futuros. Esses limites são didáticos; a máxima probabilidade horária não é a probabilidade de chuva de toda a janela.

A extensão deverá informar validade e expiração. Dados ausentes, inválidos ou expirados não significam previsão de zero chuva: removem o bloqueio climático e deixam o controle básico operar. O formato da mensagem, as regras de seleção temporal e o prazo de validade serão especificados na etapa de integração.

**Secura crítica:** uma leitura ≤ 25 deverá ignorar o bloqueio climático, mas não falhas de sensor ou bloqueio por tempo máximo. Essa condição permanecerá ativa até a umidade alcançar 40, inclusive, evitando oscilações perto de 25. Ao alcançar 40, termina a exceção climática; a demanda básica só termina ao alcançar 60.

**R e análise estatística:** analisar registros de cenários, calcular estatísticas descritivas, tempo de acionamento e frequência de comutação, e comparar pares de limiares. Uma eventual recomendação deverá ser revisada e validada antes de atualizar o firmware. Manter os valores iniciais será aceitável se não houver melhoria demonstrada. Não serão prometidas previsões de produtividade ou economia real de água.

Com a extensão climática, a prioridade proposta será: **falha de sensor ou bloqueio por tempo máximo → exceção de secura crítica → bloqueio climático → demanda por histerese**. Um bloqueio climático não apaga a demanda básica, permitindo retomada quando a previsão deixar de bloquear.

## Operação e limites

O acionamento do relé não altera automaticamente a umidade do DHT22. O demonstrador deverá movimentar o controle do sensor para representar secagem ou irrigação. Não haverá modelo físico de solo, dosagem de fertilizante, bomba real ou ligação à rede elétrica.

Antes de implementar as extensões, será necessário definir o formato dos registros e das mensagens. Os [cenários de validação](cenarios-validacao.md) registram os comportamentos esperados; não são resultados de testes já realizados.

## Referências técnicas

- [ESP32 no Wokwi](https://docs.wokwi.com/guides/esp32).
- [DHT22 no Wokwi](https://docs.wokwi.com/parts/wokwi-dht22): controles de temperatura e umidade; recomenda a biblioteca DHT sensor library for ESPx no ESP32.
- [LDR no Wokwi](https://docs.wokwi.com/parts/wokwi-photoresistor-sensor): leitura da saída analógica AO.
- [Relé no Wokwi](https://docs.wokwi.com/parts/wokwi-relay-module): contatos e configuração; verificar a polaridade na montagem, pois as descrições da tabela e da operação não são consistentes entre si.
- [Monitor Serial do Wokwi](https://docs.wokwi.com/guides/serial-monitor): entrada de texto para o programa.
- [Open-Meteo](https://open-meteo.com/en/docs): possível API pública para uso educacional não comercial, respeitando seus termos e limites.

Referências consultadas em 8 de outubro de 2026.