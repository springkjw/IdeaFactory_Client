#include <arduino.h>
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <PubSubClient.h>
#include <Adafruit_PN532.h>

#define PN532_SCK  (14)
#define PN532_MISO (12)
#define PN532_MOSI (13)
#define PN532_SS   (2)
//#define PN532_SCK  (13)
//#define PN532_MISO (12)
//#define PN532_MOSI (11)
//#define PN532_SS   (10)

#define RELAY_PIN (5)

#define US_TO_TICK(us) (us*5)
#define MS_TO_TICK(ms) (ms*5000)

//const char* ssid = "IdeaFactory_Machining";
//const char* password = "asdfghjkl"; 
//String mqtt_ip = "192.168.0.45";
const char* ssid = "SK_WiFiGIGA2354";
const char* password = "1801002168";
String mqtt_ip = "192.168.35.194";
String web_server = "http://" + mqtt_ip + ":8000/device/";
boolean tmpSuccess[2] = {false, false};
String res;

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_PN532 nfc(PN532_SCK, PN532_MISO, PN532_MOSI, PN532_SS);
const String deviceId = WiFi.macAddress();

void sendData(String message) {
  // 웹 서버에 데이터 보내기
  HTTPClient http;
  http.begin(String(web_server));
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    
  int httpCode = http.POST("value=" + message + "&dId=" + deviceId);
  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
    }
  }
  http.end();
}

void readCard() {
  uint8_t success;
  success = nfc.inListPassiveTarget(PN532_ISO14443B);

  client.publish("devices", "start read");

  boolean tmp1 = tmpSuccess[0];
  boolean tmp2 = tmpSuccess[1];

  if (tmp1 == false && tmp2 == false) {
    if (success == true) {
      // 웹 서버에 보내기

      uint8_t apdu[] = {
        0x00, 0xB2, 0x01, 0x0C, 0xC8
      };
      uint8_t apduLength = sizeof(apdu);
      uint8_t response[200];
      memset(response, 0, sizeof(response));
      uint8_t responseLength = sizeof(response);
  
      res = nfc.inDataExchange(apdu, apduLength, response, &responseLength);
      if (responseLength > 10) {
        res = (char*)response;

        sendData(res);
      }
    }
  }
  
  tmp1 = tmp2;
  tmp2 = success;
  tmpSuccess[0] = tmp1;
  tmpSuccess[1] = tmp2;

  if (tmpSuccess[0] == false && tmpSuccess[1] == false) {
    // 릴레이 오프
    digitalWrite(RELAY_PIN, LOW);
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  nfc.begin();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  client.setServer(mqtt_ip.c_str(), 1883);
  client.setCallback(callback);

  // PN532 연결
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    while (1);
  }
  nfc.SAMConfig();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  uint8_t success;
  success = nfc.inListPassiveTarget(PN532_ISO14443B);

  boolean tmp1 = tmpSuccess[0];
  boolean tmp2 = tmpSuccess[1];

  if (tmp1 == false && tmp2 == false && success == true) {
    uint8_t apdu[] = {
      0x00, 0xB2, 0x01, 0x0C, 0xC8
    };
    uint8_t apduLength = sizeof(apdu);
    uint8_t response[200];
    memset(response, 0, sizeof(response));
    uint8_t responseLength = sizeof(response);

    res = nfc.inDataExchange(apdu, apduLength, response, &responseLength);
    if (responseLength > 10) {
      res = (char*)response;

      String tmp_res(res);
      String msg = deviceId + '/' + tmp_res;

      client.publish("device/rfid", msg.c_str());
    }
  }
  
  tmp1 = tmp2;
  tmp2 = success;
  tmpSuccess[0] = tmp1;
  tmpSuccess[1] = tmp2;

  if (tmpSuccess[0] == false && tmpSuccess[1] == false) {
    // 릴레이 오프
    digitalWrite(RELAY_PIN, LOW);
  }

  delay(50);
}

void reconnect() {
  const char* d = deviceId.c_str();
  
  while (!client.connected()) {
    if (client.connect("ESP8266Client")) {      
      client.subscribe("device");
    } else {
      delay(3000);
    }
  }
}

void callback(String topic, byte* message, unsigned int length) {
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }

  String onMsg = deviceId + "1";
  String offMsg = deviceId + "0";
 
  if (messageTemp == onMsg){
    digitalWrite(RELAY_PIN, HIGH);
  } else if (messageTemp == offMsg){
    digitalWrite(RELAY_PIN, LOW);
  }
}
