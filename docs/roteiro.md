# Roteiro de vídeo — FarmTech Solutions

**Duração planejada: 4min40s, com 20 segundos de margem.**

Objetivo: demonstrar a Fase 2 com cultura pesquisada, componentes exigidos, código C/C++, decisões de irrigação, testes e integração Python com API pública. O painel local complementa, mas **não substitui**, a apresentação do circuito ESP32 no Wokwi. A análise em R é opcional e ainda não foi implementada nesta versão.

As falas abaixo são uma base: adapte à forma de falar do grupo e aos resultados observados no ensaio. Não anuncie aprovação de um teste que não terminou com sucesso.

## Preparação antes de gravar

1. Abra o repositório no VS Code e execute os comandos a partir da sua raiz. Deixe a fonte do terminal legível, sem precisar ampliar durante a gravação.
2. Separe as telas: circuito Wokwi com Monitor Serial, terminal de testes, painel local e código no VS Code. Feche notificações e arquivos pessoais.
3. No Wokwi, use o diagrama e o manifesto de bibliotecas atuais de [firmware/irrigation](../firmware/irrigation/diagram.json). Copie o sketch [irrigation.ino](../firmware/irrigation/irrigation.ino) para a aba `sketch.ino` e inclua os três headers: [alerts.h](../firmware/irrigation/alerts.h), [controller.h](../firmware/irrigation/controller.h) e [weather_command.h](../firmware/irrigation/weather_command.h). Use as dependências de [libraries.txt](../firmware/irrigation/libraries.txt). Compile e confira o funcionamento **antes** de gravar.
4. Confira os comandos manuais `1` e `0`, o contato NO e o LED azul. O firmware inicia em diagnóstico; o modo automático só é ativado pelo comando JSON indicado neste roteiro. Use quebra de linha LF no Serial.
5. Inicie o painel em um terminal separado. Se já estiver rodando, abra a página existente; não inicie outra instância na mesma porta.

```sh
python3 -m tools.simulator
```

Abra <http://127.0.0.1:8765>. Em VS Code remoto, encaminhe a porta 8765. A execução local requer Python 3 e `g++`; não requer token Wokwi.

6. No painel, ensaie os cenários em velocidade **5× ou 10×**. Para o cenário de timeout, use **25×**. A velocidade modifica a reprodução, não os tempos usados pelo controlador.
7. Deixe a pesquisa do [tomateiro](cultura-tomate.md) e os arquivos de código abertos nas abas relevantes. Não percorra cada linha de código: mostre as responsabilidades.
8. Execute os testes deste roteiro previamente. A suíte Python verificada nesta versão tem 18 testes; se a quantidade mudar, anuncie o resultado atual.
9. Teste a consulta Open-Meteo. Ela pode levar até duas tentativas de dez segundos. Grave esse trecho separadamente, se necessário; corte somente a espera e preserve comando, resposta e origem. Não apresente dados sintéticos como resposta da API.
10. Não abra o arquivo de credenciais, não imprima variáveis de token e não mostre informações pessoais. O vídeo final deve ser publicado no YouTube como **não listado**.

## Sequência de gravação

### 00:00–00:20 — Problema e cultura

**Mostrar:** título do projeto e documento da cultura, com a faixa de pH e as referências Clemson e Penn State visíveis.

**Falar:**

> “Este é o FarmTech Solutions, nosso projeto de irrigação inteligente com ESP32. Escolhemos o tomateiro, que precisa de umidade regular. A pesquisa orientou a faixa de pH de 6 a 6,5. Os limites de umidade e chuva são parâmetros didáticos do controlador, não uma recomendação para uma fazenda real.”

### 00:20–00:50 — Circuito exigido e acionamento manual

**Mostrar:** circuito **real do Wokwi**, indicando ESP32, três botões verdes, LDR, DHT22 e relé azul. Mostre o Monitor Serial e o LED de sinalização.

