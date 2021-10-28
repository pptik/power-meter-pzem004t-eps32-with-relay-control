#include <PZEM004Tv30.h>
#include <SoftwareSerial.h>
#include <PubSubClient.h>
#include <ESP8266WiFi.h>

#if defined(ESP32)
#error "Software Serial is not supported on the ESP32"
#endif
#if !defined(PZEM_RX_PIN) && !defined(PZEM_TX_PIN)
#define PZEM_RX_PIN 14
#define PZEM_TX_PIN 12
#endif

SoftwareSerial pzemSWSerial(PZEM_RX_PIN, PZEM_TX_PIN);
PZEM004Tv30 pzem(pzemSWSerial);
WiFiClient espClient;
PubSubClient client(espClient);
byte mac[6]; //array temp mac address
String MACAddress;
const char* deviceGuid = "1726ec59-b2bd-4eb4-ae1e-23924cfea544";
const char* wifiSsid              = "LSKK_Lantai2";
const char* wifiPassword          = "lskk12345";
const char* mqttHost              = "rmq2.pptik.id";
const char* mqttUserName          = "/smartpju:smartpju";
const char* mqttPassword          = "pe3jug1L4!";
//const char* mqttClient          = "PJU";
const char* mqttQueueSubscribe    = "Aktuator";
const char* mqttQueuePublish      = "Log";
int relayControlPin               = D1;
int loop_count                    = 0 ;
String statusDevice[8]            = {"0"};

void setup() {
  Serial.begin(115200);
  setup_wifi();
  printMACAddress();
  client.setServer(mqttHost, 1883);
  client.setCallback(callback);
  pinMode(D1, OUTPUT);
  digitalWrite(D1, LOW);
  watchdogSetup();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void watchdogSetup(void) {
  ESP.wdtDisable();
}

/*
   Set up WiFi connection
*/
void setup_wifi() {
  delay(10);
  //We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to :");
  Serial.println(wifiSsid);
  WiFi.begin(wifiSsid, wifiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/*
   Function for Get Mac Address from microcontroller
*/

String mac2String(byte ar[]) {
  String s;
  for (byte i = 0; i < 6; ++i)
  {
    char buf[3];
    sprintf(buf, "%2X", ar[i]);
    s += buf;
    if (i < 5) s += ':';
  }
  return s;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
   Function for Print Mac Address
*/
void printMACAddress() {
  WiFi.macAddress(mac);
  MACAddress = mac2String(mac);
  Serial.println(MACAddress);
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



/*
   Function for Get message payload from MQTT rabbit mq
*/
char sPayload[100];
char message [40] ;
char address[40];
void callback(char* topic, byte* payload, unsigned int length) {
  memcpy(sPayload, payload, length);
  memcpy(address, payload, 36);
  memcpy(message, &payload[37], length - 37);
  if (String((char *)address) == String((char *)deviceGuid))
  {
    Serial.println("address sama");
  }
  else
  {
    Serial.println("address berbeda");
    return;
  }

  Serial.println(message);

  if (message[0] == '1') {
    digitalWrite(relayControlPin, HIGH);
    Serial.println("relay 1 on");
    statusDevice[0] = "1";

  }
  if (message[0] == '0') {
    digitalWrite(relayControlPin, LOW);
    Serial.println("relay 1 off");
    statusDevice[0] = "0";
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
   Function for Reconnecting to MQTT/RabbitMQ
*/
void reconnect() {
  // Loop until we're reconnected
  printMACAddress();
  const char* CL;
  CL = MACAddress.c_str();
  Serial.println(CL);
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect(CL, mqttUserName, mqttPassword)) {
      Serial.println("connected");
      client.subscribe(mqttQueueSubscribe);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      ESP.restart();
      delay(5000);

    }
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  // Read the data from the sensor
  float voltage = pzem.voltage();
  float current = pzem.current();
  float power = pzem.power();
  float energy = pzem.energy();
  float frequency = pzem.frequency();
  float pf = pzem.pf();
  String dataSendtoMqtt;
  String convertDeviceGuid = String(deviceGuid);
  for (int i = 0; i <= loop_count; i++) {
    if (!client.connected()) {
      reconnect();
    }


    // Check if the data is valid
    if (isnan(voltage)) {
      Serial.println("Error reading voltage");
    } else if (isnan(current)) {
      Serial.println("Error reading current");
    } else if (isnan(power)) {
      Serial.println("Error reading power");
    } else if (isnan(energy)) {
      Serial.println("Error reading energy");
    } else if (isnan(frequency)) {
      Serial.println("Error reading frequency");
    } else if (isnan(pf)) {
      Serial.println("Error reading power factor");
    }
    
  }
  dataSendtoMqtt = String(convertDeviceGuid + "#" + voltage + "#" + current + "#" + power + "#" + energy + "#" + frequency + "#" + pf);
  client.publish(mqttQueuePublish, dataSendtoMqtt.c_str());
  loop_count++;
  ESP.wdtFeed();
  Serial.print(loop_count);
  Serial.print(". Watchdog fed in approx. ");
  Serial.print(loop_count * 5000);
  Serial.println(" milliseconds.");
  client.loop();
  delay(5000);
}
