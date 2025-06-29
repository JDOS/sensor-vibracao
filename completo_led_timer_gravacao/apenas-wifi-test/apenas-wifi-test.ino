#include <WiFi.h>

// Defina o nome e a senha da rede WiFi que o ESP32 irá criar
const char* ssid = "FujitekVibration";
const char* password = "12345678";

void setup() {
  Serial.begin(115200);

  delay(1000); // Aguarda 1 segundo para estabilizar

  WiFi.mode(WIFI_AP);
  // Inicializa o modo Access Point
  WiFi.softAP(ssid, password);

  Serial.println();
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.println("\" iniciado");

  // Mostra o IP do Access Point
  Serial.print("IP address: ");
  Serial.println(WiFi.softAPIP());
}

void loop() {
  // O loop pode ficar vazio, a menos que queira adicionar funcionalidades
}