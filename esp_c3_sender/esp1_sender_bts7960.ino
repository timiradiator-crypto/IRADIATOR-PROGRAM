/*
 * ============================================================================
 * PROYEK: KONTROL 2 MOTOR BTS7960 DENGAN ESP32-C3 (ESP 1 - MASTER / PENGIRIM)
 * ============================================================================
 * Fitur:
 * 1. Mengontrol 2 Motor DC via Driver BTS7960 (IBT-2).
 * 2. Input durasi putar motor diatur melalui Serial Monitor (dalam detik).
 * 3. Kedua motor berputar bersamaan (simultan) sesuai durasi yang diinput.
 * 4. Setelah waktu selesai & motor berhenti, ESP 1 otomatis mengirimkan sinyal
 *    nirkabel ESP-NOW ke ESP 2 (Penerima) untuk mengaktifkan motor di ESP 2
 *    dengan durasi yang sama persis.
 * ============================================================================
 * 
 * PINOUT ESP32-C3 KE DRIVER BTS7960:
 * ----------------------------------------------------------------------------
 * MOTOR 1 (BTS7960 #1):
 * - GPIO 4 -> RPWM (Maju)
 * - GPIO 3 -> LPWM (Mundur)
 * - GPIO 6 -> R_EN & L_EN (Pin Enable / PWM Kecepatan)
 * 
 * MOTOR 2 (BTS7960 #2):
 * - GPIO 2 -> RPWM (Maju)
 * - GPIO 1 -> LPWM (Mundur)
 * - GPIO 7 -> R_EN & L_EN (Pin Enable / PWM Kecepatan)
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
// KONFIGURASI ALAMAT MAC ESP 2 (PENERIMA)
// ============================================================================
// Ganti baris di bawah dengan MAC Address ESP 2 yang didapat dari sketch 'cek_mac_address.ino'
// Default: Broadcast address (mengirim ke semua ESP terdekat jika belum diisi spesifik)
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Contoh MAC Address spesifik:
// uint8_t broadcastAddress[] = {0x34, 0x85, 0x18, 0x7A, 0xBC, 0x12};

// ============================================================================
// DEFINISI PIN MOTOR (ESP32-C3)
// ============================================================================
// Motor 1
const int PIN_M1_RPWM = 4; // GPIO 4: PWM Maju
const int PIN_M1_LPWM = 3; // GPIO 3: PWM Mundur
const int PIN_M1_PWM  = 6; // GPIO 6: Enable (R_EN & L_EN digabung)

// Motor 2
const int PIN_M2_RPWM = 2; // GPIO 2: PWM Maju
const int PIN_M2_LPWM = 1; // GPIO 1: PWM Mundur
const int PIN_M2_PWM  = 7; // GPIO 7: Enable (R_EN & L_EN digabung)

// Kecepatan Motor (0 - 255)
const int KECEPATAN_DEFAULT = 255; // 255 = Kecepatan maksimal

// ============================================================================
// STRUKTUR PESAN ESP-NOW
// ============================================================================
typedef struct struct_message {
  uint8_t command;      // 1: Jalankan motor, 0: Berhenti
  uint32_t duration_ms; // Durasi dalam milidetik (mengikuti input ESP 1)
  uint8_t speed;        // Nilai kecepatan PWM (0-255)
} struct_message;

struct_message dataKirim;
esp_now_peer_info_t peerInfo;

// Flag status pengiriman
volatile bool pesanTerkirim = false;
volatile bool statusSukses = false;

// ============================================================================
// CALLBACK ESP-NOW SAAT DATA TERKIRIM
// ============================================================================
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  pesanTerkirim = true;
  statusSukses = (status == ESP_NOW_SEND_SUCCESS);
}

// ============================================================================
// DEKLARASI FUNGSI KONTROL MOTOR
// ============================================================================
void inisialisasiPinMotor();
void motor1Maju(int speed);
void motor1Mundur(int speed);
void motor1Berhenti();
void motor2Maju(int speed);
void motor2Mundur(int speed);
void motor2Berhenti();
void hentikanSemuaMotor();
void kirimDataKeESP2(uint32_t durasiMs, uint8_t speed);
void tampilkanMenuUtama();

// ============================================================================
// SETUP
// ============================================================================
void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println();
  Serial.println("==============================================================");
  Serial.println("   SISTEM KONTROL 2 MOTOR BTS7960 (ESP 1 - MASTER/SENDER)    ");
  Serial.println("   Board: ESP32-C3                                            ");
  Serial.println("==============================================================");

  // Inisialisasi Pin Motor
  inisialisasiPinMotor();
  hentikanSemuaMotor();

  // Inisialisasi WiFi Mode Station untuk ESP-NOW
  WiFi.mode(WIFI_STA);
  Serial.print("[WiFi] MAC Address ESP 1 ini : ");
  Serial.println(WiFi.macAddress());

  // Inisialisasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESP-NOW] ERROR: Gagal menginisialisasi ESP-NOW!");
    return;
  }
  Serial.println("[ESP-NOW] Berhasil diinisialisasi.");

  // Daftarkan callback pengiriman
  esp_now_register_send_cb(OnDataSent);

  // Daftarkan ESP 2 sebagai Peer
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ESP-NOW] Peringatan: Gagal menambahkan peer penerima!");
  } else {
    Serial.println("[ESP-NOW] Peer ESP 2 berhasil didaftarkan.");
  }

  tampilkanMenuUtama();
}

// ============================================================================
// LOOP UTAMA
// ============================================================================
void loop() {
  // Cek apakah ada input dari Serial Monitor
  if (Serial.available() > 0) {
    String input = Serial.readStringUntil('\n');
    input.trim(); // Bersihkan whitespace / newline

    if (input.length() > 0) {
      int durasiDetik = input.toInt();

      if (durasiDetik <= 0) {
        Serial.println("[ERROR] Durasi harus berupa angka bulat positif lebih dari 0!");
        Serial.print("Silakan masukkan durasi (detik): ");
      } else {
        uint32_t durasiMs = (uint32_t)durasiDetik * 1000;

        Serial.println();
        Serial.println("--------------------------------------------------------------");
        Serial.printf(">> MEMULAI PROSES: Motor 1 & Motor 2 menyala selama %d detik\n", durasiDetik);
        Serial.println("--------------------------------------------------------------");

        // 1. Nyalakan kedua motor secara bersamaan
        motor1Maju(KECEPATAN_DEFAULT);
        motor2Maju(KECEPATAN_DEFAULT);

        // 2. Tampilkan hitung mundur non-blocking
        unsigned long waktuMulai = millis();
        int sisaDetikTerakhir = durasiDetik;
        Serial.printf("[ESP 1] Motor 1 & 2 BERPUTAR... Sisa waktu: %d detik\n", sisaDetikTerakhir);

        while (millis() - waktuMulai < durasiMs) {
          int sisaDetik = durasiDetik - (int)((millis() - waktuMulai) / 1000);
          if (sisaDetik != sisaDetikTerakhir && sisaDetik >= 0) {
            sisaDetikTerakhir = sisaDetik;
            Serial.printf("[ESP 1] Motor 1 & 2 BERPUTAR... Sisa waktu: %d detik\n", sisaDetikTerakhir);
          }
          delay(50); // Mencegah watchdog timeout
        }

        // 3. Waktu selesai, hentikan kedua motor
        hentikanSemuaMotor();
        Serial.println("[ESP 1] Durasi selesai! Motor 1 dan Motor 2 BERHENTI.");

        // Beri jeda sejenak untuk stabilisasi arus motor sebelum transmisi
        delay(300);

        // 4. Kirim sinyal nirkabel ESP-NOW ke ESP 2 (Penerima)
        Serial.println("[ESP 1] Mengirim sinyal pemicu ke ESP 2 (Penerima)...");
        kirimDataKeESP2(durasiMs, KECEPATAN_DEFAULT);

        Serial.println("--------------------------------------------------------------");
        tampilkanMenuUtama();
      }
    }
  }
}

// ============================================================================
// FUNGSI INISIALISASI PIN
// ============================================================================
void inisialisasiPinMotor() {
  // Motor 1
  pinMode(PIN_M1_RPWM, OUTPUT);
  pinMode(PIN_M1_LPWM, OUTPUT);
  pinMode(PIN_M1_PWM,  OUTPUT);

  // Motor 2
  pinMode(PIN_M2_RPWM, OUTPUT);
  pinMode(PIN_M2_LPWM, OUTPUT);
  pinMode(PIN_M2_PWM,  OUTPUT);
}

// ============================================================================
// FUNGSI KONTROL MOTOR 1 (RPWM=4, LPWM=3, PWM=6)
// ============================================================================
void motor1Maju(int speed) {
  speed = constrain(speed, 0, 255);
  digitalWrite(PIN_M1_PWM, HIGH); // Aktifkan Enable
  analogWrite(PIN_M1_RPWM, speed);
  analogWrite(PIN_M1_LPWM, 0);
}

void motor1Mundur(int speed) {
  speed = constrain(speed, 0, 255);
  digitalWrite(PIN_M1_PWM, HIGH); // Aktifkan Enable
  analogWrite(PIN_M1_RPWM, 0);
  analogWrite(PIN_M1_LPWM, speed);
}

void motor1Berhenti() {
  analogWrite(PIN_M1_RPWM, 0);
  analogWrite(PIN_M1_LPWM, 0);
  digitalWrite(PIN_M1_PWM, LOW); // Matikan Enable
}

// ============================================================================
// FUNGSI KONTROL MOTOR 2 (RPWM=2, LPWM=1, PWM=7)
// ============================================================================
void motor2Maju(int speed) {
  speed = constrain(speed, 0, 255);
  digitalWrite(PIN_M2_PWM, HIGH); // Aktifkan Enable
  analogWrite(PIN_M2_RPWM, speed);
  analogWrite(PIN_M2_LPWM, 0);
}

void motor2Mundur(int speed) {
  speed = constrain(speed, 0, 255);
  digitalWrite(PIN_M2_PWM, HIGH); // Aktifkan Enable
  analogWrite(PIN_M2_RPWM, 0);
  analogWrite(PIN_M2_LPWM, speed);
}

void motor2Berhenti() {
  analogWrite(PIN_M2_RPWM, 0);
  analogWrite(PIN_M2_LPWM, 0);
  digitalWrite(PIN_M2_PWM, LOW); // Matikan Enable
}

// Menghentikan kedua motor secara serentak
void hentikanSemuaMotor() {
  motor1Berhenti();
  motor2Berhenti();
}

// ============================================================================
// FUNGSI PENGIRIMAN ESP-NOW
// ============================================================================
void kirimDataKeESP2(uint32_t durasiMs, uint8_t speed) {
  dataKirim.command = 1;              // 1 = Jalankan motor
  dataKirim.duration_ms = durasiMs;   // Durasi sama persis dengan ESP 1
  dataKirim.speed = speed;            // Kecepatan motor

  pesanTerkirim = false;
  statusSukses = false;

  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *)&dataKirim, sizeof(dataKirim));

  if (result == ESP_OK) {
    // Tunggu feedback dari callback OnDataSent (maksimal 500ms)
    unsigned long timeout = millis();
    while (!pesanTerkirim && (millis() - timeout < 500)) {
      delay(10);
    }

    if (statusSukses) {
      Serial.printf("[ESP-NOW] SUKSES: Data terkirim ke ESP 2! (Durasi: %d detik)\n", durasiMs / 1000);
    } else {
      Serial.println("[ESP-NOW] TERKIRIM (Broadcast/Pending ack). Pastikan ESP 2 dalam jangkauan.");
    }
  } else {
    Serial.printf("[ESP-NOW] GAGAL mengirim paket. Error code: %d\n", result);
  }
}

// ============================================================================
// TAMPILAN MENU SERIAL MONITOR
// ============================================================================
void tampilkanMenuUtama() {
  Serial.println();
  Serial.println("==============================================================");
  Serial.println("SILAKAN MASUKKAN DURASI PUTARAN MOTOR:");
  Serial.println("- Ketik angka durasi dalam detik (misal: 5 atau 10)");
  Serial.println("- Tekan ENTER untuk memulai");
  Serial.println("==============================================================");
  Serial.print("Masukkan durasi (detik): ");
}
