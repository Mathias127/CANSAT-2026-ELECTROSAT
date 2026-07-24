/*
  Versión CANSAT ARGENTINA 2026 con Diagnóstico, MicroSD y Giroscopio (MPU6050)
  Basado en la v3.1
*/
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <SD.h>

Adafruit_BMP280 bmp;
Adafruit_MPU6050 mpu;

char Version[] = "v3.2 - 2026";

// Pines LoRa
#define CS    18      
#define RST   14      
#define IRQ   26      
#define LED   25      

// Pin de Chip Select para la MicroSD
#define SD_CS 13      

#define SERIAL_BAUDRATE   115200    
#define INTERVAL_TIME_TX  1000      

#define LORA_FREQUENCY      927000000  
#define LORA_SYNC_WORD      0x7F      
#define LORA_POWER          17         
#define LORA_SPREAD_FACTOR  7          
#define LORA_SIG_BANDWIDTH  125E3      
#define LORA_CODING_RATE    5          

#define SCK   5
#define MISO  19
#define MOSI  27
#define SS    18

double baseline;
double T, P, A;
float gyroX, gyroY, gyroZ;
unsigned int pktNumber = 0;
double bitRate;
bool sdDisponible = false;

void setup()
{
  Serial.begin(SERIAL_BAUDRATE);
  
  pinMode(LED, OUTPUT);
  pinMode(RST, OUTPUT);
  digitalWrite(RST, HIGH);  
    
  LoRa.setPins(CS, RST, IRQ);
  SPI.begin(SCK, MISO, MOSI, SS);

  while (!LoRa.begin(LORA_FREQUENCY)) 
  {
      Serial.println("No arranca el LoRa");
      delay(1000);
  }
  
  LoRa.setTxPower(LORA_POWER);              
  LoRa.setSpreadingFactor(LORA_SPREAD_FACTOR);           
  LoRa.setSignalBandwidth(LORA_SIG_BANDWIDTH);
  LoRa.setCodingRate4(LORA_CODING_RATE);       
  LoRa.setSyncWord(LORA_SYNC_WORD);    

  bitRate = (LORA_SPREAD_FACTOR * (LORA_SIG_BANDWIDTH / pow(2, LORA_SPREAD_FACTOR))) * (4.0 / LORA_CODING_RATE);
  Serial.println("LoRa OK!");

  // Inicialización de la MicroSD
  Serial.print("Iniciando Tarjeta MicroSD...");
  if (!SD.begin(SD_CS, SPI)) {
    Serial.println(" [ERROR] No montada / Ausente.");
    sdDisponible = false;
  } else {
    Serial.println(" [OK]");
    sdDisponible = true;

    File dataFile = SD.open("/telemetria.csv", FILE_APPEND);
    if (dataFile) {
      if (dataFile.size() == 0) {
        // Se agregaron las columnas del giroscopio al encabezado
        dataFile.println("Packet,PresionBase_hPa,Presion_hPa,Altura_m,Temp_C,GyroX_rads,GyroY_rads,GyroZ_rads");
      }
      dataFile.close();
    }
  }
  
  // Iniciar bus I2C para ambos sensores (SDA=23, SCL=22)
  Wire.begin(23, 22);

  // Inicialización del BMP280
  bool bmpIsInit = false;
  Serial.println("Iniciando BMP280...");
  digitalWrite(LED, HIGH); 
  
  do {
        delay(1000);  
        if (bmp.begin(0x76))
        {
              Serial.println("BMP280 inicio OK");
              readTempPressure();
              baseline = P;
              Serial.print("Presion Base: ");
              Serial.print(baseline * 0.01);
              Serial.println(" hPa");
              bmpIsInit = true;
        } 
        else
        {
          Serial.println("Buscando sensor BMP280...");
        }     
      } while (!bmpIsInit);

  // Inicialización del MPU6050
  Serial.println("Iniciando MPU6050...");
  if (!mpu.begin()) {
    Serial.println("Error al iniciar MPU6050. Verifica conexiones!");
  } else {
    Serial.println("MPU6050 inicio OK");
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }
  
  digitalWrite(LED, LOW); 
}

void loop()
{
  readTempPressure();
  readGyro();
  pktNumber++;  

  // Armar trama agregando las 3 variables nuevas del giroscopio
  String dataString = String(pktNumber) + "," + 
                      String(baseline * 0.01, 2) + "," + 
                      String(P * 0.01, 2) + "," + 
                      String(A, 2) + "," + 
                      String(T, 1) + "," +
                      String(gyroX, 2) + "," +
                      String(gyroY, 2) + "," +
                      String(gyroZ, 2);

  // =========================================================================
  // IMPRESIÓN DETALLADA PARA CHEQUEO PRE-VUELO EN MONITOREO SERIAL
  // =========================================================================
  Serial.print("TX - Version: "); Serial.println(Version);
  Serial.print("Paquete N°: "); Serial.println(pktNumber);
  Serial.print("Temperatura: "); Serial.print(T, 1); Serial.println(" °C");
  Serial.print("Presion Absoluta: "); Serial.print(P * 0.01, 2); Serial.println(" hPa");
  Serial.print("Altura Relativa: "); Serial.print(A, 2); Serial.println(" m");
  Serial.print("Giroscopio (rad/s): X="); Serial.print(gyroX, 2); 
  Serial.print(" | Y="); Serial.print(gyroY, 2); 
  Serial.print(" | Z="); Serial.println(gyroZ, 2);
  Serial.print("Trama enviada [RAW]: "); Serial.println(dataString);

  // 1. Enviar por LoRa
  digitalWrite(LED, HIGH);    
  LoRa.beginPacket();
  LoRa.print(dataString);
  LoRa.endPacket();
  digitalWrite(LED, LOW);

  // 2. Guardar en MicroSD y notificar en pantalla
  if (sdDisponible) {
    guardarEnSD(dataString);
  } else {
    Serial.println("SD: No disponible (sin guardado a bordo)");
  }

  if (pktNumber >= 65500) pktNumber = 0;

  Serial.println("=====================================================================");
  delay(INTERVAL_TIME_TX);
}

void readTempPressure()
{
  T = bmp.readTemperature();
  P = bmp.readPressure();
  A = bmp.readAltitude(); 
} 

void readGyro()
{
  sensors_event_t a_event, g_event, temp_event;
  mpu.getEvent(&a_event, &g_event, &temp_event);
  
  // Guardamos las velocidades angulares en radianes por segundo
  gyroX = g_event.gyro.x;
  gyroY = g_event.gyro.y;
  gyroZ = g_event.gyro.z;
}

void guardarEnSD(String datos) {
  File dataFile = SD.open("/telemetria.csv", FILE_APPEND);
  
  if (dataFile) {
    dataFile.println(datos);
    dataFile.close();
    Serial.println("SD: Guardado exitoso.");
  } else {
    Serial.println("SD: Error al escribir archivo.");
  }
}