**Ações:** iniciar em diagnóstico, aguardar a leitura inicial, enviar `1` e depois `0` no Serial. Confira `RELE_COMANDO=ON CONTATO_NO=FECHADO` e, depois, `OFF` / `ABERTO`. Aguarde uma amostra após cada comando.

**Falar:**

> “Os três botões representam N, P e K. O LDR representa pH, e o DHT22 substitui didaticamente a umidade do solo. O relé azul representa a bomba. Enviando um, testo o acionamento; enviando zero, desligo. Aqui também verificamos o contato no GPIO19, em vez de confiar apenas no comando.”

**Cuidado:** um DHT22 mede umidade do ar e um LDR mede luz. Não os descreva como sensores agrícolas reais. O LED é apenas uma indicação adicional do relé.

### 00:50–01:15 — Código e divisão de responsabilidades

**Mostrar:** rapidamente o sketch, o controlador e o código meteorológico. Use as abas, não uma rolagem longa.

**Falar:**

> “O sketch lê os sensores e comanda o relé. Separar as regras em C++ permite testá-las sem hardware: o controlador decide a irrigação, os alertas avaliam NPK e pH, e o parser recebe o JSON meteorológico. O Python consulta a API. Nosso painel executa esses mesmos headers C++, sem duplicar as regras em Python.”

**Arquivos para deixar preparados:** [irrigation.ino](../firmware/irrigation/irrigation.ino), [controller.h](../firmware/irrigation/controller.h), [alerts.h](../firmware/irrigation/alerts.h), [weather_command.h](../firmware/irrigation/weather_command.h), [weather.py](../tools/weather.py) e [simulator_cli.cpp](../tools/simulator_cli.cpp).

### 01:15–01:50 — Suíte de testes no terminal

**Mostrar:** executar os comandos abaixo e deixar os resultados finais legíveis. Não instale bibliotecas ou toolchains durante a gravação.

```sh
python3 -m unittest discover -s tests -p 'test_*.py' -v
g++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/irrigation tests/controller_test.cpp -o /tmp/farmtech-controller-test
/tmp/farmtech-controller-test
g++ -std=c++17 -Wall -Wextra -Werror -Ifirmware/irrigation tests/alerts_test.cpp -o /tmp/farmtech-alerts-test
/tmp/farmtech-alerts-test
```

**Falar durante a execução:**

> “Além da demonstração visual, executamos testes automatizados. O Python verifica o contrato da previsão, entradas inválidas e o executor local. Os testes C++ verificam histerese, chuva, seca crítica, expiração, falhas e timeout. Também testamos as oito combinações NPK, os limites do pH e a conversão do ADC.”

**Evidências:** suíte Python finaliza com `OK`; os executáveis C++ imprimem seus resumos de sucesso. Se algum teste falhar, corrija ou relate a limitação antes de gravar a versão final.

### 01:50–02:15 — Entradas manuais no CLI local

**Mostrar:** executar a sequência abaixo. O painel iniciado na preparação já compilou o binário local.

```sh
printf '%s\n' \
  '0 35 24 1805 1 1 1 1 -1 0 1 1 2 60 10' \
  '2500 25 24 1805 1 1 1 1 -1 0 1 1 2 60 10' \
  '5000 40 24 1805 1 1 1 1 -1 0 1 1 2 60 10' \
  '10000 35 24 1805 1 1 1 1 -1 0 1 1 2 60 10' \
  | firmware/irrigation/build/simulator-cli
```

**Falar:**

> “Nesta entrada manual, usamos chuva sintética. Primeiro, a bomba fica suspensa. Com umidade 25, a seca crítica tem prioridade. Em 40, a suspensão retorna. Quando a previsão expira, o controle volta à umidade. Esses quatro resultados vêm diretamente do controlador C++.”

**Resultados, na ordem:** `RAIN_HOLD` / OFF; `CRITICAL_DRY` / ON; `RAIN_HOLD` / OFF; `MOISTURE_DEMAND` / ON.

