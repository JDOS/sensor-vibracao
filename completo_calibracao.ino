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

//BUFFER Memória
#define BUFFER_SIZE 50

//LIGAR LEDS
#define LED_PIN_ON 32  // Use o pino GPIO 2
#define LED_PIN_WRITE 33  // Use o pino GPIO 2
int ledState = LOW;

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

//BUFFER MEMORIA
String buffer[BUFFER_SIZE];
int bufferIndex = 0;

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
  server.on("/download", HTTP_GET, handleDownloadCSV);
  server.on("/format", handleFormat);
  server.on("/restart", HTTP_GET, handleRestart);
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
    if(!SPIFFS.begin(false)){
      Serial.println("SPIFFS Mount Failed");
      return;
    }
    

  // Obtendo informações do SPIFFS
  // size_t total = SPIFFS.totalBytes();
  // size_t used = SPIFFS.usedBytes();

  // Serial.printf("Total: %u bytes\n", total);
  // Serial.printf("Usado: %u bytes\n", used);
  // Serial.printf("Livre: %u bytes\n", total - used);
  
  listFiles();
  //   // Abre o arquivo para escrita
  // dataFile = SPIFFS.open("/dados.csv", FILE_WRITE);
  //   if(!dataFile){
  //     Serial.println("Failed to open file for writing");
  //   }
  // // Escreve o cabeçalho do arquivo
  // dataFile.println("Tempo,Ax,Ay,Az");
  // Serial.println("Arquivo CSV criado");

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

  //LED ON
  pinMode(LED_PIN_ON, OUTPUT); // Define o pino como saída
  digitalWrite(LED_PIN_ON, HIGH); // Liga o LED
  pinMode(LED_PIN_WRITE, OUTPUT); // Define o pino como saída
  digitalWrite(LED_PIN_WRITE, LOW); // Liga o LED

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
      
      salvaBuffer(tempoFormatado);
    }


    // size_t total = SPIFFS.totalBytes();
    // size_t used = SPIFFS.usedBytes();
    // size_t disponivel = total - used;
    // Serial.printf("Armazenamento Livre: %s ",disponivel);

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

// void salvaSPIFFS(String tempoFormatado){
//         dataFile.printf("%s,%.8f,%.8f,%.8f\n", tempoFormatado, Ax, Ay, Az);  // tempo em segundos, temperatura
//         Serial.printf("%s,%.8f,%.8f,%.8f\n", tempoFormatado, Ax, Ay, Az);
//        // dataFile.close();
// }

void salvaBuffer(String tempoFormatado) {
    // Monta a linha de dados
    String linha = tempoFormatado + "," + String(Ax, 8) + "," + String(Ay, 8) + "," + String(Az, 8);
    buffer[bufferIndex++] = linha;
    // Se atingiu o tamanho do buffer, grava no SPIFFS
    if (bufferIndex >= BUFFER_SIZE) {
        File dataFile = SPIFFS.open("/dados.csv", FILE_APPEND);
        if (!dataFile) {
            Serial.println("Erro ao abrir arquivo para escrita!");
            return;
        }
        for (int i = 0; i < BUFFER_SIZE; i++) {
            dataFile.println(buffer[i]);
        }
        dataFile.close();
        bufferIndex = 0; // Limpa o buffer
        
        if (ledState == LOW) {
          ledState = HIGH;
        } else {
          ledState = LOW;
        }
      digitalWrite(LED_PIN_WRITE,ledState);
    }
    // Também pode imprimir no Serial, se quiser
 
    Serial.println(linha);
}

void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<meta http-equiv='refresh' content='0;URL=http://192.168.4.1/portal'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 0; padding: 20px; text-align: center; }";
  html += "h1 { color: #0066cc; }";
  html += ".container { max-width: 500px; margin: 0 auto; }";
  html += "button { background-color: #0066cc; color: white; border: none; padding: 10px 20px; ";
  html += "border-radius: 4px; cursor: pointer; font-size: 16px; margin-top: 20px; }";
  html += "</style>";
  html += "<title>ESP32 Captive Portal</title></head>";
  html += "<body>";
  html += "<div class='container'>";
  html += "<h1>ESP32 Captive Portal</h1>";
  html += "<p>Welcome to the ESP32 captive portal. If you're seeing this page, the portal is working correctly.</p>";
  html += "<p>If you weren't automatically redirected, please click the button below:</p>";
  html += "<button onclick=\"window.location.href='http://192.168.4.1/portal'\">Open Portal</button>";
  html += "</div>";
  html += "<script>";
  html += "window.location.href = 'http://192.168.4.1/portal';";
  html += "</script>";
  html += "</body></html>";
  
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
  html += "<a href='/controle'><button class='button'>Ir para Controles</button></a>";
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
  html += ".buttonAlert { background-color: #f44336; border: none; color: white; padding: 15px 32px;";
  html += "text-align: center; font-size: 16px; margin: 4px 2px; cursor: pointer; border-radius: 8px; }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<h1>Controles</h1>";
  html += "<a href='/'><button class='button'>Calibrar Acelerometro</button></a>";
  html += "<a href='/download'><button class='button'>Download Oscilações CSV</button></a>";
  html += "<a href='/format'><button class='buttonAlert' onclick='confirmFunction();'>Formatar disco interno</button></a>";
  html += "<script>function confirmFunction() { if (confirm('Deseja realmente apagar a memória interna?')) {window.location.href = '/format';}}</script>";
  html += "</body>";
  html += "</html>";
  
  server.send(200, "text/html", html);
}

void handleDownloadCSV() {
  const char* path = "/dados.csv";
  if (!SPIFFS.exists(path)) {
    server.send(404, "text/plain", "Arquivo não encontrado");
    return;
  }

  File file = SPIFFS.open(path, "r");
  if (!file) {
    server.send(500, "text/plain", "Erro ao abrir o arquivo");
    return;
  }

  // Define o cabeçalho para download
  server.sendHeader("Content-Type", "text/csv");
  server.sendHeader("Content-Disposition", "attachment; filename=\"dados.csv\"");
  server.streamFile(file, "text/csv");
  file.close();
}

void handleFormat(){
// To delete all files, format SPIFFS
  handlePortal();
  SPIFFS.format();
  Serial.println("SPIFFS formatted - all files deleted");
  handleRestart();
}

void handleRestart() {
  server.send(200, "text/plain", "Reiniciando ESP32...");
  delay(1000); // Pequeno delay para garantir que a resposta seja enviada
  ESP.restart();
}

void listFiles() {
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  
  if(!file){
    Serial.println("No files found");
  }
  
  while(file) {
    Serial.print("File: ");
    Serial.print(file.name());
    Serial.print(" - Size: ");
    Serial.println(file.size());
    file = root.openNextFile();
  }
}


