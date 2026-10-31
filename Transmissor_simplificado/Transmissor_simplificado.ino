#include <WiFi.h>
#include <esp_now.h>

// ========== MAC DO RECEPTOR (SUBSTITUA PELO SEU) ==========
uint8_t broadcastAddress[] = {0x40, 0x91, 0x51, 0xAB, 0x20, 0xC8}; // <-- ATUALIZE!

// ========== DEFINIÇÃO DOS PINOS (use pinos seguros) ==========
#define BTN_PIN_1  13
#define BTN_PIN_2  14
#define BTN_PIN_3  4
#define BTN_PIN_4  5
#define BTN_PIN_5  18
#define BTN_PIN_6  19
#define LED_ENVIO  23

typedef struct struct_message {
  uint8_t comando;
} struct_message;
struct_message myData;

bool peerAdded = false;

// ========== CALLBACK DE ENVIO ==========
void OnDataSent(const esp_now_send_info_t *info, esp_now_send_status_t status) {
  digitalWrite(LED_ENVIO, status == ESP_NOW_SEND_SUCCESS ? HIGH : LOW);
  delay(50);
  digitalWrite(LED_ENVIO, LOW);
  Serial.print(status == ESP_NOW_SEND_SUCCESS ? "✅ " : "❌ ");
  Serial.println("Envio");
}

// ========== SETUP ==========
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== TRANSMISSOR (com detecção de borda) ===");

  // Configura botões com pull-up
  pinMode(BTN_PIN_1, INPUT_PULLUP);
  pinMode(BTN_PIN_2, INPUT_PULLUP);
  pinMode(BTN_PIN_3, INPUT_PULLUP);
  pinMode(BTN_PIN_4, INPUT_PULLUP);
  pinMode(BTN_PIN_5, INPUT_PULLUP);
  pinMode(BTN_PIN_6, INPUT_PULLUP);
  pinMode(LED_ENVIO, OUTPUT);
  digitalWrite(LED_ENVIO, LOW);

  WiFi.mode(WIFI_STA);
  delay(100);

  Serial.print("📡 MAC do transmissor: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("❌ Falha ao iniciar ESP-NOW");
    return;
  }
  Serial.println("✅ ESP-NOW iniciado");

  esp_now_register_send_cb(OnDataSent);

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.ifidx = WIFI_IF_STA;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("✅ Peer adicionado!");
    peerAdded = true;
  } else {
    Serial.println("❌ Falha ao adicionar peer. Verifique o MAC.");
  }
}

// ========== FUNÇÃO AUXILIAR PARA ENVIAR COMANDO ==========
void enviarComando(uint8_t comando, const char* nome) {
  if (!peerAdded) return;
  myData.comando = comando;
  esp_now_send(broadcastAddress, (uint8_t*)&myData, sizeof(myData));
  Serial.printf("🔘 Botão %d pressionado (%s)\n", comando, nome);
}

// ========== LOOP COM DETECÇÃO DE BORDA ==========
void loop() {
  if (!peerAdded) {
    delay(1000);
    return;
  }

  // Variáveis estáticas para guardar o estado anterior de cada botão
  static int ultimoEstado1 = HIGH;
  static int ultimoEstado2 = HIGH;
  static int ultimoEstado3 = HIGH;
  static int ultimoEstado4 = HIGH;
  static int ultimoEstado5 = HIGH;
  static int ultimoEstado6 = HIGH;

  int estado1 = digitalRead(BTN_PIN_1);
  int estado2 = digitalRead(BTN_PIN_2);
  int estado3 = digitalRead(BTN_PIN_3);
  int estado4 = digitalRead(BTN_PIN_4);
  int estado5 = digitalRead(BTN_PIN_5);
  int estado6 = digitalRead(BTN_PIN_6);

  // Detecta borda de descida (HIGH -> LOW)
  if (estado1 == LOW && ultimoEstado1 == HIGH) {
    enviarComando(1, "Reta Alta");
    delay(50); // pequeno debounce
  }
  ultimoEstado1 = estado1;

  if (estado2 == LOW && ultimoEstado2 == HIGH) {
    enviarComando(2, "Curva");
    delay(50);
  }
  ultimoEstado2 = estado2;

  if (estado3 == LOW && ultimoEstado3 == HIGH) {
    enviarComando(3, "Mudança");
    delay(50);
  }
  ultimoEstado3 = estado3;

  if (estado4 == LOW && ultimoEstado4 == HIGH) {
    enviarComando(4, "Alta");
    delay(50);
  }
  ultimoEstado4 = estado4;

  if (estado5 == LOW && ultimoEstado5 == HIGH) {
    enviarComando(5, "Baixa");
    delay(50);
  }
  ultimoEstado5 = estado5;

  if (estado6 == LOW && ultimoEstado6 == HIGH) {
    enviarComando(6, "Externa");
    delay(50);
  }
  ultimoEstado6 = estado6;

  delay(10); // pequena pausa para evitar uso excessivo da CPU
}