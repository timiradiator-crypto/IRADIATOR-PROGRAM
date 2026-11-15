/*
 * ============================================================================
 * PROYEK: KONTROL MOTOR BTS7960 DENGAN ESP32-C3 (ESP 2 - SLAVE / PENERIMA)
 * ============================================================================
 * Fitur:
 * 1. Menerima sinyal pemicu (trigger) dari ESP 1 via komunikasi ESP-NOW.
 * 2. Menyalakan motor driver BTS7960 dengan durasi yang sama persis seperti
 *    yang dikirimkan oleh ESP 1.
 * 3. Setelah waktu durasi habis, motor otomatis berhenti dan siap menerima
 *    perintah berikutnya.
 * ============================================================================
 * 
 * PINOUT ESP32-C3 KE DRIVER BTS7960 (ESP 2):
 * ----------------------------------------------------------------------------
 * MOTOR BTS7960:
 * - GPIO 4 -> RPWM (Maju)
 * - GPIO 3 -> LPWM (Mundur)
 * - GPIO 6 -> R_EN & L_EN (Pin Enable / PWM Kecepatan)
 * 
 * (Catatan: Jika Anda menggunakan pin yang berbeda di ESP 2, cukup ubah konstanta
 *  pin di bawah ini).
 * 
 * CATATAN DAYA (POWER SUPPLY & COMMON GROUND):
 * - VCC BTS7960 -> 5V (Bisa dari 5V/VIN ESP atau Power Supply 5V)
 * - GND BTS7960 -> GND ESP32-C3 DAN Kutub Negatif Power Supply Motor (Wajib Common GND!)
 * - B+ / B-     -> Kutub Positif & Negatif Baterai/Aki/Power Supply Motor (12V - 24V)
 * - M+ / M-     -> Terminal Motor DC
 * ============================================================================
 */

#include <WiFi.h>
#include <esp_now.h>

// ============================================================================
// DEFINISI PIN MOTOR DRIVER BTS7960 (ESP 2)
// ============================================================================
const int PIN_RPWM = 4; // GPIO 4: PWM Maju
const int PIN_LPWM = 3; // GPIO 3: PWM Mundur
const int PIN_PWM  = 6; // GPIO 6: Enable (R_EN & L_EN digabung)

// ============================================================================
// STRUKTUR DATA ESP-NOW (Harus identik dengan ESP 1)
// ============================================================================
typedef struct struct_message {
  uint8_t command;      // 1: Jalankan motor, 0: Berhenti
  uint32_t duration_ms; // Durasi dalam milidetik (mengikuti ESP 1)
  uint8_t speed;        // Nilai kecepatan PWM (0-255)
} struct_message;

struct_message dataTerima;

// Flag dan variabel kontrol motor
volatile bool perintahDiterima = false;
bool motorSedangAktif = false;
unsigned long waktuMulaiMotor = 0;
uint32_t durasiTargetMs = 0;
int sisaDetikTerakhir = -1;

// ============================================================================
// DEKLARASI FUNGSI
// ============================================================================
void inisialisasiPinMotor();
void motorMaju(int speed);
void motorMundur(int speed);
void motorBerhenti();

// ============================================================================
// CALLBACK PENERIMA ESP-NOW (Kompatibel dengan Core ESP32 v2.x & v3.x)
// ============================================================================
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
void OnDataRecv(const esp_now_recv_info_t *info, const uint8_t *incomingData, int len) {
#else
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
#endif
  if (len == sizeof(dataTerima)) {
    memcpy(&dataTerima, incomingData, sizeof(dataTerima));
    perintahDiterima = true;
  }
}

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("==============================================================");
  Serial.println("   SISTEM KONTROL MOTOR BTS7960 (ESP 2 - SLAVE / PENERIMA)    ");
  Serial.println("   Board: ESP32-C3                                            ");
  Serial.println("==============================================================");

  // Inisialisasi pin motor
  inisialisasiPinMotor();
  motorBerhenti();

  // Inisialisasi WiFi Mode Station
  WiFi.mode(WIFI_STA);
  Serial.print("[WiFi] MAC Address ESP 2 ini : ");
  Serial.println(WiFi.macAddress());

  // Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] ERROR: Gagal inisialisasi ESP-NOW!");
    return;
  }
  Serial.println("[ESP-NOW] Inisialisasi Berhasil.");

  // Daftarkan callback penerima data
  esp_now_register_recv_cb(OnDataRecv);

  Serial.println();
  Serial.println("[ESP 2] Siap menerima sinyal dari ESP 1...");
  Serial.println("==============================================================");
}

