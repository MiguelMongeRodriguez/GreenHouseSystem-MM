#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include "DHT.h"

#define DHTTYPE DHT11   

const char* ssid = "MOVISTAR_41AC";
const char* password = "Nme63wKxVRBXFite2F93";

const char* mqtt_server = "192.168.1.59";

WiFiClient espClient;
PubSubClient client(espClient);

const int DHTPin = 5;
const int led = 4;

// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

struct Data
{
  float tempC;
  float humi;
  int luz;
} Datos;

// Don't change the function below. This functions connects your ESP8266 to your router
void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
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

// This functions is executed when some device publishes a message to a topic that your ESP8266 is subscribed to
// Change the function below to add logic to your program, so when a device publishes a message to a topic that 
// your ESP8266 is subscribed you can actually do something
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

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic room/lamp, you check if the message is either on or off. Turns the lamp GPIO according to the message
  if(topic=="Pipote/Led"){
      Serial.print("Changing Room lamp to ");
      if(messageTemp == "ON"){
        digitalWrite(led, HIGH);
        Serial.print("ON");
      }
      else if(messageTemp == "OFF"){
        digitalWrite(led, LOW);
        Serial.print("OFF");
      }
  }
  Serial.println();
}

// This functions reconnects your ESP8266 to your MQTT broker
// Change the function below if you want to subscribe to more topics with your ESP8266 
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    /*
     YOU MIGHT NEED TO CHANGE THIS LINE, IF YOU'RE HAVING PROBLEMS WITH MQTT MULTIPLE CONNECTIONS
     To change the ESP device ID, you will have to give a new name to the ESP8266.
     Here's how it looks:
       if (client.connect("ESP8266Client")) {
     You can do it like this:
       if (client.connect("ESP1_Office")) {
     Then, for the other ESP:
       if (client.connect("ESP2_Garage")) {
      That should solve your MQTT multiple connections problem
    */
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
      client.subscribe("Pipote/Led");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

// The callback function is what receives messages and actually controls the LEDs
void setup() {
  pinMode(led, OUTPUT);
  dht.begin();
  
  Serial.begin(115200);
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  if(!client.loop())
    client.connect("ESP8266Client");

  now = millis();
  // Publishes new temperature and humidity every 10 seconds
  if (now - lastMeasure > 10000) {
    lastMeasure = now;
    
    sensores();
    
    static char temperatureTemp[7];
    dtostrf(Datos.tempC, 6, 2, temperatureTemp);
   
    static char humidityTemp[7];
    dtostrf(Datos.humi, 2, 0, humidityTemp);
    
    static char lumTemp[7];
    dtostrf(Datos.luz, 6, 2, lumTemp);

    Serial.print(": ");
    Serial.println();
    

    // Publishes Temperature and Humidity values
    client.publish("Pipote/Temperatura", temperatureTemp);
    client.publish("Pipote/Humedade", humidityTemp);
    client.publish("Pipote/Luz", lumTemp);
  }
} 

void sensores() {
  //Reads
  int luz = analogRead(A0);
  float humi  = dht.readHumidity();
  float tempC = dht.readTemperature();
  
  // check if any reads failed
  if (isnan(humi) || isnan(tempC) || isnan(luz)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Humidity: ");
    Serial.print(humi);
    Serial.print("%");
    Serial.print("  |  "); 
    Serial.print("Temperature: ");
    Serial.print(tempC);
    Serial.print("°C ~ ");
    Serial.print("Luminosity: ");
    Serial.print(luz);
    Serial.print(" ~ ");
    Serial.println(" ");
    
  Datos.luz = luz;
  Datos.humi  = humi;
  Datos.tempC = tempC;
  }
}
