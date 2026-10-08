# Cenários de validação planejados

Os casos abaixo verificam a [especificação proposta](especificacao-simulacao.md), fundamentada na [pesquisa do tomateiro](cultura-tomate.md). **Nenhum cenário funcional foi executado até o momento.** Os resultados são expectativas, não evidências de funcionamento. O [guia do circuito](circuito-wokwi.md) registra o lint, a compilação do diagnóstico e o roteiro manual da etapa 2; esses checks não validam as regras de irrigação.

## Preparação

Na futura montagem, confirmar entradas independentes, conversão do LDR e estado real dos contatos do relé. Para os casos básicos, usar pH 6,2, NPK adequados e nenhuma previsão bloqueante, salvo indicação diferente. Reiniciar entre casos independentes. Sequências dentro de um mesmo caso preservam o histórico da demanda.

Registrar entradas, estado anterior, resultado observado, alertas, versão do firmware e evidência. Aplicar leituras por tempo suficiente para uma nova amostragem e o tratamento dos botões; não presumir resposta instantânea do DHT22.

## Controle básico

| ID | Entrada ou sequência | Resultado esperado | Situação |
| --- | --- | --- | --- |
| B01 | Inicializar, antes de qualquer leitura válida | Bomba desligada | Pendente |
| B02 | Primeira leitura válida: 50 | Bomba permanece desligada | Pendente |
| B03 | Umidade 35 → 50 → 65 → 50 | Ligada → ligada → desligada → desligada | Pendente |
| B04 | Umidade 40 → 59 → 60 | Ligada → ligada → desligada | Pendente |
| B05 | Após demanda desligada, umidade 41 → 40 | Desligada → ligada | Pendente |
| B06 | Umidade válida 0; depois 100 | Ligada; depois desligada, sem confundir extremos com erros | Pendente |
| B07 | Umidade 35; alternar os botões conforme a tabela NPK | Bomba ligada; alertas acompanham cada combinação | Pendente |
| B08 | Umidade 35; pH 5,9 → 6,0 → 6,5 → 6,6 | Bomba ligada; alerta de pH apenas em 5,9 e 6,6 | Pendente |
| B09 | Alterar NPK e ajustar o LDR manualmente | Estados e pH registrados separadamente; sem pH calculado a partir dos botões | Pendente |
| B10 | Acionar o relé sem mover o controle de umidade | Nenhuma mudança automática de umidade causada pela bomba | Pendente |

Os valores exatos de pH serão usados nos testes da lógica. Na demonstração pelo LDR, registrar os valores efetivamente alcançados e a tolerância da conversão.

## Combinações de NPK

`1` significa botão pressionado e condição adequada; `0`, botão solto e condição insuficiente. Com umidade válida 35, sem bloqueios e antes do tempo máximo, todas as combinações devem manter a bomba ligada.

| N | P | K | Alertas esperados | Situação |
| --- | --- | --- | --- | --- |
| 0 | 0 | 0 | N, P e K insuficientes | Pendente |
| 0 | 0 | 1 | N e P insuficientes | Pendente |
| 0 | 1 | 0 | N e K insuficientes | Pendente |
| 0 | 1 | 1 | N insuficiente | Pendente |
| 1 | 0 | 0 | P e K insuficientes | Pendente |
| 1 | 0 | 1 | P insuficiente | Pendente |
| 1 | 1 | 0 | K insuficiente | Pendente |
| 1 | 1 | 1 | Nenhum alerta de NPK | Pendente |

## Segurança e recuperação

