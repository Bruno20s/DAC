#include <EEPROM.h>
#include <LiquidCrystal_I2C.h>

// pinos para o LCD 
LiquidCrystal_I2C lcd(0x27,16,2);

// Unir os pinos dos botões para A e B
const int pinosBotoes[2][4] = {{7, 8, 9, 10}, // Lado A: Eixo1, Eixo2, RodDupla1, RodDupla2
                               {6, 5, 4, 3}}; // Lado B: Eixo1, Eixo2, RodDupla1, RodDupla2

const int interruptorDMM = 11;
const int interruptorBO = 2;
const int led = 13;

const int enderecosEixos[2][3] = {{0, 2, 4},  // Endereços EEPROM para A: Eixo1, Eixo2, RodagemDupla
                                  {6, 8, 10}}; // Endereços EEPROM para B: Eixo1, Eixo2, RodagemDupla

int enderecoDistanciaSensores = 12; // Endereço na EEPROM
int enderecoVelocidadeSerial = 14;

float distanciaSensores = 0.4;
int velocidadeSerial = 9600; // Velocidade inicial da porta serial
int opcao = 3; // Valor inicial

// Estados e contadores
int sistemaAtivo = 0;
int ultimoSistemaAtivo = LOW;
unsigned long inicioTempoInterruptor = 0;
unsigned long duracaoInterruptor = 0;

// Estados dos botões e tempos
int estadosBotoes[2][4] = {{LOW, LOW, LOW, LOW}, {LOW, LOW, LOW, LOW}};
unsigned long temposBotoes[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};

// Contadores
int contadoresEixos[2][2] = {{0, 0}, {0, 0}};  // Eixos 1 e 2 para A e B
int contadoresRodagemDupla[2] = {0, 0};        // Rodagem dupla para A e B

int contagemBotoes[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Para botões 1, 2, 3, 4, 7, 8, 9, 0

bool rodagemDuplaDetectada[2] = {false, false};  // A e B
bool velocidadeComBotaoRodDupla[2][2] = {{false, false}, {false, false}};  // A e B: RodDupla1, RodDupla2

int estadoSequencia[2] = {0, 0};  // Estado de sequência para A e B

bool retrocessoDetectado = false;
bool velocidadeExibida = false;

// Arrays para debounce
unsigned long ultimoTempoBotao[2][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}};
unsigned long tempoDebounce = 50;

void setup() {
  lcd.begin (16,2);
  lcd.backlight();
  lcd.print("Sistema Iniciado"); // Exibe uma mensagem inicial
  delay(3000); // Mostra a mensagem por 3 segundos
  lcd.clear(); // Limpa o display

  pinMode(led, OUTPUT);
  pinMode(interruptorBO, INPUT_PULLUP);
  pinMode(interruptorDMM, INPUT_PULLUP);

  for (int lado = 0; lado < 2; lado++) {
    for (int i = 0; i < 4; i++) {
      pinMode(pinosBotoes[lado][i], INPUT_PULLUP);
    }
  }

  recuperarVelocidadeSerialEEPROM();  // Recupera a velocidade da EEPROM
  Serial.begin(velocidadeSerial);     // Inicializa a serial com a velocidade recuperada
  recuperarDistanciaSensoresEEPROM(); // Recupera a distância dos sensores da EEPROM
}

int estado = 0; // Estado inicial
String bufferComando = ""; // Armazena o comando recebido

void verificarComando(char entrada) {
  switch (estado) {
    case 0: // Estado inicial
      if (entrada == '<') {
        estado = 1; // Vai para o estado de verificação de comando
        bufferComando = ""; // Limpa o buffer
      }
      break;

    case 1: // Verifica se tem "<", indicando que é um comando
      if (entrada == '>') {
        executarComando(bufferComando); // Comando completo, processa
        estado = 0; // Volta ao estado inicial
      } 
      else if (entrada == '<') {
        estado = 3; // Se "<" aparecer antes de ">", descarta o comando
      } 
      else {
        bufferComando += entrada; // Continua acumulando o comando
      }
      break;

    case 3: // Descartar tudo antes do "<" anterior
      if (entrada == '>') {
        estado = 0; // Volta ao estado inicial após descartar o comando
      }
      break;

    default:
      estado = 0; // Em qualquer outro caso, reseta para o estado inicial
      break;
  }
}

