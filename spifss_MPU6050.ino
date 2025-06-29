// Basic demo for accelerometer readings from Adafruit MPU6050

#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "FS.h"
#include "SPIFFS.h"


unsigned long lastSave = 0;
const unsigned long interval = 50; // Salva a cada 20hz
const unsigned long limiteLinhas = 100;
unsigned long contadorLinhas = 0;
float Ax,Ay,Az;

Adafruit_MPU6050 mpu;

void setup(void) {
  Serial.begin(115200);

  if (!SPIFFS.begin(true)) {
    Serial.println("Falha ao montar SPIFFS");
    return;
  }

  // Cria o arquivo e escreve o cabeçalho se não existir
  if (!SPIFFS.exists("/dados.csv")) {
    File file = SPIFFS.open("/dados.csv", FILE_WRITE);
    if (file) {
      file.println("tempo,temperatura");
      file.close();
    }
  }

  while (!Serial)
    delay(10); // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

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

  mpu.setFilterBandwidth(MPU6050_BAND_260_HZ);
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

  Serial.println("");
  delay(100);
}

void loop() {

  /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  /* Print out the values */
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x,8);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y,8);
  Serial.print(", Z: ");
  Serial.print(a.acceleration.z,8);
  Serial.println(" m/s^2");

  Serial.print("Rotation X: ");
  Serial.print(g.gyro.x);
  Serial.print(", Y: ");
  Serial.print(g.gyro.y);
  Serial.print(", Z: ");
  Serial.print(g.gyro.z);
  Serial.println(" rad/s");

  Serial.print("Temperature: ");
  Serial.print(temp.temperature);
  Serial.println(" degC");

  Serial.println("");

  Ax=a.acceleration.x;
  Ay=a.acceleration.y;
  Az=a.acceleration.z;

  unsigned long now = millis();

  if(contadorLinhas<limiteLinhas){
      if (now - lastSave >= interval) {
        lastSave = now;
        contadorLinhas+=1;


        // Abre o arquivo para adicionar dados
        File file = SPIFFS.open("/dados.csv", FILE_APPEND);
        if (file) {
          file.printf("%.8f,%.8f,%.8f\n", Ax,Ay,Az); // tempo em segundos, temperatura
          file.close();
          Serial.printf("Salvo: %.8f,%.8f,%.8f\n", Ax,Ay,Az);
        } else {
          Serial.println("Erro ao abrir arquivo para escrita");
        }
      }
    }
  //delay(1000);


}



