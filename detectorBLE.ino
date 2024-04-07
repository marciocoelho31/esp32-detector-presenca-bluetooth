#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// --- WIFI ---
#include <WiFi.h>
const char* ssid     = "..."; // Change this to your WiFi SSID
const char* password = "###"; // Change this to your WiFi password
WiFiClient esp32Client;

// --- MQTT ---
#include <PubSubClient.h>
PubSubClient client(esp32Client);
const char* mqtt_broker = "mqtt.eclipseprojects.io";
const char* mqtt_clientId = "alura-exercicio-iot";
const char* mqtt_topico_controle = "labmcoelho/iluminacao";
const char* mqtt_topico_monitoracao = "labmcoelho/monitoracao";
bool statusMonitor = false;

// --- BLUETOOTH ---
int scanTime = 5; //In seconds
int nivelRSSI = -70;  // nivel de proximidade do dispositivo com a ESP32
String dispositivoAutorizado = "d0:f5:ae:36:70:90";   // Amazfit Smart Watch
bool dispositivoPresente = false;

// --- SETUP---
void setup() {
  Serial.begin(115200);

  conectaWifi();

  client.setServer(mqtt_broker, 1883);
  client.setCallback(monitoraTopico);

  Serial.println("Scanning...");
  BLEDevice::init("");
}

void loop() {
  if (!client.connected()) {
    conectaMQTT();
  }

  client.loop();  // chama a função de callback
  desabilitaScan();

  delay(2000);
}

// --- FUNCOES AUXILIARES ---
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      String dispositivoEncontrado = advertisedDevice.getAddress().toString().c_str();
      if (dispositivoEncontrado == dispositivoAutorizado) {
        if (advertisedDevice.getRSSI() > nivelRSSI){
            Serial.println("Identificador detectado!");
            Serial.print("RSSI: ");
            Serial.println(advertisedDevice.getRSSI());
          dispositivoPresente = true;
        } else {
          dispositivoPresente = false;
        }
      }
      //Serial.printf("Advertised Device: %s \n", advertisedDevice.getAddress().toString().c_str());
    }
};

// --- SCAN BLUETOOTH ---
void scanBLE() {
  BLEScan* pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
}

// --- CONECTA AO WIFI ---
void conectaWifi() {
  
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
  }  

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}

// --- CONECTA AO MQTT ---
void conectaMQTT() {
  while (!client.connected()) {
    client.connect(mqtt_clientId);
    client.subscribe(mqtt_topico_monitoracao);
  }
}

// --- MONITORA TOPICO labmcoelho/monitoracao ---
void monitoraTopico(char* mqtt_topico_monitoracao, byte* payload, unsigned int length) {
  if ((char)payload[0] == '1') {
    statusMonitor = 1;
  } else {
    statusMonitor = 0;
  }
}

// --- PUBLICA NO TOPICO iluminacao (liga/desliga o dispositivo) ---
void publicaStatusNoTopico() {
  if (dispositivoPresente == 1) {
    client.publish(mqtt_topico_controle, String("on").c_str(), true);
    Serial.println("Power ON");
  } else {
    //if (millis() - ultimoTempoMedido > intervaloPublicacao) {
      client.publish(mqtt_topico_controle, String("off").c_str(), true);
      Serial.println("Power OFF");
    //}
  }
}

// --- DESABILITA O SCAN ---
void desabilitaScan() {
  if (statusMonitor == 0){
    Serial.println("Scan ATIVO");
    scanBLE();
  } else {
    Serial.println("Scan DESLIGADO!!");
    dispositivoPresente = true;   // para ligar remotamente o dispositivo
  }
  publicaStatusNoTopico();
}
