# Cultura escolhida: tomateiro

Este documento registra a pesquisa da etapa 1 do projeto FarmTech Solutions, conforme o [enunciado](../CONTEXT.md). A cultura recomendada é o tomateiro (*Solanum lycopersicum*). O [circuito de diagnóstico](circuito-wokwi.md) passou pelos testes da etapa 2 na nuvem; o controle automático e a conversão do LDR para pH ainda estão pendentes.

## Recorte do projeto

O cenário didático representa uma lavoura de tomate para consumo fresco, cultivado em solo a céu aberto, na fase de florescimento e desenvolvimento dos frutos. Não serão simuladas mudanças automáticas de estágio, variedades ou tipos de solo. Esse recorte permite discutir irrigação e nutrição sem representar todo o ciclo da cultura.

O tomateiro foi escolhido porque existem referências institucionais sobre seu manejo, sua necessidade de umidade regular e os riscos do excesso de água. Essas características permitem demonstrar decisões de irrigação e alertas com os componentes exigidos pelo projeto.

## Solo e pH

A Clemson Extension recomenda manter o pH do solo entre **6,0 e 6,5**, com correções e adubação orientadas por análise de solo [1]. Esse será o intervalo de referência adotado no perfil didático.

A Penn State Extension apresenta uma faixa de **5,8 a 6,6** para produção de tomate [2]. A diferença entre as fontes deve ser preservada: não existe aqui a pretensão de estabelecer uma faixa universal para qualquer região, solo ou sistema produtivo.

Na simulação, valores de pH abaixo de 6,0 ou acima de 6,5 gerarão alertas. O LDR não mede pH: sua leitura será convertida para uma escala artificial de 0 a 14. Os extremos e a direção dessa conversão precisam ser verificados no circuito antes da implementação definitiva.

## Necessidades de nutrientes

Segundo a Penn State Extension, a adubação deve considerar análises anuais do solo e, durante o cultivo, análises de tecido vegetal. A publicação menciona aplicações de nitrogênio e fósforo, além da necessidade de potássio e de outros nutrientes em momentos como florescimento e desenvolvimento dos frutos [2].

| Nutriente exigido no projeto | Tratamento na simulação | Limitação |
| --- | --- | --- |
| Nitrogênio (N) | Botão indica condição adequada ou insuficiente | Não informa concentração, dose ou excesso |
| Fósforo (P) | Botão indica condição adequada ou insuficiente | Não substitui análise de solo |
| Potássio (K) | Botão indica condição adequada ou insuficiente | Não quantifica disponibilidade para a planta |

O grupo não adotará uma dose universal de fertilizante nem converterá o estado dos botões em valores de massa ou concentração. Uma recomendação real depende do solo, da região, do estágio e das análises agronômicas. Caso sejam discutidas doses futuramente, será necessário distinguir nutrientes elementares de formulações expressas em P₂O₅ e K₂O.

O enunciado solicita ajustar o LDR ao alterar os botões de NPK. Esse ajuste será uma ação manual do demonstrador, não uma fórmula química: o estado de um botão não determina sozinho o pH do solo.

## Necessidades de água

A Penn State Extension destaca a necessidade de fornecimento constante de umidade durante o cultivo e o risco de rachaduras nos frutos associado ao excesso de água, especialmente após a formação dos frutos [2]. A publicação também descreve o uso de irrigação por gotejamento.

Para a demonstração, o controle buscará evitar acionamentos repetidos perto de um único limite. Serão propostos dois limiares, um para iniciar e outro para encerrar a irrigação. Seus valores são parâmetros de engenharia do simulador, **não recomendações de umidade real para o tomateiro**.

O DHT22 mede umidade relativa do ar. Conforme a substituição exigida pelo enunciado, sua leitura será tratada como um **índice simulado de umidade de 0 a 100**, sem equivalência com teor volumétrico de água no solo, tensão da água ou capacidade de campo.

## Aplicação da pesquisa

- Usar pH de referência entre 6,0 e 6,5, inclusive nos limites.
- Representar NPK como estados binários e emitir alertas para condições insuficientes.
- Priorizar a necessidade de água sem afirmar que irrigação corrige pH ou deficiência nutricional.
- Documentar os parâmetros didáticos e suas limitações na [especificação](especificacao-simulacao.md).
- Verificar os resultados esperados nos [cenários de validação](cenarios-validacao.md).

## Referências e limites

**[1] Clemson Cooperative Extension.** [Tomato, HGIC 1323](https://hgic.clemson.edu/factsheet/tomato/). Revisão: 2 de março de 2022. Consulta: 8 de outubro de 2026. Recomendações voltadas às condições da Carolina do Sul, Estados Unidos.

**[2] Penn State Extension.** [Tomato Production](https://extension.psu.edu/tomato-production). Atualização indicada: 21 de setembro de 2006. Consulta: 8 de outubro de 2026. Referência sobre produção de tomate fresco na Pensilvânia, Estados Unidos; sua idade e contexto regional limitam a aplicação direta.

As fontes sustentam a escolha de uma cultura para uma atividade educacional. Não constituem uma prescrição para uma fazenda brasileira. Aplicação em campo exigirá referências regionais atualizadas, análises de solo e orientação agronômica. Os dados simulados também não demonstram aumento de produtividade ou economia real de água.