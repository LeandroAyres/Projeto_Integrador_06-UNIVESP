#include <WiFi.h>
#include <esp_now.h>

// ========== MAC DO RECEPTOR (SUBSTITUA PELO SEU) ==========
uint8_t broadcastAddress[] = {0x40, 0x91, 0x51, 0xAB, 0x20, 0xC8}; // <-- ATUALIZE! 40:91:51:ab:20:c8 é o correto 

// ========== DEFINIÇÃO DOS PINOS ==========
#define BTN_PIN_1  13
#define BTN_PIN_2  14
#define BTN_PIN_3  16
#define BTN_PIN_4  17
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
  Serial.println("\n=== TRANSMISSOR ===");

  // Configura botões com pull-up interno
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

// ========== LOOP ==========
void loop() {
  if (!peerAdded) {
    delay(1000);
    return;
  }

  // Leitura dos botões com debounce simples
  static unsigned long lastPressTime = 0;
  if (millis() - lastPressTime < 300) return; // evita múltiplos envios

  if (digitalRead(BTN_PIN_1) == LOW) {
    myData.comando = 1;
    esp_now_send(broadcastAddress, (uint8_t*)&myData, sizeof(myData));
    Serial.println("🔘 Botão 1 pressionado (Reta Alta)");
    lastPressTime = millis();
  }
  else if (digitalRead(BTN_PIN_2) == LOW) {
    myData.comando = 2;
    esp_now_send(broadcastAddress, (uint8_t*)&myData, sizeof(myData));
    Serial.println("🔘 Botão 2 pressionado (Curva)");
    lastPressTime = millis();
  }
  else if (digitalRead(BTN_PIN_3) == LOW) {
    myData.comando = 3;
    esp_now_send(broadcastAddress, (uint8_t*)&myData, sizeof(myData));
    Serial.println("🔘 Botão 3 pressionado (Mudança)");
    lastPressTime = millis();
  }
  else if (digitalRead(BTN_PIN_4) == LOW) {
    myData.comando = 4;
    esp_now_send(broadcastAddress, (uint8_t*)&myData, sizeof(myData));
    Serial.println("🔘 Botão 4 pressionado (Alta)");
    lastPressTime = millis();
  }
  else if (digitalRead(BTN_PIN_5) == LOW) {
    myData.comando = 5;
    esp_now_send(broadcastAddress, (uint8_t*)&myData, sizeof(myData));
    Serial.println("🔘 Botão 5 pressionado (Baixa)");
    lastPressTime = millis();
  }
  else if (digitalRead(BTN_PIN_6) == LOW) {
    myData.comando = 6;
    esp_now_send(broadcastAddress, (uint8_t*)&myData, sizeof(myData));
    Serial.println("🔘 Botão 6 pressionado (Externa)");
    lastPressTime = millis();
  }

  delay(10);
}