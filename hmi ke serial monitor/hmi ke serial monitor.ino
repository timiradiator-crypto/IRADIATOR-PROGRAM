#include <SoftwareSerial.h>

// Mendefinisikan pin RX dan TX tambahan
// Pin 2 sebagai RX (menerima data dari TX perangkat luar)
// Pin 3 sebagai TX (mengirim data ke RX perangkat luar)
SoftwareSerial mySerial(2, 3); 

void setup() {
  // Memulai komunikasi Serial Monitor komputer
  Serial.begin(9600);
  
  // Memulai komunikasi serial dengan perangkat luar melalui pin 2 dan 3
  // Sesuaikan baud rate (misal: 9600) dengan kecepatan perangkat Anda
  mySerial.begin(9600);
  
  Serial.println("Sistem Siap. Menunggu data dari pin 2 (RX) dan 3 (TX)...");
}

void loop() {
  // Mengecek apakah ada data yang masuk melalui SoftwareSerial (Pin 2 & 3)
  if (mySerial.available() > 0) {
    byte dataMasuk = mySerial.read();

    // Menampilkan format dua digit heksadesimal ke Serial Monitor
    if (dataMasuk < 16) {
      Serial.print("0"); // Menambahkan angka 0 di depan jika nilai kurang dari 0x10
    }
    Serial.print(dataMasuk, HEX);
    Serial.print(" "); // Memberikan spasi antar byte agar menyerupai tampilan simulator
  }
}
