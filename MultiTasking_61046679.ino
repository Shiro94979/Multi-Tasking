#include <WiFi.h>
#include <PubSubClient.h>

TaskHandle_t A0;
TaskHandle_t A1;

const char* ssid = "Pichai_5GHz";
const char* password = "0881674069";
const char* mqtt_server = "";// เชื่อมต่อmqttserverไม่ได้ครับ

#define mqtt_port 1883

WiFiClient espClient;
PubSubClient *client = NULL;
long lastMsg = 0;
char msg[50];
int value = 0;

long lastReconnectAttempt = 0;
String STATE = "None";

void setup_wifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
}

void cback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

boolean reconnect() {
  if (client->connect("arduinoClient")) {
    Serial.println("MQTT connected");
    client->publish("device/outTopic", "hello world");
    client->subscribe("device/inTopic");
  }
  return client->connected();
}

void setup() {
  pinMode(LED_IN,OUTPUT);
  Serial.begin(115200);
  
  xTaskCreatePinnedToCore(
    tLedAndSerialFunc,  
    "LedAndSerial",10000,NULL,1,&A1,0);                 
  delay(500);
  
  xTaskCreatePinnedToCore(
    tNetworkFunc,       
    "Network",10000,NULL,1,&A0,1);                 
  delay(500);
}

void loop() {}
void tNetworkFunc(void *params) {

  Serial.print("tNetworkFunc running on core ");
  Serial.println(xPortGetCoreID());
  setup_wifi();
  
  while (true) {
    if (WiFi.status() != WL_CONNECTED) { 
      STATE = "None";
    } else if (WiFi.status() == WL_CONNECTED && STATE == "None") {\
     
      Serial.println("WiFi connected");
      STATE = "WiFi-Connect";
      client = new PubSubClient(espClient);
      client->setServer(mqtt_server, mqtt_port);
      client->setCallback(callback);
    } else if (WiFi.status() != WL_CONNECTED && STATE == "MQTT-Connect") {
     
      delete client;
      STATE = "None";
    } else { 
      if (!client->connected()) {
        long now = millis();
        if (now - lastReconnectAttempt > 5000) {
          lastReconnectAttempt = now;
         
          if (reconnect()) {
            lastReconnectAttempt = 0;
          }
        }
        STATE = "WiFi-Connect";
      } else {
       
        STATE = "MQTT-Connect";
        client->loop();

        long now = millis();
        if (now - lastMsg > 2000) {
          lastMsg = now;
          ++value;
          snprintf (msg, 50, "ok!! #%ld", value);
          Serial.print("Publish message: ");
          Serial.println(msg);
          client->publish("device/outTopic", msg);
        }
      }
    }
    delay(10);
  }
}

void tLedAndSerialFunc(void *params) {

  Serial.print("tLedAndSerialFunc running on core ");
  Serial.println(xPortGetCoreID());
  unsigned int Counter = 0;
  Serial.println((String)"Counter is reset(" + Counter + ")");
  uint32_t _nextSerial = millis() + 1000;
  uint32_t _nextLED = millis() + 500;

  while (true) {
    uint32_t cur = millis();
    if (cur >= _nextSerial) {
      Serial.println((String)"Counter is changed to " + Counter++);
      _nextSerial = cur + 1000;}

    if (cur >= _nextLED) {
      if (STATE == "WiFi-Connect") {
        digitalWrite(LED_IN, !digitalRead(LED_IN));
      } else if (STATE == "MQTT-Connect") {
        digitalWrite(LED_IN, LOW);
      } else if (STATE == "None") {
        digitalWrite(LED_IN, HIGH);
      }
      _nextLED = cur + 500;
    }
    delay(10);
  }
}
