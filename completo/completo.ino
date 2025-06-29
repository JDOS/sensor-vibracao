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

//TICKER
#include <Ticker.h>


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

//MPU6050 
unsigned long lastSave = 0;
const unsigned long interval = 50; // Salva a cada 20hz
const unsigned long limiteLinhas = 100;//set limite de linhas
unsigned long contadorLinhas = 0;
//define para gravacao com interrupcoes
volatile float Ax,Ay,Az;
Adafruit_MPU6050 mpu;

//TIMER
Ticker timer;
// Variáveis para controle de tempo
volatile unsigned long contadorMs = 0;
// Variáveis para controle de tempo
bool gravar = false;
File dataFile;

// Função handler chamada a cada 50ms
void timerHandler() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  Ax=a.acceleration.x;
  Ay=a.acceleration.y;
  Az=a.acceleration.z;

  contadorMs += 50; // Incrementa 50ms
  gravar = true; // Sinaliza para gravar no loop principal
}


void setup() {
  Serial.begin(115200);

  //CONFIGURACOES do ACESSPOINT

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

  //CONFIGURACOES CAPTIVE PORTAL e SERVER

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

  //CONFIGURACOES DO SPIFFS

  // Inicializa SPIFFS
    if(!SPIFFS.begin(true)){
      Serial.println("SPIFFS Mount Failed");
      return;
    }
    
    // Abre o arquivo para escrita
    dataFile = SPIFFS.open("/accel_data.csv", FILE_WRITE);
    if(!dataFile){
      Serial.println("Failed to open file for writing");
      return;
    }
  // Escreve o cabeçalho do arquivo
  dataFile.println("Tempo,Ax,Ay,Az");
  Serial.println("Arquivo CSV criado");
  


    //TIMER

    // Inicia o timer para chamar o handler a cada 50ms
    timer.attach_ms(50, timerHandler);
    Serial.println("Timer iniciado com sucesso");

  //CONFIGURACOES DO MPU6050 

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  //SET DE OPERACAO A 16G
  mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }

  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  //set de amostragem HZ
  mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("MPU 6050 configurado!");
  delay(100);

}


void loop() {

  //WEBSERVER ROTINA

  // Processa as requisições DNS
  dnsServer.processNextRequest();
  
  // Processa as requisições do cliente
  server.handleClient();

  //SENSOR ROTINA
//  sensors_event_t a, g, temp;
//  mpu.getEvent(&a, &g, &temp);

//  Ax=a.acceleration.x;
//  Ay=a.acceleration.y;
//  Az=a.acceleration.z;

  //GRAVACAO ROTINA utilizando SPIFSS

 // unsigned long now = millis();
  //verifica limite de linhas
 // if(contadorLinhas<limiteLinhas){
//    salvaSPIFFS(now);
//  }

  if (gravar) {
      gravar = false;
      
      // Calcula horas, minutos, segundos e milissegundos
      unsigned long totalMs = contadorMs;
      unsigned int ms = totalMs % 1000;
      unsigned int segundos = (totalMs / 1000) % 60;
      unsigned int minutos = (totalMs / 60000) % 60;
      unsigned int horas = (totalMs / 3600000) % 24;
      
      // Formata a string de tempo
      char tempoFormatado[16];
      sprintf(tempoFormatado, "%02u:%02u:%02u:%03u", horas, minutos, segundos, ms);
      
      // Imprime o tempo formatado
      Serial.println(tempoFormatado);
      salvaSPIFFS(tempoFormatado);
    }


}

//limite interno 4mb verifica se passou os 50 mili (1s/0,05s=20hz) desde a ultima gravacao
// void salvaSPIFFS_old(now){
//     //verifica os 20hz
//     if (now - lastSave >= interval) {
//       lastSave = now;
//       contadorLinhas+=1;

//       // Abre o arquivo para adicionar dados
//       File file = SPIFFS.open("/dados.csv", FILE_APPEND);
//       if (file) {
//         file.printf("%.8f,%.8f,%.8f\n", Ax,Ay,Az); // tempo em segundos, temperatura
//         file.close();
//         Serial.printf("Salvo: %.8f,%.8f,%.8f\n", Ax,Ay,Az);
//       } else {
//         Serial.println("Erro ao abrir arquivo para escrita");
//       }
//     }
// }

void salvaSPIFFS(String tempoFormatado){
        dataFile.printf("%s,%.8f,%.8f,%.8f\n", tempoFormatado, Ax, Ay, Az);  // tempo em segundos, temperatura
        Serial.printf("%s,%.8f,%.8f,%.8f\n", tempoFormatado, Ax, Ay, Az);
        dataFile.close();
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

