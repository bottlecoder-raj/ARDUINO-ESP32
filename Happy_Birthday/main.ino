#define BUZZER 32

// Notes
#define C4  262
#define D4  294
#define E4  330
#define F4  349
#define G4  392
#define A4  440
#define B4  494

#define C5  523
#define D5  587
#define E5  659
#define F5  698
#define G5  784

#define BUZZER_CHANNEL 0

void playNote(int frequency, int duration) {

  if (frequency == 0) {
    ledcWrite(BUZZER, 0);
    delay(duration);
    return;
  }

  // ESP32 Arduino Core 3.x
  ledcAttach(BUZZER, frequency, 8);

  // 50% duty cycle
  ledcWrite(BUZZER, 128);

  delay(duration);

  ledcWrite(BUZZER, 0);
  ledcDetach(BUZZER);

  delay(40);
}

void happyBirthday() {

  playNote(G4, 250);
  playNote(G4, 250);
  playNote(A4, 500);
  playNote(G4, 500);
  playNote(C5, 500);
  playNote(B4, 900);

  delay(100);

  playNote(G4, 250);
  playNote(G4, 250);
  playNote(A4, 500);
  playNote(G4, 500);
  playNote(D5, 500);
  playNote(C5, 900);

  delay(100);

  playNote(G4, 250);
  playNote(G4, 250);
  playNote(G5, 500);
  playNote(E5, 500);
  playNote(C5, 500);
  playNote(B4, 500);
  playNote(A4, 900);

  delay(100);

  playNote(F5, 250);
  playNote(F5, 250);
  playNote(E5, 500);
  playNote(C5, 500);
  playNote(D5, 500);
  playNote(C5, 1000);
}

void setup() {
  happyBirthday();
}

void loop() {
}
