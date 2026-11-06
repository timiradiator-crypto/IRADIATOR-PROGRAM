# IRADIATOR PROGRAM

Program sistem kontrol untuk tim Iradiator.

## Struktur Direktori
- `PROGRAM UJI/`
  - `Code_Sumber/`
    - `sumber.ino`: Program Arduino Mega 2560 untuk kontrol pergerakan motor stepper menggunakan library `AccelStepper` (Driver mode, Pin Step: 3, Pin Dir: 4) berbasis perintah Serial Monitor.

## Panduan Perintah Serial (Baud Rate: 9600)
- `n` atau `N` : Bergerak Naik (`targetSpeed = 1000.0`)
- `t` atau `T` : Bergerak Turun (`targetSpeed = -1000.0`)
- `s` atau `S` : Stop / Berhenti (`targetSpeed = 0`)
