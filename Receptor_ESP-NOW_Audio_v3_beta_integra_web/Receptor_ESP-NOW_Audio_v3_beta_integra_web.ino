#include <WiFi.h>
#include <esp_now.h>
#include <SPIFFS.h>
#include <WebServer.h>
#include <esp_wifi.h>   
#include "AudioGeneratorWAV.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourceSPIFFS.h"

// ==================== HARDWARE ====================
#define I2S_BCLK  27
#define I2S_LRC   26
#define I2S_DOUT  25
#define PIN_JACK_DETECT  32
#define LED_ATIVIDADE    23
#define PIN_BOTAO_SYNC   4   

// ==================== OBJETOS ====================
AudioGeneratorWAV *wav;
AudioFileSourceSPIFFS *file;
AudioOutputI2S *out;
WebServer server(80);

// ==================== ESP-NOW ====================
typedef struct struct_message {
  uint8_t comando;
} struct_message;
struct_message myData;

// ==================== VARIÁVEIS GLOBAIS ====================
const char* audioFiles[] = {"/01.wav", "/02.wav", "/03.wav", "/04.wav", "/05.wav", "/06.wav"};
const int NUM_COMANDOS = 6;
bool tocando = false;
bool modoSincronizacao = false;

// ==================== FUNÇÕES DE ÁUDIO E ESP-NOW ====================
void atualizarGanho() {
  bool foneConectado = (digitalRead(PIN_JACK_DETECT) == HIGH);
  if (foneConectado) {
    out->SetGain(0.4);
  } else {
    out->SetGain(1.0);
  }
}

void tocarComando(int comando) {
  if (comando < 1 || comando > NUM_COMANDOS) return;
  const char* caminho = audioFiles[comando - 1];
  Serial.printf("🔊 Tocando: %s\n", caminho);

  file = new AudioFileSourceSPIFFS(caminho);
  if (!file->isOpen()) {
    delete file; file = nullptr;
    return;
  }
  wav = new AudioGeneratorWAV();
  wav->begin(file, out);
  tocando = true;
}

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("📥 Comando recebido: %d\n", myData.comando);

  if (myData.comando >= 1 && myData.comando <= NUM_COMANDOS) {
    
    // --- 1. SALVA NO CARTÃO SD (SPIFFS) IMEDIATAMENTE ---
    // Usamos FILE_APPEND para adicionar ao final sem apagar o que já existe
    File logFile = SPIFFS.open("/temp_logs.txt", FILE_APPEND);
    if (logFile) {
      logFile.print(String(myData.comando) + ",");
      logFile.close();
    } else {
      Serial.println("❌ Erro ao salvar log no SPIFFS.");
    }

    // --- 2. TOCA O ÁUDIO ---
    if (wav != nullptr && wav->isRunning()) {
      wav->stop(); delete wav; wav = nullptr;
    }
    if (file != nullptr) { delete file; file = nullptr; }

    digitalWrite(LED_ATIVIDADE, HIGH);
    tocarComando(myData.comando);
    delay(200);
    digitalWrite(LED_ATIVIDADE, LOW);
  }
}

void iniciarModoPartida() {
  Serial.println("\n⚽ MODO PARTIDA (OFFLINE 100%) ⚽");
  
  // Apaga logs da partida anterior ao iniciar uma nova
  if (SPIFFS.exists("/temp_logs.txt")) {
    SPIFFS.remove("/temp_logs.txt");
    Serial.println("🗑️ Logs antigos limpos. Pronto para nova partida.");
  }

  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
  delay(100);

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Falha ao inicializar ESP-NOW");
    return;
  }
  
  esp_now_register_recv_cb(OnDataRecv);
  Serial.println("✅ Aguardando comandos...");
}

// ==================== FUNÇÕES DO MODO SINCRONIZAÇÃO ====================
void handleRoot() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Receptor</title></head>";
  html += "<body><h1>Receptor - Sincronização</h1><p>Use o Dashboard HTML para baixar.</p></body></html>";
  server.send(200, "text/html", html);
}

void handleDownloadLogs() {
  File arquivo = SPIFFS.open("/metricas.json", "r");
  
  if (!arquivo) {
    server.send(404, "text/plain", "Nenhuma metrica encontrada.");
    return;
  }

  String conteudo = arquivo.readString();
  arquivo.close();

  Serial.println("📤 Enviando metricas.json para o navegador...");
  
  // Cabeçalhos para evitar cache no navegador e permitir acesso externo
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
  server.sendHeader("Content-Type", "application/json");
  server.send(200, "application/json", conteudo);
}

void iniciarModoSincronizacao() {
  Serial.println("\n📶 MODO SINCRONIZAÇÃO (WEB) 📶");
  
  // --- 1. PROCESSA OS DADOS DO ARQUIVO TEMPORÁRIO ---
  String logFinal = "[]";
  if (SPIFFS.exists("/temp_logs.txt")) {
    File fTemp = SPIFFS.open("/temp_logs.txt", "r");
    if (fTemp) {
      String conteudo = fTemp.readString();
      fTemp.close();
      
      // Remove a última vírgula sobrando
      if (conteudo.endsWith(",")) {
        conteudo.remove(conteudo.length() - 1);
      }
      logFinal = "[" + conteudo + "]";
    }
  }

  // Grava o arquivo JSON formatado final
  File fFinal = SPIFFS.open("/metricas.json", "w");
  if (fFinal) {
    fFinal.print(logFinal);
    fFinal.close();
    Serial.println("💾 JSON processado e salvo com sucesso!");
  }

  // --- 2. INICIA O ACCESS POINT ---
  WiFi.mode(WIFI_AP);
  delay(100); 
  WiFi.softAP("Receptor-Audio-Config", "12345678");
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("✅ Rede criada. Acesse: http://");
  Serial.println(IP);

  server.on("/", handleRoot);
  server.on("/download_logs", handleDownloadLogs);
  server.begin();
}

// ==================== SETUP ====================
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== RECEPTOR V6 - GRAVAÇÃO DIRETA NO SD ===");

  pinMode(LED_ATIVIDADE, OUTPUT);
  pinMode(PIN_JACK_DETECT, INPUT_PULLUP);
  pinMode(PIN_BOTAO_SYNC, INPUT_PULLUP); 
  digitalWrite(LED_ATIVIDADE, LOW);

  if (!SPIFFS.begin(true)) {
    Serial.println("❌ Falha ao montar SPIFFS.");
    return;
  }

  out = new AudioOutputI2S(0, 1);
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  atualizarGanho();

  if (digitalRead(PIN_BOTAO_SYNC) == LOW) {
    modoSincronizacao = true;
    iniciarModoSincronizacao();
  } else {
    modoSincronizacao = false;
    iniciarModoPartida();
  }
}

// ==================== LOOP PRINCIPAL ====================
void loop() {
  if (modoSincronizacao) {
    server.handleClient();
  } else {
    static int ultimoEstado = -1;
    int estadoAtual = digitalRead(PIN_JACK_DETECT);
    if (estadoAtual != ultimoEstado) {
      ultimoEstado = estadoAtual;
      atualizarGanho();
    }

    if (wav != nullptr && wav->isRunning()) {
      wav->loop();
    } else {
      if (tocando) {
        tocando = false;
      }
      delay(10);
    }
  }
}