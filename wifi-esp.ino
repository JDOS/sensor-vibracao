//hotspot
#include <WiFi.h>
#include <WebServer.h>
//captive portal
#include <DNSServer.h>
//Sensor de aceleracao e temp MPU6050
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
//Gravacao interna ESP32
#include "FS.h"
#include "SPIFFS.h"

//CONFIGURACOES DO ACESSPOINT

// Configurações do Access Point
const char* ssid = "FujitekVibration";
const char* password = "12345678";

//CONFIGURACOES DO DNS
// Porta do servidor DNS (padrão: 53)
const byte DNS_PORT = 53;

// Cria o objeto do servidor web na porta 80
WebServer server(80);

DNSServer dnsServer;

// Endereço IP do AP
IPAddress apIP(192, 168, 4, 1);





void setup() {
  Serial.begin(115200);

  // Configura IP fixo para o AP
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

  delay(1000); // Aguarda 1 segundo para estabilizar

  WiFi.mode(WIFI_AP);

  // Configura o ESP32 como Access Point
  WiFi.softAP(ssid, password);
  
  Serial.println();
  Serial.print("Access Point iniciado: ");
  Serial.println(ssid);
  Serial.print("Endereço IP: ");
  Serial.println(WiFi.softAPIP());

  // Configura o servidor DNS para redirecionar todas as solicitações para o ESP32
  dnsServer.start(DNS_PORT, "*", apIP);
  
  // Define as rotas do servidor
  server.on("/", handleRoot);
  server.on("/portal", handlePortal);
  server.on("/controle", handleControle);
// Rotas específicas para Android
  server.on("/generate_204", handleRoot);  // Android 9+
  server.on("/gen_204", handleRoot);       // Android 4-8
  server.on("/connecttest.txt", handleRoot); // Windows
  server.on("/redirect", handleRoot);      // Alguns navegadores
  server.on("/hotspot-detect.html", handleRoot); // iOS
  server.on("/ncsi.txt", handleRoot);      // Windows

  // Rota para capturar todas as solicitações não definidas
  server.onNotFound([]() {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
  });
  
  // Inicia o servidor
  server.begin();
  Serial.println("Servidor HTTP e DNS iniciados");
}

void loop() {

  // Processa as requisições DNS
  dnsServer.processNextRequest();
  
  // Processa as requisições do cliente
  server.handleClient();
}

// Função que será chamada quando o cliente acessar a página raiz
void handleRoot() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta http-equiv='refresh' content='0;url=http://192.168.4.1/portal'>";
  html += "<title>ESP32 Web Server</title>";
  html += "</head>";
  html += "<body>";
  html += "<p>Redirecionando...</p>";
  html += "<script>window.location.href='http://192.168.4.1/portal';</script>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

// Página principal do portal
void handlePortal() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Fujitek VibraSense</title>";
  html += "<style>";
  html += "body { font-family: Arial; text-align: center; margin: 0px auto; padding: 20px; }";
  html += "h1 { color: #0F3376; margin: 50px auto 30px; }";
  html += ".button { background-color: #4CAF50; border: none; color: white; padding: 15px 32px;";
  html += "text-align: center; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 8px; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<h1>Fujitek Motion Vibration Sensor</h1>";
  html += "<a href='/controle'><button class='button'>Ir para Controle</button></a>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}


// Página principal do portal
void handleControle() {
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<meta charset='UTF-8'>";
  html += "<title>Fujitek VibraSense</title>";
  html += "<style>";
  html += "body { font-family: Arial; text-align: center; margin: 0px auto; padding: 20px; }";
  html += "h1 { color: #0F3376; margin: 50px auto 30px; }";
  html += ".button { background-color: #4CAF50; border: none; color: white; padding: 15px 32px;";
  html += "text-align: center; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 8px; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<h1>Controles</h1>";
  html += "<a href='/'><button class='button'>Calibrar Acelerometro</button></a>";
  html += "<a href='/'><button class='button'>Captar Oscilações</button></a>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}