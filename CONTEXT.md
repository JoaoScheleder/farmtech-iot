Introdução:

Antes de iniciar, vamos relembrar: você e seu grupo estão na Startup FarmTech Solutions, trabalhando na equipe de desenvolvedores. Naturalmente, vocês podem utilizar o ChatGPT, Gemini ou outra IA de seu interesse para ajudar com essa tarefa. A FIAP não condena o uso de IAs em seus estudos, desde que o aluno tenha o olhar crítico para filtrar os erros e acertos das respostas propostas por elas e montar a sua própria resposta de forma autoral.

No entanto, é importante tomar cuidado ao consultar essas plataformas, pois grupos concorrentes podem estar fazendo o mesmo. Caso a solução apresentada seja exatamente um “copy-paste” do GPT para o portal da FIAP, o resultado poderá ser idêntico entre os grupos e, nesse caso, isso caracterizará plágio e os grupos envolvidos não terão nota.

A FarmTech Solutions continua seu desenvolvimento na Agricultura Digital. Nesta atividade, teremos um desafio interessante: trabalhar em grupo para construir/simular um dispositivo eletrônico capaz de coletar dados em uma fazenda.

 

Descrição do Projeto:

Considerando como base a Fase anterior do projeto — que envolveu o cálculo de área plantada, monitoramento climático, entre outros —, a Fase 2 vai avançar no sistema de gestão agrícola da empresa FarmTech Solutions usando um dispositivo construído por você e seu grupo.

Agora, vamos imaginar em como podemos conectar os sensores físicos para otimizar a irrigação agrícola e criar um sistema de irrigação inteligente. Toda cultura agrícola depende, em maior ou menor proporção, de três elementos químicos: Nitrogênio (N), Fósforo (P) e Potássio (K) — o famoso NPK. Isso vai influenciar o pH da terra e, obviamente, a produtividade daquela planta. Além disso, é preciso considerar a umidade do solo, que indica o quanto choveu em um determinado período observado. Infelizmente, no Wokwi.com — plataforma onde simulamos projetos ESP32 — não há sensores exclusivamente agrícolas. Por isso, faremos simulações e algumas substituições didáticas.

No lugar dos sensores de nutrientes N, P e K, utilizaremos um botão verde em cada. Portanto, seu projeto precisa ter três botões simulando os níveis de cada elemento.

No lugar do sensor de pH, utilizaremos um sensor de intensidade de luz chamado LDR (Light Dependent Resistor) que trará dados analógicos da intensidade da luz, mas, para fins de simulação, vamos assumir que ele representa o pH da terra. Como referência, podemos comparar os dados analógicos do pH que variam de 0 a 14, sendo próximo de 7, pH neutro. Você também pode adotar outras escalas maiores que 0 a 14 para melhorar sua mecânica ao manipular o sensor LDR.

Quanto ao sensor de umidade, este possui um similar no Wokwi que mede a umidade do ar. Portanto, vamos adotar o DHT22 como medidor de umidade do solo (embora seja do ar na prática).

O objetivo do projeto na Fase 2 será desenvolver um sistema de irrigação automatizado e inteligente que monitore a umidade do solo em tempo real, os níveis dos nutrientes N, P e K representados por botões (que vão “medir” os níveis como tudo ou nada, isto é, “true” ou “false”, ou em outras palavras, como botão pressionado ou não pressionado).  

Quando você mexer nos botões e alterar os níveis do NPK, você deve mexer no sensor pH representado pelo sensor LDR, pois, em tese, você estaria alterando o pH da terra.

Além disso, o sensor de umidade DHT22 vai decidir se precisa ligar ou não a irrigação conforme o necessário — isto é, ligando um relé azul representando uma bomba d’água de verdade.

Você e seu grupo podem pensar em irrigar uma lavoura (ligando o relé azul) de acordo com a combinação dos níveis de N, P, K, pH e umidade que desejarem, bastando para isso, escolher uma cultura agrícola e pesquisar quais suas reais necessidades de nutrientes em uma fazenda. A lógica de decisão de quando ligar ou desligar a bomba d’água é do grupo. Só precisa documentar isso na entrega.

 

Ir além – Integração Python com API Pública (opcional 1):

Além do controle básico de irrigação com base nos sensores já mencionados, você e seu grupo podem integrar dados meteorológicos obtidos de uma API pública, como a OpenWeather, para prever condições climáticas adversas e ajustar a irrigação automaticamente.

Por exemplo, se houver previsão de chuva, o sistema pode suspender a irrigação para economizar recursos. A integração entre Python e ESP32 do Wokwi.com não é trivial no plano gratuito. Caso não obtenha sucesso, transfira os dados manualmente entre os sistemas C/C++ e Python, copiando os dados dos resultados da API que conseguiu usando o Python para uma variável qualquer no código C/C++ do ESP32 no Wokwi, a qual indicará o nível de chuva na sua lógica de irrigação.

Caso encontre uma forma automática para resolver isso, melhor ainda! Uma opção é ler caracteres via Monitor Serial do simulador ESP32, onde você pode inserir dados via tela do seu computador com o seu teclado enquanto o código está rodando. Basta explorar as funções Serial.available() e Serial.read().

 

Ir além – Análise em R (opcional 2):

O grupo pode tentar implementar uma análise estatística em R qualquer — seja as apresentadas até o momento ou buscando nas referências da disciplina de R — para decidir se deve ligar ou não a bomba de irrigação (que é o relé azul). Isso lhe trará conhecimento de Data Science, um cargo que é bastante procurado no mercado de trabalho.

Os grupos que desenvolverem, de forma parcial ou total, os itens opcionais do programa Ir Além estarão se desenvolvendo mais e, consequentemente, serão monitorados internamente. Além disso, eventualmente, poderão ser convidados a participar de outros programas oferecidos pela FIAP, pela coordenação ou pelos professores.

 

Os benefícios ao concluir essa Fase 2 serão:

Este desafio permitirá que você e seu grupo aplique conhecimentos em sensoriamento, IoT, consulta de API e Data Science. Além disso, trará uma visão prática do uso da IoT e da IA para otimização de recursos em um ambiente agrícola.

Essa integração é fundamental para o sucesso da nossa startup FarmTech Solutions, e também para o projeto de uma fazenda inteligente, garantindo que a água seja utilizada de forma eficiente, reduzindo desperdícios e maximizando a produtividade agrícola.

Entregáveis:

Organizar os arquivos fonte no seu GitHub, separando no seu Git, um repositório. Por exemplo: meugit/cursotiaor/pbl/fase3/pastas;
Anexar ao Git o REAME.MD feito em markdown que explica todo o funcionamento do projeto, documentando toda a lógica e especificidades;
Anexar ao README.MD as imagens do circuito da plataforma Wokwi.com que demonstre as conexões dos sensores solicitados;
Anexar ao Git o código C/C++ desenvolvido no ESP32;
Anexar os códigos do Programa Ir Além (opcionais 1 e/ou 2);
Anexar ao Git o link de um vídeo de até 5 minutos postado no YouTube (sem listagem), demonstrando o funcionamento completo de seu projeto referente ao que foi solicitado aqui.