Não leia os 15 campos em voz alta. Para referência, a ordem é: tempo em ms, umidade, temperatura, ADC, N, P, K, automático, comando manual, reset, nova amostra, previsão presente, chuva em mm, probabilidade máxima horária e validade em segundos. `-1` significa ausência de novo comando manual; os três botões estão adequados. A previsão desta sequência é recebida no instante zero e vale dez segundos simulados.

### 02:15–02:50 — Painel, histerese e alertas

**Mostrar:** painel local, com indicadores e histórico visíveis.

**Ações:** selecione **Histerese 35 / 50 / 65 / 50**, configure clima **Sem previsão** e execute em 5× ou 10×. Depois, selecione **Entradas atuais**, deixe umidade 35 e modo automático. Desmarque N e ajuste também o LDR para ADC 1001; execute. Volte N para adequado e ADC 1805 para mostrar a recuperação dos alertas, se houver tempo.

**Falar:**

> “A bomba liga em 40 ou menos e desliga em 60 ou mais. Entre esses limites, mantém a demanda anterior, evitando oscilações. Ao alterar N e ajustar o LDR, surgem alertas de nitrogênio e pH. Eles não bloqueiam água necessária: irrigação não corrige deficiência nutricional nem acidez.”

**Cuidado:** ao trocar de cenário, confira os inputs e a fonte do clima antes de executar. O painel mostra contato **lógico**, não retorno elétrico. A umidade é alterada pelo demonstrador, não pela bomba automaticamente.

### 02:50–03:25 — Chuva, seca crítica e expiração

**Ações:** executar **Seca crítica e recuperação**, mantendo sua previsão sintética, e depois **Previsão expirada**. Use o histórico para apontar as transições; pause ou avance uma leitura se necessário.

**Falar:**

> “A suspensão por chuva exige dois critérios: pelo menos dois milímetros nas seis horas e máxima probabilidade horária de 60%. A seca crítica, em 25 ou menos, permite irrigar apesar da chuva e mantém essa prioridade até alcançar 40. Sem previsão válida, ou depois da expiração, a decisão volta às regras de umidade.”

Não chame a máxima probabilidade horária de “probabilidade de chover nas seis horas”. Os limiares são uma política ilustrativa, não um modelo agronômico validado.

### 03:25–04:00 — Python e API pública

**Mostrar:** terminal executando o CLI, com coordenadas configuráveis e JSON de saída.

```sh
python3 tools/weather.py --latitude=-23.55 --longitude=-46.63
```

**Ações:** copie somente o JSON resultante, volte ao Wokwi em execução e cole no Monitor Serial com LF. Mostre a confirmação `ACK WEATHER`. Para ativar o controle automático, envie:

```json
{"type":"auto","schema_version":1}
```

Se o circuito estiver com umidade 50, o modo automático pode permanecer desligado: ajuste o DHT22 para 35 e confira a decisão após uma nova leitura. A previsão real pode não indicar chuva; a suspensão determinística já foi demonstrada com dados identificados como sintéticos.

**Falar:**

> “Este Python consulta o Open-Meteo para coordenadas configuráveis e valida seis intervalos futuros completos em UTC. O resultado vira um JSON com validade, transferido manualmente pelo Serial, como permite o enunciado. A previsão real depende do local e do momento; não inventamos chuva para a demonstração.”

**Se a API falhar:** mostre a mensagem de erro e explique o retorno ao controle por umidade no modo local. Não cole uma previsão antiga como se fosse nova. No ESP32 sem relógio confiável, a validade é contada desde o recebimento; a primeira mensagem antiga após reinício não é identificada apenas pelo timestamp. Consulte imediatamente antes de colar.

### 04:00–04:25 — Segurança e falhas

**Ações:** no painel, executar **Falha de sensor e recuperação** e **Limite de 60 s e reset**; neste último, usar 25× e mostrar no histórico a linha `TIMEOUT` e a retomada após reset.

