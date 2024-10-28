#include <EEPROM.h>

const int pinosBotoesA[] = {7, 3, 4, 5}; // Eixo1A, Eixo2A, RodDupla1A, RodDupla2A
const int pinosBotoesB[] = {6, 10, 9, 8}; // Eixo1B, Eixo2B, RodDupla1B, RodDupla2B
const int interruptorDMM = 11;
const int interruptorBO = 2;
const int led = 13;

const int enderecoEixo1A = 0;
const int enderecoEixo2A = 2;
const int enderecoRodagemDuplaA = 4;
const int enderecoEixo1B = 6;
const int enderecoEixo2B = 8;
const int enderecoRodagemDuplaB = 10;

float distanciaSensores = 0.4; // Distância entre sensores em metros

// Estados e contadores
int sistemaAtivo = 0;
int ultimoSistemaAtivo = LOW;
unsigned long inicioTempoInterruptor = 0;
unsigned long duracaoInterruptor = 0;

int estadosBotoesA[4] = {LOW, LOW, LOW, LOW};
int estadosBotoesB[4] = {LOW, LOW, LOW, LOW};

unsigned long temposBotoesA[4] = {0, 0, 0, 0};
unsigned long temposBotoesB[4] = {0, 0, 0, 0};

int contadoresEixosA[2] = {0, 0}; // Eixo1A, Eixo2A
int contadoresEixosB[2] = {0, 0}; // Eixo1B, Eixo2B
int contadoresRodagemDupla[2] = {0, 0}; // Rodagem dupla A e B

int contagemBotoes[8] = {0, 0, 0, 0, 0, 0, 0, 0}; // Para botões 1, 2, 3, 4, 7, 8, 9, 0

bool rodagemDuplaDetectada[2] = {false, false}; // A e B
bool velocidadeComBotaoRodDupla[2][2] = {{false, false}, {false, false}}; // A e B: RodDupla1, RodDupla2

int estadoSequenciaA = 0;
int estadoSequenciaB = 0;

bool retrocessoDetectado = false;
bool velocidadeExibida = false; 

int velocidadeSerial = 9600; // Velocidade inicial da porta serial
int opcao = 3; // Valor inicial

void recuperarDadosEEPROM() {
  EEPROM.get(enderecoEixo1A, contadoresEixosA[0]);
  EEPROM.get(enderecoEixo2A, contadoresEixosA[1]);
  EEPROM.get(enderecoRodagemDuplaA, contadoresRodagemDupla[0]);
  EEPROM.get(enderecoEixo1B, contadoresEixosB[0]);
  EEPROM.get(enderecoEixo2B, contadoresEixosB[1]);
  EEPROM.get(enderecoRodagemDuplaB, contadoresRodagemDupla[1]);
}

void setup() {
  pinMode(led, OUTPUT);
  pinMode(interruptorBO, INPUT_PULLUP);
  pinMode(interruptorDMM, INPUT_PULLUP);  
  
  for (int i = 0; i < 4; i++) {
    pinMode(pinosBotoesA[i], INPUT_PULLUP);
    pinMode(pinosBotoesB[i], INPUT_PULLUP);
  }
  
  Serial.begin(velocidadeSerial); // Inicia a comunicação serial
  recuperarDadosEEPROM(); 
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
    mostrarRelatorio(); // Exibe o status atual
  } 
  else if (comando.startsWith("d:")) {
    configurarDistanciaSensores(comando);
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
    digitalWrite(led, HIGH);  // Acende o led quando o sistema estiver ativo
    verificarSequencia(pinosBotoesA, estadosBotoesA, temposBotoesA, "A", estadoSequenciaA);
    verificarSequencia(pinosBotoesB, estadosBotoesB, temposBotoesB, "B", estadoSequenciaB);
    verificarRodagemDuplaSimultanea(); // Verifica rodagem dupla
  } 
  else if (sistemaAtivo == 0) {
    digitalWrite(led, LOW);  // Apaga o led quando o sistema estiver desativado
    calcularVelocidade();
  }

  delay(2); 
}

void salvarDadosEEPROM() {
  EEPROM.put(enderecoEixo1A, contadoresEixosA[0]);
  EEPROM.put(enderecoEixo2A, contadoresEixosA[1]);
  EEPROM.put(enderecoRodagemDuplaA, contadoresRodagemDupla[0]);
  EEPROM.put(enderecoEixo1B, contadoresEixosB[0]);
  EEPROM.put(enderecoEixo2B, contadoresEixosB[1]);
  EEPROM.put(enderecoRodagemDuplaB, contadoresRodagemDupla[1]);
}