void executarComando(String comando) {
  if (comando.startsWith("s:")) {
    configurarVelocidadeSerial(comando); 
  } 
  else if (comando == "reset") {
    resetarVelocidade(); 
    Serial.println("{\"status\":\"OK\",\"message\":\"Sistema resetado.\"}");
  } 
  else if (comando == "status") {
    mostrarRelatorio();
  } 
  else if (comando.startsWith("d:")) {
    configurarDistanciaSensores(comando); // Salva na EEPROM
  } 
  else {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Comando desconhecido.\"}");
  }
}

void loop() {
  while (Serial.available()) {
    char entrada = Serial.read();
    verificarComando(entrada);
  }

  verificarInterruptores();

  if (sistemaAtivo == 1) {
    digitalWrite(led, HIGH);
    verificarSequencia(0);  // Lado A
    verificarSequencia(1);  // Lado B
    verificarRodagemDuplaSimultanea();
  } else if (sistemaAtivo == 0) {
    digitalWrite(led, LOW);
    calcularVelocidade();
  }

  delay(2);
}

// Salva a distância dos sensores na EEPROM
void salvarDistanciaSensoresEEPROM(float novaDistancia) {
  EEPROM.put(enderecoDistanciaSensores, novaDistancia);
}

// Salva a velocidade da porta serial na EEPROM
void salvarVelocidadeSerialEEPROM(int novaVelocidade) {
  EEPROM.put(enderecoVelocidadeSerial, novaVelocidade);
}

// Configura a distância dos sensores a partir do comando
void configurarDistanciaSensores(String comando) {
  float novaDistancia = comando.substring(3).toFloat();
  if (novaDistancia > 0) {
    distanciaSensores = novaDistancia;
    salvarDistanciaSensoresEEPROM(novaDistancia); // Salva na EEPROM somente para "d:"
    Serial.print("{\"status\":\"OK\",\"distancia\":");
    Serial.print(distanciaSensores);
    Serial.println("}");
  } else {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Distância inválida.\"}");
  }
}

void configurarVelocidadeSerial(String comando) {
  char escolha = comando.charAt(3); // Pega o caracter após "<s:"

  if (escolha < '1' || escolha > '5') {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Opção inválida! Escolha entre 1 a 5.\"}");
    return;
  }

  switch (escolha) {
    case '1':
      velocidadeSerial = 2400;
      opcao = 1;
      break;
    case '2':
      velocidadeSerial = 4800;
      opcao = 2;
      break;
    case '3':
      velocidadeSerial = 9600;
      opcao = 3;
      break;
    case '4':
      velocidadeSerial = 14400;
      opcao = 4;
      break;
    case '5':
      velocidadeSerial = 19200;
      opcao = 5;
      break;
  }

  salvarVelocidadeSerialEEPROM(velocidadeSerial);  // Salva a nova velocidade na EEPROM

  Serial.end();
  Serial.begin(velocidadeSerial);  // Reinicia a comunicação serial com a nova velocidade

  Serial.print("{\"status\":\"OK\",\"velocidade\":");
  Serial.print(velocidadeSerial);
  Serial.println("}");
}