| ID | Condição | Resultado esperado | Situação |
| --- | --- | --- | --- |
| S01 | Bomba ligada; leitura ausente ou não finita | Desligar imediatamente, informar falha e limpar a demanda | Pendente |
| S02 | Injetar leitura de umidade -1 ou 101 no teste da lógica | Rejeitar a leitura e manter bomba desligada | Pendente |
| S03 | Mais de 7,5 segundos desde a última leitura bem-sucedida | Bomba desligada e leitura considerada desatualizada | Pendente |
| S04 | Após falha, nova leitura válida 50; depois 35 | Desligada em 50; ligada em 35 | Pendente |
| S05 | Umidade 35 constante por 60 segundos de acionamento contínuo | Desligar ao atingir o limite e bloquear novo acionamento | Pendente |
| S06 | Após S05, manter umidade 35 sem reset | Bomba permanece desligada | Pendente |
| S07 | Reset explícito com falha de sensor; depois leitura válida 35 | Reset não liga a bomba com falha; leitura válida permite nova decisão | Pendente |
| S08 | Reiniciar com umidade 50 | Bomba desligada, sem restaurar demanda anterior | Pendente |

Falhas que não possam ser produzidas pela interface do Wokwi serão injetadas nos testes da lógica ou em uma configuração isolada de teste. Registrar esse método: uma falha injetada não comprova o comportamento de uma desconexão física.

## Extensões futuras

| ID | Condição | Resultado esperado | Situação |
| --- | --- | --- | --- |
| E01 | Umidade 35; previsão válida com 2 mm acumulados e máxima probabilidade horária 60% | Bomba desligada por bloqueio climático; demanda preservada | Pendente da integração Python |
| E02 | Umidade 35; previsão com 1,9 mm e 60%, ou 2 mm e 59% | Sem bloqueio climático; bomba ligada | Pendente da integração Python |
| E03 | Umidade 35; previsão bloqueante expira ou é removida | Retomar controle básico e ligar a bomba | Pendente da integração Python |
| E04 | Sem previsão disponível, umidade 35 | Controle básico ativo; estado climático desconhecido, não zero chuva | Pendente da integração Python |
| E05 | Com previsão bloqueante, umidade 25 → 30 → 40 | Ligada → ligada → desligada; exceção crítica termina em 40 | Pendente da integração Python |
| E06 | Secura crítica junto a falha de sensor ou bloqueio por tempo máximo | Bomba desligada; segurança prevalece | Pendente da integração Python |
| E07 | Mensagem climática incompleta, inválida ou excessivamente longa | Rejeitar sem atualizar validade; processamento continua | Pendente do contrato de mensagens |
| E08 | Registros de diferentes execuções, incluindo falhas e lacunas | R separa execuções e não conta intervalos desconhecidos como tempo contínuo ligado | Pendente da análise em R |
| E09 | Comparação de pares de limiares sem melhoria demonstrada | R recomenda manter os valores iniciais 40/60 | Pendente da análise em R |

## Rastreabilidade da entrega

| Exigência do enunciado | Preparação nesta etapa | Evidência ainda necessária |
| --- | --- | --- |
| Escolher uma cultura e pesquisar suas necessidades | Pesquisa do tomateiro com referências | Revisão crítica pelo grupo |
| Três botões verdes para NPK | Semântica e oito combinações descritas | Circuito e testes |
| LDR representando pH | Faixa de referência e conversão proposta | Configuração e valores observados |
| DHT22 representando umidade | Índice didático e limiares documentados | Leituras e transições registradas |
| Relé azul representando bomba | Regras de acionamento e segurança descritas | Contatos verificados e demonstração |
| Documentar lógica e especificidades | Especificação e resultados esperados | README final e resultados executados |
| Código C/C++ e imagens no GitHub | Fora desta etapa | Firmware, arquivos do circuito e imagens reais |
| Integração Python opcional | Comportamento previsto | Script, contrato de mensagens e demonstração |
| Análise em R opcional | Objetivo e limites definidos | Script, dados identificados e relatório |
| Vídeo não listado de até cinco minutos | Fora desta etapa | Publicação no YouTube e link na documentação |

## Critério de conclusão da etapa 1

A documentação deve ter referências rastreáveis, links locais válidos, parâmetros consistentes e distinção explícita entre pesquisa, proposta e evidência. A conclusão desta etapa **não** significa que os testes do sistema passaram. Firmware, circuito, integrações, imagens e vídeo continuam pendentes.