// ============================================================================
// LOOP UTAMA
// ============================================================================
void loop() {
  // 1. Cek apakah ada paket data baru yang masuk dari ESP 1
  if (perintahDiterima) {
    perintahDiterima = false; // Reset flag

    if (dataTerima.command == 1 && dataTerima.duration_ms > 0) {
      durasiTargetMs = dataTerima.duration_ms;
      uint8_t speed = (dataTerima.speed > 0) ? dataTerima.speed : 255;

      Serial.println();
      Serial.println("--------------------------------------------------------------");
      Serial.printf("[ESP 2] SINYAL DITERIMA DARI ESP 1!\n");
      Serial.printf("[ESP 2] Durasi: %u ms (%u detik), Kecepatan PWM: %d\n", 
                    durasiTargetMs, durasiTargetMs / 1000, speed);
      Serial.println("--------------------------------------------------------------");

      // Nyalakan motor
      motorMaju(speed);
      motorSedangAktif = true;
      waktuMulaiMotor = millis();
      sisaDetikTerakhir = durasiTargetMs / 1000;

      Serial.printf("[ESP 2] Motor BERPUTAR... Sisa waktu: %d detik\n", sisaDetikTerakhir);
    }
  }

  // 2. Monitoring durasi motor yang sedang aktif
  if (motorSedangAktif) {
    unsigned long waktuBerjalan = millis() - waktuMulaiMotor;

    if (waktuBerjalan < durasiTargetMs) {
      int sisaDetik = (durasiTargetMs - waktuBerjalan) / 1000;
      if (sisaDetik != sisaDetikTerakhir && sisaDetik >= 0) {
        sisaDetikTerakhir = sisaDetik;
        Serial.printf("[ESP 2] Motor BERPUTAR... Sisa waktu: %d detik\n", sisaDetikTerakhir);
      }
    } else {
      // Waktu durasi telah selesai, hentikan motor
      motorBerhenti();
      motorSedangAktif = false;

      Serial.println("[ESP 2] Durasi SELESAI! Motor BERHENTI.");
      Serial.println("[ESP 2] Kembali standby menunggu sinyal berikutnya...");
      Serial.println("--------------------------------------------------------------");
    }
  }

  delay(20); // Menjaga stabilitas dan efisiensi CPU
}

// ============================================================================
// FUNGSI INISIALISASI PIN
// ============================================================================
void inisialisasiPinMotor() {
  pinMode(PIN_RPWM, OUTPUT);
  pinMode(PIN_LPWM, OUTPUT);
  pinMode(PIN_PWM,  OUTPUT);
}

// ============================================================================
// FUNGSI KONTROL MOTOR
// ============================================================================
void motorMaju(int speed) {
  speed = constrain(speed, 0, 255);
  digitalWrite(PIN_PWM, HIGH); // Aktifkan Enable
  analogWrite(PIN_RPWM, speed);
  analogWrite(PIN_LPWM, 0);
}

void motorMundur(int speed) {
  speed = constrain(speed, 0, 255);
  digitalWrite(PIN_PWM, HIGH); // Aktifkan Enable
  analogWrite(PIN_RPWM, 0);
  analogWrite(PIN_LPWM, speed);
}

void motorBerhenti() {
  analogWrite(PIN_RPWM, 0);
  analogWrite(PIN_LPWM, 0);
  digitalWrite(PIN_PWM, LOW); // Matikan Enable
}