void mostrarRelatorio() {
  String mensagem = "<\"EX1A\":" + String(contadoresEixos[0][0]) + 
                    ",\"EX2A\":" + String(contadoresEixos[0][1]) +
                    ",\"RDA\":" + String(contadoresRodagemDupla[0]) +
                    ",\"EX1B\":" + String(contadoresEixos[1][0]) +  
                    ",\"EX2B\":" + String(contadoresEixos[1][1]) +
                    ",\"RDB\":" + String(contadoresRodagemDupla[1]) + ">";

  Serial.println(mensagem);

  // Exibe a contagem de pressionamentos de botões
  String mensagemBotoes = "<\"botao1\":" + String(contagemBotoes[0]) +
                          ",\"botao2\":" + String(contagemBotoes[1]) +
                          ",\"botao3\":" + String(contagemBotoes[2]) +
                          ",\"botao4\":" + String(contagemBotoes[3]) +
                          ",\"botao7\":" + String(contagemBotoes[4]) +
                          ",\"botao8\":" + String(contagemBotoes[5]) +
                          ",\"botao9\":" + String(contagemBotoes[6]) +
                          ",\"botao0\":" + String(contagemBotoes[7]) + ">";

  Serial.println(mensagemBotoes);

    String mensagemLCD = "E1A:" + String(contadoresEixos[0][0]) + 
                     "E2A:" + String(contadoresEixos[0][1]) +
                     " RDA:" + String(contadoresRodagemDupla[0]);

String mensagemLCD2 = "E1B:" + String(contadoresEixos[1][0]) + 
                      "E2B:" + String(contadoresEixos[1][1]) +
                      " RDB:" + String(contadoresRodagemDupla[1]);

// Exibe a primeira linha
lcd.setCursor(0, 0);  // Coluna 0, Linha 0
lcd.print(mensagemLCD);

// Exibe a segunda linha
lcd.setCursor(0, 1);  // Coluna 0, Linha 1
lcd.print(mensagemLCD2);


  Serial.print("{ \"status\":\"OK\",\"tempoInterruptor\":");
  Serial.print(duracaoInterruptor);
  Serial.println("}");

  // Exibe a velocidade serial atual
  Serial.print("{ \"status\":\"OK\",\"velocidadeSerial\":");
  Serial.print(velocidadeSerial);
  Serial.println("}");
}

// Função que verifica a rodagem dupla simultânea
void verificarRodagemDuplaSimultanea() {
  for (int lado = 0; lado < 2; lado++) {
    if (!digitalRead(pinosBotoes[lado][2]) == HIGH && !digitalRead(pinosBotoes[lado][3]) == HIGH) {
      if (!rodagemDuplaDetectada[lado]) {
        contadoresRodagemDupla[lado]++;
        rodagemDuplaDetectada[lado] = true;
        Serial.print("Rodagem dupla simultânea detectada no lado ");
        Serial.println(lado == 0 ? "A" : "B");
      }
    } else {
      rodagemDuplaDetectada[lado] = false;
    }
  }
}

void verificarInterruptores() {
  int estadoAtualDMM = !digitalRead(interruptorDMM);
  int estadoAtualBO = !digitalRead(interruptorBO);

  sistemaAtivo = estadoAtualDMM && estadoAtualBO;

  if (sistemaAtivo && ultimoSistemaAtivo == LOW) {
    Serial.println("inicio de avance");
    lcd.print("inicio de avance");
    inicioTempoInterruptor = millis();
    resetarVelocidade();
    velocidadeExibida = false; // Permite calcular novamente quando o sistema estiver ativo
  }

  if (!estadoAtualBO && ultimoSistemaAtivo == HIGH) {
  duracaoInterruptor = millis() - inicioTempoInterruptor;
  Serial.println("fim de avance");
  mostrarRelatorio();
  salvarDistanciaSensoresEEPROM(distanciaSensores);
  resetarContagemBotoes();
  }

  ultimoSistemaAtivo = sistemaAtivo;
}

void resetarContagemBotoes() {
  // Zera as contagens dos botões
  memset(contagemBotoes, 0, sizeof(contagemBotoes));
  
  // Zera as contagens de eixos e rodagem dupla
  memset(contadoresEixos, 0, sizeof(contadoresEixos));  
  memset(contadoresRodagemDupla, 0, sizeof(contadoresRodagemDupla));
}

void resetarVelocidade() {
  // Resetar estados relacionados à velocidade
  velocidadeExibida = false;
  for (int i = 0; i < 2; i++) {
    velocidadeComBotaoRodDupla[i][0] = false;
    velocidadeComBotaoRodDupla[i][1] = false;
  }
}

