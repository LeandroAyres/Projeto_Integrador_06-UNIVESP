#include "driver/i2s.h"

// Pinos I2S – ajuste se necessário
#define I2S_BCLK  27
#define I2S_LRC   26
#define I2S_DOUT  25

// Configuração do I2S
#define SAMPLE_RATE  44100
#define AMPLITUDE    10000   // volume (0 a 32767)

void setup() {
  Serial.begin(115200);
  Serial.println("Iniciando teste do MAX98357A...");

  // Configura o I2S como mestre para saída
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,   // o MAX98357A usa mono
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = 0,
    .dma_buf_count = 8,
    .dma_buf_len = 64,
    .use_apll = false
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
  i2s_set_pin(I2S_NUM_0, &pin_config);
  i2s_set_clk(I2S_NUM_0, SAMPLE_RATE, I2S_BITS_PER_SAMPLE_16BIT, I2S_CHANNEL_MONO);

  Serial.println("I2S configurado. Tocando tom de 440 Hz por 5 segundos...");
}

void loop() {
  // Gera um tom de 440 Hz (senoide) em mono
  int16_t sample[64];                // buffer de 64 amostras
  static float phase = 0.0;
  const float freq = 440.0;
  const float increment = 2.0 * PI * freq / SAMPLE_RATE;

  for (int i = 0; i < 64; i++) {
    sample[i] = (int16_t)(sin(phase) * AMPLITUDE);
    phase += increment;
    if (phase >= 2.0 * PI) phase -= 2.0 * PI;
  }

  // Envia o buffer via I2S
  size_t bytes_written;
  i2s_write(I2S_NUM_0, sample, sizeof(sample), &bytes_written, portMAX_DELAY);

  // Opcional: imprime algo no serial a cada 100 buffers
  static unsigned long lastPrint = 0;
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    Serial.println("Tocando... (440 Hz)");
  }
}