void configurarDistanciaSensores(String comando) {
  float novaDistancia = comando.substring(3).toFloat();
  if (novaDistancia > 0) {
    distanciaSensores = novaDistancia;
    Serial.print("{\"status\":\"OK\",\"distancia\":");
    Serial.print(distanciaSensores);
    Serial.println("}");
  } else {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Distância inválida.\"}");
  }
}

void configurarVelocidadeSerial(String comando) {

  char escolha = comando.charAt(3); // Pega o caracter após "<s:"
  
  // Verifica se o caractere é numérico entre '1' e '5'
  if (escolha < '1' || escolha > '5') {
    Serial.println("{\"status\":\"ERROR\",\"message\":\"Opção inválida! Escolha entre 1 a 5.\"}");
    return;
  }

  // Definir a velocidade e a opção com base no valor de 'escolha'
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

  // Reiniciar a comunicação serial com a nova velocidade
  Serial.end();
  Serial.begin(velocidadeSerial);

  // Confirmação da nova velocidade serial
  Serial.print("{\"status\":\"OK\",\"velocidade\":");
  Serial.print(velocidadeSerial);
  Serial.println("}");
}

void resetarVelocidade() {
  memset(velocidadeComBotaoRodDupla, 0, sizeof(velocidadeComBotaoRodDupla));
  memset(contadoresEixosA, 0, sizeof(contadoresEixosA));
  memset(contadoresEixosB, 0, sizeof(contadoresEixosB));
  contadoresRodagemDupla[0] = 0;
  contadoresRodagemDupla[1] = 0;

  Serial.println("{ \"status\":\"OK\",\"message\":\"Velocidade e contadores resetados.\"}");
}

void mostrarRelatorio() {
  String mensagem = "<\"EX1A\":" + String(contadoresEixosA[0]) +
                    ",\"EX2A\":" + String(contadoresEixosA[1]) +
                    ",\"RDA\":" + String(contadoresRodagemDupla[0]) +
                    ",\"EX1B\":" + String(contadoresEixosB[0]) +
                    ",\"EX2B\":" + String(contadoresEixosB[1]) +
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
  if (!digitalRead(pinosBotoesA[2]) == HIGH && !digitalRead(pinosBotoesA[3]) == HIGH) {
    if (!rodagemDuplaDetectada[0]) {
      contadoresRodagemDupla[0]++;
      rodagemDuplaDetectada[0] = true;
      Serial.println("Rodagem dupla simultânea detectada no lado A!");
    }
  } else {
    rodagemDuplaDetectada[0] = false; // Reseta o estado para permitir nova detecção
  }

  // Verifica se RodDupla1B e RodDupla2B estão pressionados simultaneamente
  if (!digitalRead(pinosBotoesB[2]) == HIGH && !digitalRead(pinosBotoesB[3]) == HIGH) {
    if (!rodagemDuplaDetectada[1]) {
      contadoresRodagemDupla[1]++;
      rodagemDuplaDetectada[1] = true;
      Serial.println("Rodagem dupla simultânea detectada no lado B!");
    }
  } else {
    rodagemDuplaDetectada[1] = false; // Reseta o estado para permitir nova detecção
  }
}

void verificarInterruptores() {
  int estadoAtualDMM = !digitalRead(interruptorDMM);
  int estadoAtualBO = !digitalRead(interruptorBO);

  sistemaAtivo = estadoAtualDMM && estadoAtualBO;

  if (sistemaAtivo && ultimoSistemaAtivo == LOW) {
    Serial.println("inicio de avance");
    inicioTempoInterruptor = millis();
    resetarVelocidade();
    velocidadeExibida = false; // Permite calcular novamente quando o sistema estiver ativo
  }

  if (!estadoAtualBO && ultimoSistemaAtivo == HIGH) {
  duracaoInterruptor = millis() - inicioTempoInterruptor;
  Serial.println("fim de avance");
  mostrarRelatorio();
  salvarDadosEEPROM();
  resetarContagemBotoes();
  }

  ultimoSistemaAtivo = sistemaAtivo;
}

void resetarContagemBotoes() {
  // Zera as contagens dos botões
  memset(contagemBotoes, 0, sizeof(contagemBotoes));
  
  // Zera as contagens de eixos e rodagem dupla
  memset(contadoresEixosA, 0, sizeof(contadoresEixosA));
  memset(contadoresEixosB, 0, sizeof(contadoresEixosB));
  memset(contadoresRodagemDupla, 0, sizeof(contadoresRodagemDupla));
}

