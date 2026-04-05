#pragma once

#include <Preferences.h>
// valores por defecto WIFI
#define defaultAP_SSID "D708"
#define defaultAP_PASS "z7sbsPZbhZ4tAb"
#define apip "192, 168, 0, 1"
#define apgateway "192, 168, 0, 1"
#define apnetmask "255, 255, 255, 0"

// servidores para ajustar el reloj
#define ntpServer "pool.ntp.org"
#define ntpServer2 "time.nist.gov"

// tiempo para el reseteo
//#define resetPressTime = 0;

// --- PINOUT ESP32-C3 ---
#define BUTTON_PIN 9      // Botón físico para interacción
#define SPEAKER_PIN 8     // PWM para parlante
#define RESET_BUTTON_PIN 4   // Botón para reset total (deja default las prefs)

// Pines del motor
#define IN1 3
#define IN2 2
#define IN3 1
#define IN4 0

const char* ssid = "D708";
const char* password = "z7sbsPZbhZ4tAb";

Preferences prefs;

// variable para calcular el tiempo para el reseteo
unsigned long resetPressTime = 0;

// Estructura del evento programado
struct Evento {
  int hora;
  int minuto;
  int pasos;
};
std::vector<Evento> eventos;

// flags
bool ESaver;
bool CatButton = true;
bool Sound = true;

// IP del AP
IPAddress apIP(apip);
IPAddress apGateway(apgateway);
IPAddress apNetmask(apnetmask);

File uploadFile;
