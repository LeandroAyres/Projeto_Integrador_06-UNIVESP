#include <WiFi.h>
#include <esp_now.h>
#include <SPIFFS.h>
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourceSPIFFS.h"

// ==================== CONFIGURAÇÕES DE HARDWARE ====================
// Pinos do MAX98357A
#define I2S_BCLK  27
#define I2S_LRC   26
#define I2S_DOUT  25

// Pino de detecção do fone (jack)
#define PIN_JACK_DETECT  32

// LEDs de indicação (opcional)
#define LED_ATIVIDADE  23

// ==================== OBJETOS DE ÁUDIO ====================
AudioGeneratorWAV *wav;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;

// ==================== ESP-NOW ====================
typedef struct struct_message {
  uint8_t comando;
} struct_message;
struct_message myData;

// ==================== VARIÁVEIS GLOBAIS ====================
// Mapeamento simples: comando (1 a 6) -> nome do arquivo .wav
// (futuramente, isso virá de um arquivo config.json)
const char* audioFiles[] = {
  "/01.wav",   // comando 1
  "/02.wav",   // comando 2
  "/03.wav",   // comando 3
  "/04.wav",   // comando 4
  "/05.wav",   // comando 5
  "/06.wav"    // comando 6
};
const int NUM_COMANDOS = 6;

// Controle de reprodução
bool tocando = false;
int ultimoComando = 0;

// ==================== CALLBACK DO ESP-NOW ====================
void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("📥 Comando recebido: %d\n", myData.comando);

  // Só processa comandos válidos (1 a 6)
  if (myData.comando >= 1 && myData.comando <= NUM_COMANDOS) {
    // Se já estiver tocando algo, interrompe (opcional: pode deixar terminar)
    // Vamos parar o atual e tocar o novo imediatamente
    if (wav != nullptr && wav->isRunning()) {
      wav->stop();
      delete wav;
      wav = nullptr;
    }
    if (file != nullptr) {
      delete file;
      file = nullptr;
    }

    // Acende o LED de atividade (feedback visual)
    digitalWrite(LED_ATIVIDADE, HIGH);

    // Toca o áudio correspondente
    tocarComando(myData.comando);

    // Apaga o LED após um breve momento (pode ser mantido enquanto toca)
    // Neste exemplo, desligamos após 200ms, mas você pode deixar ligado
    // e desligar no callback de fim de áudio (mais adiante)
    delay(200);
    digitalWrite(LED_ATIVIDADE, LOW);
  } else {
    Serial.println("⚠️ Comando inválido ou fora do range (1-6)");
  }
}

// ==================== FUNÇÃO PARA TOCAR UM COMANDO ====================
void tocarComando(int comando) {
  if (comando < 1 || comando > NUM_COMANDOS) {
    Serial.printf("❌ Comando %d fora do range\n", comando);
    return;
  }

  const char* caminho = audioFiles[comando - 1]; // índice 0 = comando 1
  Serial.printf("🔊 Tocando: %s\n", caminho);

  file = new AudioFileSourceSPIFFS(caminho);
  if (!file->isOpen()) {
    Serial.printf("❌ ERRO: não foi possível abrir %s\n", caminho);
    delete file;
    file = nullptr;
    return;
  }

  wav = new AudioGeneratorWAV();
  wav->begin(file, out);
  tocando = true;
  ultimoComando = comando;
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== RECEPTOR COM ÁUDIO E ESP-NOW ===");

  // ---- Configuração do LED de atividade ----
  pinMode(LED_ATIVIDADE, OUTPUT);
  digitalWrite(LED_ATIVIDADE, LOW);

  // ---- Configuração do pino de detecção do fone ----
  pinMode(PIN_JACK_DETECT, INPUT_PULLUP);
  Serial.println("Pino de detecção do jack configurado (GPIO32)");

  // ---- Inicialização do SPIFFS (sistema de arquivos) ----
  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Falha ao montar SPIFFS.");
    return;
  }
  Serial.println("✅ SPIFFS montado com sucesso!");

  // ---- Configuração do áudio (I2S) ----
  out = new AudioOutputI2S(0, 1);  // I2S_NUM_0, mono
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  // Define o ganho inicial conforme o estado do jack
  atualizarGanho();

  // ---- Configuração do Wi-Fi em modo estação (obrigatório para ESP-NOW) ----
  WiFi.mode(WIFI_STA);

  // ---- Inicialização do ESP-NOW ----
  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Falha ao inicializar ESP-NOW");
    return;
  }
  Serial.println("✅ ESP-NOW iniciado");

  // ---- Registro do callback de recebimento ----
  esp_now_register_recv_cb(OnDataRecv);

  // ---- Exibe o MAC address do receptor (para configurar no transmissor) ----
  Serial.print("📡 MAC do receptor: ");
  Serial.println(WiFi.macAddress());
  Serial.println("✅ Aguardando comandos...");
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  // ---- Verifica se o estado do fone mudou (para ajustar o volume) ----
  static int ultimoEstado = -1;
  int estadoAtual = digitalRead(PIN_JACK_DETECT);
  if (estadoAtual != ultimoEstado) {
    ultimoEstado = estadoAtual;
    atualizarGanho();
    Serial.print("🎧 Jack detectado: ");
    Serial.println(estadoAtual == HIGH ? "Fone conectado" : "Alto-falante");
  }

  // ---- Mantém a reprodução de áudio ativa ----
  if (wav != nullptr && wav->isRunning()) {
    wav->loop();
  } else {
    // Se terminou a reprodução, libera os objetos e indica que não está tocando
    if (tocando) {
      tocando = false;
      Serial.println("🔇 Fim da reprodução.");
      // Aqui você pode, por exemplo, enviar um ACK para o transmissor
      // ou registrar o log localmente
    }
    // Pequeno delay para não sobrecarregar o loop
    delay(10);
  }
}

// ==================== FUNÇÃO DE ATUALIZAÇÃO DO GANHO ====================
void atualizarGanho() {
  // Lógica: HIGH = fone conectado, LOW = sem fone
  bool foneConectado = (digitalRead(PIN_JACK_DETECT) == HIGH);
  if (foneConectado) {
    out->SetGain(0.4);   // volume mais baixo para fone
    Serial.println("🔊 Ganho ajustado para FONE (0.4)");
  } else {
    out->SetGain(1.0);   // volume máximo para alto-falante
    Serial.println("🔊 Ganho ajustado para ALTO-FALANTE (1.0)");
  }
}