//debounce
unsigned long tempoDebounce = 50; // Define o tempo mínimo de debounce (em milissegundos)

// Adiciona arrays para armazenar o último tempo de mudança dos botões
unsigned long ultimoTempoBotaoA[4] = {0, 0, 0, 0};
unsigned long ultimoTempoBotaoB[4] = {0, 0, 0, 0};

// Função de debounce para verificar se o botão foi realmente pressionado
bool debounce(int pinoBotao, unsigned long &ultimoTempo, int &ultimoEstadoBotao) {
  int estadoAtual = !digitalRead(pinoBotao);
  unsigned long tempoAtual = millis();

  // Verifica se o estado mudou e se passou o tempo mínimo para debounce
  if (estadoAtual != ultimoEstadoBotao) {
    if (tempoAtual - ultimoTempo > tempoDebounce) {
      ultimoEstadoBotao = estadoAtual;  // Atualiza o estado do botão
      ultimoTempo = tempoAtual;         // Atualiza o tempo da última mudança
      return true;                      // Indica que houve uma mudança válida
    }
  }
  
  return false; // Sem mudança válida
}

void verificarSequencia(const int pinosBotoes[], int estadosBotoes[], unsigned long temposBotoes[], const char* lado, int &estadoSequencia) {
  for (int i = 0; i < 4; i++) {
    // Usa a função de debounce para cada botão
    if (debounce(pinosBotoes[i], (lado == "A" ? ultimoTempoBotaoA[i] : ultimoTempoBotaoB[i]), estadosBotoes[i])) {
      if (estadosBotoes[i] == HIGH) {
        temposBotoes[i] = millis();  // Registra o tempo de ativação
        contagemBotoes[i + (lado == "B" ? 4 : 0)]++; // Incrementa o contador correspondente ao botão

        if (estadoSequencia == i) {
          estadoSequencia++; // Avança na sequência
          Serial.print("# Sequência correta: Botão ");
          Serial.print(i + 1);
          Serial.print(" (lado ");
          Serial.print(lado);
          Serial.println(")!");

          if (lado == "A") {
            if (i < 2) {
              contadoresEixosA[i]++;
            }
          } else {
            if (i < 2) {
              contadoresEixosB[i]++;
            }
          }

          if (i == 3) { // Após RodDupla2, reinicia a sequência para o próximo eixo
            estadoSequencia = 0;
            Serial.print("# Rodagem dupla completa no lado ");
            Serial.println(lado);
            Serial.println("!");
          }

          if (i >= 2) {  // Verifica velocidade em rodagem dupla
            if (lado == "A") velocidadeComBotaoRodDupla[0][i - 2] = true;
            else velocidadeComBotaoRodDupla[1][i - 2] = true;
          }
        } else if (estadoSequencia > i) {
          Serial.print("# Retrocesso detectado no lado ");
          Serial.println(lado);
          Serial.println("!");

          // Diminui os contadores de eixo correspondentes
          if (lado == "A") {
            if (estadoSequencia == 2 && contadoresEixosA[1] > 0) {
              contadoresEixosA[1]--;
            }
            if (estadoSequencia == 1 && contadoresEixosA[0] > 0) {
              contadoresEixosA[0]--;
            }
          } else {
            if (estadoSequencia == 2 && contadoresEixosB[1] > 0) {
              contadoresEixosB[1]--;
            }
            if (estadoSequencia == 1 && contadoresEixosB[0] > 0) {
              contadoresEixosB[0]--;
            }
          }
          estadoSequencia--; // Volta na sequência
          retrocessoDetectado = true;
        }
      }
    }
  }
}

void calcularVelocidade() {
  if (!velocidadeExibida) { // Garante que a velocidade será exibida apenas uma vez
    for (int i = 0; i < 2; i++) {
      if (velocidadeComBotaoRodDupla[i][0] && velocidadeComBotaoRodDupla[i][1]) {
        unsigned long tempoTotal = (i == 0 ? temposBotoesA[3] : temposBotoesB[3]) - (i == 0 ? temposBotoesA[2] : temposBotoesB[2]);
        if (tempoTotal > 0) {
          float velocidade = (distanciaSensores / (tempoTotal / 1000.0)) * 3.6; // Converte m/s para km/h
          Serial.print("<Velocidade do veículo no lado ");
          Serial.print(i == 0 ? "A" : "B");
          Serial.print(": ");
          Serial.print(velocidade);
          Serial.println(" km/h>");
          velocidadeExibida = true; // Marca que a velocidade foi exibida
        }
      }
    }
  }
}