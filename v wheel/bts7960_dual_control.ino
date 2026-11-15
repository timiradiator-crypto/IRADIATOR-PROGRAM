/*
 * ============================================================================
 * PROYEK: KONTROL 2x MOTOR BTS7960 OTOMATIS (TANPA SERIAL MONITOR)
 * ============================================================================
 * 
 * PEMILIHAN PIN PWM YANG AMAN:
 * - Driver 1 (Motor 1): RPWM = Pin 5,  LPWM = Pin 6   (PWM aman)
 * - Driver 2 (Motor 2): RPWM = Pin 9,  LPWM = Pin 10  (PWM aman)
 * - R_EN & L_EN kedua driver: Sambungkan ke 5V (Enable selalu aktif)
 * ============================================================================
 */

// Konfigurasi Pin Motor (Gunakan Pin PWM)
const int PIN_RPWM_1 = 0;  // Driver 1 Maju
const int PIN_LPWM_1 = 1;  // Driver 1 Mundur
const int PIN_RPWM_2 = 3;  // Driver 2 Maju
const int PIN_LPWM_2 = 4; // Driver 2 Mundur
int kecepatanMotor = 250;

void setup() {
  Serial.begin(115200);

  // Inisialisasi Pin Motor sebagai OUTPUT
  pinMode(PIN_RPWM_1, OUTPUT);
  pinMode(PIN_LPWM_1, OUTPUT);
  pinMode(PIN_RPWM_2, OUTPUT);
  pinMode(PIN_LPWM_2, OUTPUT);
  pinMode(LED_BUILTIN, OUTPUT);

  berhenti();
  delay(1000);

  Serial.println(F("=================================================="));
  Serial.println(F("   SISTEM KONTROL 2x BTS7960 - OTOMATIS AKTIF     "));
  Serial.println(F("   Kedua motor langsung berputar tanpa Serial     "));
  Serial.println(F("=================================================="));

  // Langsung jalankan kedua motor maju
  maju(kecepatanMotor);
}

void loop() {
  /*
  // ==========================================================================
  // OPSI 1: Motor berjalan MAJU terus-menerus (Aktif secara default)
  // ==========================================================================
  maju(kecepatanMotor);
 */
 
  // ==========================================================================
  // OPSI 2: Gerak Otomatis Berkala (Maju -> Berhenti -> Mundur -> Berhenti)
  // (Aktifkan baris di bawah ini jika ingin siklus bolak-balik)
  // ==========================================================================
  
  // 1. Maju selama 3 detik
  maju(kecepatanMotor);
  delay(3000);

  // 2. Berhenti selama 1 detik
  berhenti();
  delay(1000);

  // 3. Mundur selama 3 detik
  mundur(kecepatanMotor);
  delay(3000);

  // 4. Berhenti selama 1 detik
  berhenti();
  delay(1000);
 
}

// ============================================================================
// FUNGSI PENGGERAK MOTOR
// ============================================================================

/**
 * Menjalankan kedua motor ke arah MAJU
 * @param pwm: Nilai PWM (0 - 255)
 */
void maju(int pwm) {
  pwm = constrain(pwm, 0, 255);
  digitalWrite(LED_BUILTIN, HIGH);
  analogWrite(PIN_RPWM_1, pwm);
  analogWrite(PIN_LPWM_1, 0);
  analogWrite(PIN_RPWM_2, pwm);
  analogWrite(PIN_LPWM_2, 0);
}

/**
 * Menjalankan kedua motor ke arah MUNDUR
 * @param pwm: Nilai PWM (0 - 255)
 */
void mundur(int pwm) {
  pwm = constrain(pwm, 0, 255);
  digitalWrite(LED_BUILTIN, HIGH);
  analogWrite(PIN_RPWM_1, 0);
  analogWrite(PIN_LPWM_1, pwm);
  analogWrite(PIN_RPWM_2, 0);
  analogWrite(PIN_LPWM_2, pwm);
}

/**
 * Menghentikan kedua motor
 */
void berhenti() {
  digitalWrite(LED_BUILTIN, LOW);
  analogWrite(PIN_RPWM_1, 0);
  analogWrite(PIN_LPWM_1, 0);
  analogWrite(PIN_RPWM_2, 0);
  analogWrite(PIN_LPWM_2, 0);
}

/**
 * Belok Kiri (Motor 1 mundur/berhenti, Motor 2 maju)
 */
void belokKiri(int pwm) {
  pwm = constrain(pwm, 0, 255);
  digitalWrite(LED_BUILTIN, HIGH);
  analogWrite(PIN_RPWM_1, 0);
  analogWrite(PIN_LPWM_1, pwm);
  analogWrite(PIN_RPWM_2, pwm);
  analogWrite(PIN_LPWM_2, 0);
}

/**
 * Belok Kanan (Motor 1 maju, Motor 2 mundur/berhenti)
 */
void belokKanan(int pwm) {
  pwm = constrain(pwm, 0, 255);
  digitalWrite(LED_BUILTIN, HIGH);
  analogWrite(PIN_RPWM_1, pwm);
  analogWrite(PIN_LPWM_1, 0);
  analogWrite(PIN_RPWM_2, 0);
  analogWrite(PIN_LPWM_2, pwm);
}