// Adiciona arrays para armazenar o último tempo de mudança dos botões
unsigned long ultimoTempoBotaoA[4] = {0, 0, 0, 0};
unsigned long ultimoTempoBotaoB[4] = {0, 0, 0, 0};

// Função de debounce para verificar se o botão foi realmente pressionado
bool debounce(int lado, int botao) {
  int estadoAtual = !digitalRead(pinosBotoes[lado][botao]);
  unsigned long tempoAtual = millis();

  if (estadoAtual != estadosBotoes[lado][botao]) {
    if (tempoAtual - ultimoTempoBotao[lado][botao] > tempoDebounce) {
      estadosBotoes[lado][botao] = estadoAtual;
      ultimoTempoBotao[lado][botao] = tempoAtual;
      return true;
    }
  }

  return false;
}

void verificarSequencia(int lado) {
  for (int i = 0; i < 4; i++) {
    if (debounce(lado, i)) {
      if (estadosBotoes[lado][i] == HIGH) {
        temposBotoes[lado][i] = millis();
        contagemBotoes[i + (lado == 1 ? 4 : 0)]++;

        if (estadoSequencia[lado] == i) {
          estadoSequencia[lado]++;
          Serial.print("# Sequência correta: Botão ");
          Serial.print(i + 1);
          Serial.print(" (lado ");
          Serial.print(lado == 0 ? "A" : "B");
          Serial.println(")!");

          if (i < 2) {
            contadoresEixos[lado][i]++;
          }

          if (i == 3) {
            estadoSequencia[lado] = 0;
            Serial.print("# Rodagem dupla completa no lado ");
            Serial.println(lado == 0 ? "A" : "B");
          }

          if (i >= 2) {
            velocidadeComBotaoRodDupla[lado][i - 2] = true;
          }
        } else if (estadoSequencia[lado] > i) {
          Serial.print("# Retrocesso detectado no lado ");
          Serial.println(lado == 0 ? "A" : "B");

          if (estadoSequencia[lado] == 2 && contadoresEixos[lado][1] > 0) {
            contadoresEixos[lado][1]--;
          }
          if (estadoSequencia[lado] == 1 && contadoresEixos[lado][0] > 0) {
            contadoresEixos[lado][0]--;
          }
          estadoSequencia[lado]--;
          retrocessoDetectado = true;
        }
      }
    }
  }
}

void recuperarDistanciaSensoresEEPROM() {
  EEPROM.get(enderecoDistanciaSensores, distanciaSensores);
  if (isnan(distanciaSensores) ) {
    distanciaSensores = 0.4;
  }
}

// Recupera a velocidade serial armazenada na EEPROM
void recuperarVelocidadeSerialEEPROM() {
  EEPROM.get(enderecoVelocidadeSerial, velocidadeSerial);
  if (velocidadeSerial != 2400 && velocidadeSerial != 4800 && 
      velocidadeSerial != 9600 && velocidadeSerial != 14400 && 
      velocidadeSerial != 19200) {
    // Se o valor recuperado não for uma velocidade válida, definir padrão 9600
    velocidadeSerial = 9600;
  }
}

void calcularVelocidade() {
  if (!velocidadeExibida) { // Garante que a velocidade será exibida apenas uma vez
    for (int i = 0; i < 2; i++) {
      if (velocidadeComBotaoRodDupla[i][0] && velocidadeComBotaoRodDupla[i][1]) {
        unsigned long tempoTotal = (i == 0 ? temposBotoes[0][3] : temposBotoes[1][3]) - (i == 0 ? temposBotoes[0][2] : temposBotoes[1][2]);      
        if (tempoTotal > 0) {
          float velocidade = distanciaSensores / (tempoTotal / 1000.0);  // Cálculo de velocidade
          Serial.print("Velocidade lado ");
          Serial.print(i == 0 ? "A" : "B");
          Serial.print(": ");
          Serial.print(velocidade);
          Serial.println(" m/s");
        }
      }
    }
    velocidadeExibida = true;  // Garante que a velocidade será exibida uma única vez por ciclo
  }
}