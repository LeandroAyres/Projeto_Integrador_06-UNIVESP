int speakerPin = 25; // Pino com DAC ou PWM

void setup() {
  pinMode(speakerPin, OUTPUT);
}

void loop() {
  // Toca um tom de 1000Hz por 500ms
  tone(speakerPin, 1000);
  delay(500);
  noTone(speakerPin);
  delay(500);
  
  // Toca um tom mais grave (600Hz)
  tone(speakerPin, 600);
  delay(500);
  noTone(speakerPin);
  delay(1000);
}