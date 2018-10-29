#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <MFRC522.h>

#define ledGPIO4 16
#define RST_PIN 5
#define SS_PIN 4

const char* ssid = "chalkchalk";
const char* password = "51075107";
const char* mqtt_server = "192.168.0.82";

WiFiClient espClient;
PubSubClient client(espClient);
MFRC522 mfrc(SS_PIN, RST_PIN);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if(topic=="esp8266/4"){
      Serial.print("Changing GPIO 4 to ");
      if(messageTemp == "1"){
        digitalWrite(ledGPIO4, HIGH);
        Serial.print("On");
      }
      else if(messageTemp == "0"){
        digitalWrite(ledGPIO4, LOW);
        Serial.print("Off");
      }
  }
  Serial.println();
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      client.subscribe("esp8266/4");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void sendRFID() {
  String payload = "{";
  payload += "\"uuid\":";
  payload += mfrc.uid.uidByte[0];
  payload += ",";
  payload += mfrc.uid.uidByte[1];
  payload += ",";
  payload += mfrc.uid.uidByte[2];
  payload += ",";
  payload += mfrc.uid.uidByte[3];
  payload += "}";

  char attributes[100];
  payload.toCharArray( attributes, 100 );
  
  HTTPClient http;
  http.begin("http://192.168.0.82:8000/rfid");
  delay(100);
  int httpCode = http.GET();
  Serial.println(httpCode);
  http.end(); 
  delay(250);
}

void setup() {
  pinMode(ledGPIO4, OUTPUT);
  
  Serial.begin(115200);
  delay(250);
  SPI.begin(); 
  mfrc.PCD_Init();

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }

  if(!client.loop()) {
    client.connect("ESP8266Client");
  }

  if ( ! mfrc.PICC_IsNewCardPresent()) {
    delay(50);
    return;
  } else if ( ! mfrc.PICC_ReadCardSerial()) {
    delay(50);
    return;
  } else {
    sendRFID();

    for (byte i = 0; i < 4; i++) {               
      Serial.print(mfrc.uid.uidByte[i]);      
      Serial.print(" ");                     
    }
    Serial.println(); 
  }
}
