#include <WiFi.h>
#include <esp_now.h>

#define PIN_HABILITA_WIFI  0   // GPIO0 – se puxado para GND, ativa Wi-Fi (opcional)

// Pinos dos LEDs
#define LED_CMD_1  13
#define LED_CMD_2  14
#define LED_CMD_3  16
#define LED_CMD_4  17
#define LED_CMD_5  18
#define LED_CMD_6  19
#define LED_ATIVIDADE  23   // mesmo pino do trasmissor

typedef struct struct_message {
  uint8_t comando;
} struct_message;
struct_message myData;

void OnDataRecv(const esp_now_recv_info *info, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.printf("Comando: %d\n", myData.comando);

  // Apaga todos
  digitalWrite(LED_CMD_1, LOW);
  digitalWrite(LED_CMD_2, LOW);
  digitalWrite(LED_CMD_3, LOW);
  digitalWrite(LED_CMD_4, LOW);
  digitalWrite(LED_CMD_5, LOW);
  digitalWrite(LED_CMD_6, LOW);

  switch (myData.comando) {
    case 1: digitalWrite(LED_CMD_1, HIGH); break;
    case 2: digitalWrite(LED_CMD_2, HIGH); break;
    case 3: digitalWrite(LED_CMD_3, HIGH); break;
    case 4: digitalWrite(LED_CMD_4, HIGH); break;
    case 5: digitalWrite(LED_CMD_5, HIGH); break;
    case 6: digitalWrite(LED_CMD_6, HIGH); break;
  }

  digitalWrite(LED_ATIVIDADE, HIGH);
  delay(200);
  digitalWrite(LED_ATIVIDADE, LOW);
}

void setup() {
  Serial.begin(115200);
  Serial.println("Receptor ESP-NOW iniciado");

  // Configura LEDs
  pinMode(LED_CMD_1, OUTPUT);
  pinMode(LED_CMD_2, OUTPUT);
  pinMode(LED_CMD_3, OUTPUT);
  pinMode(LED_CMD_4, OUTPUT);
  pinMode(LED_CMD_5, OUTPUT);
  pinMode(LED_CMD_6, OUTPUT);
  pinMode(LED_ATIVIDADE, OUTPUT);
  // Inicializa todos apagados
  digitalWrite(LED_CMD_1, LOW);
  digitalWrite(LED_CMD_2, LOW);
  digitalWrite(LED_CMD_3, LOW);
  digitalWrite(LED_CMD_4, LOW);
  digitalWrite(LED_CMD_5, LOW);
  digitalWrite(LED_CMD_6, LOW);
  digitalWrite(LED_ATIVIDADE, LOW);

  // Inicializa WiFi em modo estação (necessário para ESP-NOW)
  WiFi.mode(WIFI_STA);
  
  // Inicializa ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Erro ao inicializar ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Aguardando comandos...");
}

void loop() {
  // Nada aqui – tudo é tratado no callback
  delay(10);
}