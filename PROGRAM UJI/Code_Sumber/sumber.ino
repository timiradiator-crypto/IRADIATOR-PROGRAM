#include <AccelStepper.h>

// Definisi Pin Driver ke Arduino Mega 2560 (Tanpa ENA)
const int stepPin = 3;
const int dirPin  = 4;

// Inisialisasi objek stepper dengan mode DRIVER
AccelStepper stepper(AccelStepper::DRIVER, stepPin, dirPin);

// Variabel untuk menyimpan target kecepatan
float targetSpeed = 0;

void setup() {
  // Mulai komunikasi Serial dengan Baud Rate 9600
  Serial.begin(9600);

  // Atur kecepatan maksimal dan akselerasi motor
  stepper.setMaxSpeed(100.0);    
  stepper.setAcceleration(50.0); 

  // Petunjuk di Serial Monitor
  Serial.println("=== Kontrol Stepper Motor via Serial (Tanpa ENA) ===");
  Serial.println("Ketik 'n' -> Naik");
  Serial.println("Ketik 't' -> Turun");
  Serial.println("Ketik 's' -> Stop (Berhenti)");
  Serial.println("==================================================");
}

void loop() {
  // Cek apakah ada data yang masuk dari Serial Monitor
  if (Serial.available() > 0) {
    char perintah = Serial.read(); // Baca karakter yang diketik

    // Abaikan karakter enter (\n atau \r)
    if (perintah != '\n' && perintah != '\r') {
      
      // Cek huruf (mendukung huruf kecil maupun kapital)
      if (perintah == 'n' || perintah == 'N') {
        targetSpeed = 1000.0; // Kecepatan arah Naik (Positif)
        Serial.println("Perintah: NAIK");
      } 
      else if (perintah == 't' || perintah == 'T') {
        targetSpeed = -1000.0; // Kecepatan arah Turun (Negatif)
        Serial.println("Perintah: TURUN");
      } 
      else if (perintah == 's' || perintah == 'S') {
        targetSpeed = 0; // Berhenti
        Serial.println("Perintah: STOP");
      }
    }
  }

  // Terapkan kecepatan ke motor secara terus-menerus
  stepper.setSpeed(targetSpeed);
  stepper.runSpeed();
}
