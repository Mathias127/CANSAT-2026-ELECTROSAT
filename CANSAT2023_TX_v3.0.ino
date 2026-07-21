/*
  Versión CANSAT ARGENTINA 2026 con Diagnóstico Completo y MicroSD
  Basado en la v2.7 de LU6APA y LU4BA
*/
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <Adafruit_BMP085.h>
#include <SD.h>

Adafruit_BMP085 bmp;

char Version[] = "v3.0 - 2026";

// Pines LoRa
#define CS    18      
#define RST   14      
#define IRQ   26      
#define LED   25      

// Pin de Chip Select para la MicroSD
#define SD_CS 13      // <--- GPIO 13 para la SD

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
        dataFile.println("Packet,PresionBase_hPa,Presion_hPa,Altura_m,Temp_C");
      }
      dataFile.close();
    }
  }
  
  // Inicialización del BMP180
  bool bmpIsInit = false;
  Serial.println("Iniciando BMP180...");
  digitalWrite(LED, HIGH); 
  
  do {
        delay(1000);  
        Wire.begin(23, 22);
        if (bmp.begin(BMP085_STANDARD))
        {
              Serial.println("BMP180 inicio OK");
              readTempPressure();
              baseline = P;
              Serial.print("Presion Base: ");
              Serial.print(baseline * 0.01);
              Serial.println(" hPa");
              bmpIsInit = true;
              digitalWrite(LED, LOW); 
        } 
        else
        {
          Serial.println("Buscando sensor BMP180...");
        }     
      } while (!bmpIsInit);
}

void loop()
{
  readTempPressure();
  pktNumber++;  

  // Armar trama idéntica a la versión original de la competencia
  String dataString = String(pktNumber) + "," + 
                      String(baseline * 0.01, 2) + "," + 
                      String(P * 0.01, 2) + "," + 
                      String(A, 2) + "," + 
                      String(T, 1);

  // =========================================================================
  // IMPRESIÓN DETALLADA PARA CHEQUEO PRE-VUELO EN MONITOREO SERIAL
  // =========================================================================
  Serial.print("TX - Version: "); Serial.println(Version);
  Serial.print("Paquete N°: "); Serial.println(pktNumber);
  Serial.print("Temperatura: "); Serial.print(T, 1); Serial.println(" °C");
  Serial.print("Presion Absoluta: "); Serial.print(P * 0.01, 2); Serial.println(" hPa");
  Serial.print("Altura Relativa: "); Serial.print(A, 2); Serial.println(" m");
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