**Falar:**

> “Na irrigação automática, falha ou atraso de sensor desliga a bomba. Sessenta segundos contínuos também desligam e travam o acionamento até reset. Essas proteções têm prioridade sobre a seca crítica. O teste manual de cinco segundos é um diagnóstico separado, não um modo seguro para uma bomba física.”

**Evidências:** `SENSOR_FAULT`, `TIMEOUT` e bomba OFF; após leitura válida ou reset apropriado, as entradas são reavaliadas. O vídeo é uma demonstração de lógica, não certificação de segurança física.

### 04:25–04:40 — Encerramento e entrega

**Mostrar:** estrutura do repositório, documentação e endereço do projeto Wokwi salvo, quando disponível.

**Falar:**

> “Entregamos código C/C++, pesquisa da cultura, documentação, testes e integração Python com API pública. O painel facilita reproduzir os casos; o Wokwi demonstra os componentes exigidos. A análise em R ainda não faz parte desta versão. Esta é uma solução educacional, sem promessa de economia de água medida em campo.”

**Encerrar até 04:40.** Não use os vinte segundos de margem para iniciar uma nova demonstração.

## Comandos auxiliares para o ensaio

Estes comandos ajudam a preparar ou investigar a gravação; **não precisam todos entrar no vídeo**.

### Compilar somente o CLI local

Se preferir não iniciar o painel antes da sequência de terminal:

```sh
mkdir -p firmware/irrigation/build
g++ -std=c++11 -Wall -Wextra -Werror -Ifirmware/irrigation tools/simulator_cli.cpp -o firmware/irrigation/build/simulator-cli
```

### Remover a previsão no Serial Wokwi

```json
{"type":"weather_clear","schema_version":1}
```

Resultado esperado: `ACK WEATHER_CLEAR`; a irrigação automática volta a considerar a umidade sem suspensão meteorológica.

### Reset do timeout no Serial Wokwi

```json
{"type":"reset","schema_version":1}
```

Resultado esperado: `ACK RESET`. O reset reavalia as condições; não garante acionamento se um sensor estiver inválido ou outra regra impedir a irrigação.

### Lint e suíte Wokwi na nuvem

```sh
wokwi-cli lint --warnings-as-errors firmware/irrigation/diagram.json
node tools/test-circuit.mjs
```

O lint verifica o diagrama, não executa o circuito. A suíte requer firmware compilado e token configurado, usa a **nuvem** e consome quota. Não rode essa suíte longa durante o vídeo de cinco minutos. Se quiser mostrar sua evidência, execute antes e grave apenas um trecho do resultado real, identificado como execução anterior.

O diagnóstico e os alertas tiveram validações anteriores na nuvem. A execução da suíte ampliada com clima atingiu um limite de tempo e não foi concluída; **não afirme que essa suíte completa passou**. Uma execução incompleta não equivale automaticamente a defeito no controlador, mas também não comprova a integração inteira.

## Checklist de entrega

- [ ] Vídeo com até cinco minutos, áudio legível e publicado como não listado.
- [ ] Circuito Wokwi de fato mostrado; não apenas o desenho lógico do painel.
- [ ] NPK, LDR, DHT22, relé azul e estados ON/OFF visíveis.
- [ ] NPK e LDR alterados juntos em uma demonstração, conforme o enunciado.
- [ ] Testes mostrados com os resultados reais, sem esconder falhas.
- [ ] Previsão da API distinguida de dados sintéticos; JSON recebido no Serial demonstrado.
- [ ] Limitações dos sensores substitutos e do painel local esclarecidas.
- [ ] Repositório com fontes e README atualizado, explicando lógica e instalação.
- [ ] Imagens reais do circuito, link Wokwi e link do vídeo incluídos no README.
- [ ] Análise em R não apresentada como implementada; é extensão opcional pendente.
- [ ] Falas revisadas pelo grupo para uma apresentação autoral e compatível com a versão entregue.