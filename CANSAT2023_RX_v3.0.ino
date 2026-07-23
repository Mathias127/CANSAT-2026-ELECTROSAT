/*********
  Modified from the examples of the Arduino LoRa library
  More resources: https://randomnerdtutorials.com

  Adaptado por Matias Graiño LU4BA, LU1CJM, LU1AET, LU6APA.
  23/05/2022 adecuaciones LU6APA.
  27/05/2022 adecuaciones LU6APA.
  28/03/2023 adecuaciones LU6APA y LU4BA.
  09/05/2026 adecuaciones menores LU4BA.
  Actualización v3.1: Recepción de datos giroscopio MPU6050
*********/

#include <SPI.h>
#include <LoRa.h>

char Version[] = "v3.1 - 2026";

//Defino los pines a ser usados por el modulo transceptor LoRa
#define CS    18      // Pin de CS del módulo LoRa
#define RST   14      // Pin de Reset del módulo LoRa
#define IRQ   26      // Pin del IRQ del módulo LoRa
#define LED   25      // Pin del LED onboard

#define SERIAL_BAUDRATE   115200    // Velocidad del Puerto Serie

// Configuraciones del módulo LoRa.
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

double bitRate;

void setup() 
{  
  // Set Led onboard con Output
  pinMode(LED, OUTPUT);
  
  //Incializo el Serial Monitor
  Serial.begin(SERIAL_BAUDRATE);
  
  while (!Serial) 
  {
    // Mientras el COM no esté disponible el LED onbooard encendido
    digitalWrite(LED, HIGH);  
  }

  // Apaga el LED si se conecta al COM
  digitalWrite(LED, LOW);  
  
  Serial.println("LoRa Receiver - Iniciando...");
  
  SPI.begin(SCK, MISO, MOSI, SS);

  // Inicializar módulo LoRa
  LoRa.setPins(CS, RST, IRQ);
  while (!LoRa.begin(LORA_FREQUENCY)) 
  {
    Serial.println(".");
    delay(500);
  }

  LoRa.setSyncWord(LORA_SYNC_WORD);
  LoRa.setTxPower(LORA_POWER);              
  LoRa.setSpreadingFactor(LORA_SPREAD_FACTOR);           
  LoRa.setSignalBandwidth(LORA_SIG_BANDWIDTH);
  LoRa.setCodingRate4(LORA_CODING_RATE);  
  
  // Calculo del BitRate = (SF * (BW / 2 ^ SF)) * (4.0 / CR)
  bitRate = (LORA_SPREAD_FACTOR * (LORA_SIG_BANDWIDTH / pow(2, LORA_SPREAD_FACTOR))) * (4.0 / LORA_CODING_RATE);
  
  Serial.println("LoRa Initializing OK!");  
}


void loop() 
{
  // Trato de parsear el paquete  
  int packetSize = LoRa.parsePacket();
  
  if (packetSize) 
  {     
    // Encender LED onboard
    digitalWrite(LED, HIGH);  
  
    // Paquete recibido
    // Lectura del paquete
    while (LoRa.available()) 
    {
      String LoRaData = LoRa.readString();
      
      Serial.print("RX - Version ");     
      Serial.println(Version);      
  
      Serial.print("LoRa BitRate: ");
      Serial.print(bitRate);
      Serial.println(" bps");
            
      Serial.print("Telemetria RAW Recibida: ");
      Serial.println(LoRaData); 
        
      int indicador1 = LoRaData.indexOf(',');
      String pktNumber = LoRaData.substring(0, indicador1);
      Serial.print("Packet Number: ");
      Serial.println(pktNumber);
      
      int indicador2 = LoRaData.indexOf(',', indicador1+1);
      String presionBase = LoRaData.substring(indicador1+1, indicador2);
      Serial.print("Presion Base: ");
      Serial.println(presionBase);

      int indicador3 = LoRaData.indexOf(',', indicador2+1);
      String presionAbsoluta = LoRaData.substring(indicador2+1, indicador3);
      Serial.print("Presion Absoluta: ");
      Serial.println(presionAbsoluta);

      int indicador4 = LoRaData.indexOf(',', indicador3+1);
      String altura = LoRaData.substring(indicador3+1, indicador4);
      Serial.print("Altura: ");
      Serial.println(altura);  
        
      int indicador5 = LoRaData.indexOf(',', indicador4+1);
      String temperatura = LoRaData.substring(indicador4+1, indicador5);
      Serial.print("Temperatura: ");
      Serial.println(temperatura);    

      // ----- NUEVA SECCIÓN: Parseo del Giroscopio -----
      int indicador6 = LoRaData.indexOf(',', indicador5+1);
      String gyroX = LoRaData.substring(indicador5+1, indicador6);
      Serial.print("Gyro X (rad/s): ");
      Serial.println(gyroX);

      int indicador7 = LoRaData.indexOf(',', indicador6+1);
      String gyroY = LoRaData.substring(indicador6+1, indicador7);
      Serial.print("Gyro Y (rad/s): ");
      Serial.println(gyroY);

      int indicador8 = LoRaData.indexOf(',', indicador7+1);
      String gyroZ = LoRaData.substring(indicador7+1, indicador8);
      Serial.print("Gyro Z (rad/s): ");
      Serial.println(gyroZ);
      // ------------------------------------------------
    }
    
    // Nivel de señal RSSI del paquete
    Serial.print("Nivel de señal [RSSI]: ");
    Serial.println(LoRa.packetRssi());
    Serial.println("=====================================================================");

    // Apagar LED onboard
    digitalWrite(LED, LOW); 
  }
}
