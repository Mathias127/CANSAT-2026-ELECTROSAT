/*********
  Modified from the examples of the Arduino LoRa library
  More resources: https://randomnerdtutorials.com

  Adaptado por Matias Graiño LU4BA, LU1CJM, LU1AET, LU6APA.
  23/05/2022 adecuaciones LU6APA.
  27/05/2022 adecuaciones LU6APA.
  28/03/2023 adecuaciones LU6APA y LU4BA.
  09/05/2026 adecuaciones menores LU4BA.
*********/

#include <SPI.h>
#include <LoRa.h>

char Version[] = "v3.0 - 2026";

//Defino los pines a ser usados por el modulo transceptor LoRa
#define CS    18      // Pin de CS del módulo LoRa
#define RST   14      // Pin de Reset del módulo LoRa
#define IRQ   26      // Pin del IRQ del módulo LoRa
#define LED   25     // Pin del LED onboard

#define SERIAL_BAUDRATE   115200    // Velocidad del Puerto Serie

// Configuraciones del módulo LoRa. Tener en cuenta que esta configuración debe ser igual en el Rx!
// 433E6 for Asia
// 866E6 for Europe
// 915E6 for North America 
// 915E6 to 928E3 for Argentina
#define LORA_FREQUENCY      927000000  // Frecuencia en Hz a la que se quiere transmitir.
#define LORA_SYNC_WORD      0x7F       // Byte value to use as the sync word, defaults to 0x12
#define LORA_POWER          17         // TX power in dB, defaults to 17. Supported values are 2 to 20 for PA_OUTPUT_PA_BOOST_PIN, and 0 to 14 for PA_OUTPUT_RFO_PIN.
#define LORA_SPREAD_FACTOR  7          // Spreading factor, defaults to 7. Supported values are between 6 and 12 (En Argentina se puede utilizar entre 7 a 10)
#define LORA_SIG_BANDWIDTH  125E3      // Signal bandwidth in Hz, defaults to 125E3. Supported values are 7.8E3, 10.4E3, 15.6E3, 20.8E3, 31.25E3, 41.7E3, 62.5E3, 125E3, 250E3, and 500E3 
#define LORA_CODING_RATE    5          // Denominator of the coding rate, defaults to 5. Supported values are between 5 and 8, these correspond to coding rates of 4/5 and 4/8. The coding rate numerator is fixed at 4.


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
  
  Serial.println("LoRa Receiver");
  
  SPI.begin(SCK, MISO, MOSI, SS);

  // Inicializar módulo LoRa
  LoRa.setPins(CS, RST, IRQ);
  while (!LoRa.begin(LORA_FREQUENCY)) 
  {
    Serial.println(".");
    delay(500);
  }

  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
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
  // Version v2.7 - 09/05/2026
  // LoRa BitRate: 5468.75 bps
  // Telemetria RAW Recibida 530,1014.66,1014.56,0.81,22.15
  // Packet Number: 530
  // Presion Base: 1014.66
  // Presion Absoluta: 1014.56
  // Altura: 0.81
  // Temperatura: 22.15
  // Nivel de señal [RSSI]: -45
  // =====================================================================
  
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
            
      Serial.print("Telemetria RAW Recibida ");
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
    }
    
    // Nivel de señal RSSI del paquete
    Serial.print("Nivel de señal [RSSI]: ");
    Serial.println(LoRa.packetRssi());
    Serial.println("=====================================================================");

    // Apagar LED onboard
    digitalWrite(LED, LOW); 
  }
}
