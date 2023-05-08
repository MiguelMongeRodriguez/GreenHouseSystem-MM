#include <DHT.h>
#include <stdio.h>
#include <AccelStepper.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

void sensores();

// Timers auxiliar variables
long now = millis();
long lastMeasure = 0;

// bools to check if Actuators are On or Off
bool Puerta = false;
bool LedUV = false;
// bools to automate the behaviour of the greenhouse
bool Auto = true;

/* -------- Automatismos: thresholds ---------- */
float ThresholdTemp = 0;

struct Data
{
  float tempC;
  float humi;
  int luz;
  float moisture[6] = {0};
} Datos;

/* ------------------ MQTT --------------------*/
const char* ssid = "MOVISTAR_41AC";
const char* password = "Nme63wKxVRBXFite2F93";

const char* mqtt_server = "192.168.1.51";

WiFiClient espClient;
PubSubClient client(espClient);

/* ------------------ Motor Puerta --------------------*/
const int stepsPerRevolution = 1600;  // change this to fit the number of steps per revolution

// ULN2003 Motor Driver Pins
#define IN1 5
#define IN2 4
#define IN3 0
#define IN4 2

// initialize the stepper library
AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

/* ------------------ Sensores --------------------*/
// Definir los pines utilizados para el multiplexor
int s0 = 12;
int s1 = 13;
int s2 = 15;
//int s3 = 12;
int inPin = A0; // Pin de entrada analógica
int DHTPin = 3;
int UVPin = 14;
int FanPin = 16;

#define DHTTYPE DHT11   
// Initialize DHT sensor.
DHT dht(DHTPin, DHTTYPE);

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
  
  String messageTemp;
  /*Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");*/
  
  for (int i = 0; i < length; i++) {
    //Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  
  /* --------------------------------------- Manual --------------------------------------*/
  if(topic=="GreenHouse/Auto"){
      if(messageTemp == "true"){
        Auto = true;
        Serial.println("Automatismos activados");
      }else{
        Auto = false;
        Serial.println("Automatismos desactivados");
      }
  }
  else if(topic=="GreenHouse/Thresholds/Temp"){
      ThresholdTemp = messageTemp.toFloat();
      Serial.println("Se ha modificado el umbral de temperatura a :" + String(ThresholdTemp) + "ºC\n");
  }
  else if(topic=="GreenHouse/Actuators/Motor"){
      if(messageTemp == "On" && Puerta == false){
        stepper.moveTo(stepsPerRevolution);
        stepper.runToPosition();
        Serial.println("Abriendo Puerta");
        Puerta = true;
        digitalWrite(FanPin, HIGH);
      }
      else if(messageTemp == "Off" && Puerta == true){
        stepper.moveTo(-stepsPerRevolution);
        stepper.runToPosition();
        Serial.println("Cerrando Puerta");
        Puerta = false;
        digitalWrite(FanPin, LOW);
      }
      else{
        Serial.println("No es posible");
      }
  }
  else if(topic=="GreenHouse/Actuators/LedUV"){
      if(messageTemp == "On" && LedUV == false){
        digitalWrite(UVPin, HIGH);
        Serial.println("Encendiendo Leds UV");
        LedUV = true;
      }
      else if(messageTemp == "Off" && LedUV == true){
        digitalWrite(UVPin, LOW);
        Serial.println("Apagando Leds UV");
        LedUV = false;
      }
      else{
        Serial.println("No es posible");
      }
  }
  else{
    
  }
  /*  ---------------------------------- Automatismos  ---------------------------------- */
  if (Auto == true){
    if(topic=="GreenHouse/Sensors/Temp"){
      if(messageTemp.toInt() >= ThresholdTemp && Puerta == false){
        client.publish("GreenHouse/Actuators/Motor", "On");
      }
      else if(messageTemp.toInt() < ThresholdTemp && Puerta == true){
        client.publish("GreenHouse/Actuators/Motor", "Off");
      }
    }
  }
  else{
    
  }
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
      client.subscribe("GreenHouse/Sensors/Soil/SM0");
      client.subscribe("GreenHouse/Sensors/Soil/SM1");
      client.subscribe("GreenHouse/Sensors/Soil/SM2");
      client.subscribe("GreenHouse/Sensors/Soil/SM3");
      client.subscribe("GreenHouse/Sensors/Soil/SM4");
      client.subscribe("GreenHouse/Sensors/Soil/SM5");
      client.subscribe("GreenHouse/Sensors/Temp");
      client.subscribe("GreenHouse/Sensors/LDR");
      client.subscribe("GreenHouse/Actuators/Fan");
      client.subscribe("GreenHouse/Actuators/Motor");
      client.subscribe("GreenHouse/Actuators/LedUV");
      client.subscribe("GreenHouse/Auto");
      client.subscribe("GreenHouse/Thresholds/Temp");
      
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // Configurar los pines como salidas o entradas
  pinMode(s0, OUTPUT);
  pinMode(s1, OUTPUT);
  pinMode(s2, OUTPUT);
  //pinMode(s3, OUTPUT);
  pinMode(inPin, INPUT);
  pinMode(UVPin, OUTPUT);
  pinMode(FanPin, OUTPUT);
  // initialize the sensor
  dht.begin(); 

  // set the speed and acceleration
  stepper.setMaxSpeed(500);
  stepper.setAcceleration(1000);
  
  // initialize the serial port
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
  if (now - lastMeasure > 5000) {
    lastMeasure = now;

    sensores();
    for(int i = 0; i < 6; i++) {
        static char SM[6];
        dtostrf(Datos.moisture[i], 5, 2, SM);
  
        Serial.print("Moisture " + String(i) + (": "));
        Serial.print(SM);
        Serial.println("%");
        
        String topic = "GreenHouse/Sensors/Soil/SM" + String(i); 
        client.publish(topic.c_str(), SM);
    }
        
    static char LDR[6];
    dtostrf(Datos.luz, 5, 2, LDR);
    static char Temp[6];
    dtostrf(Datos.tempC, 5, 2, Temp);
    static char Hum[6];
    dtostrf(Datos.humi, 5, 2, Hum);

    Serial.print("Luminosidad: ");
    Serial.print(LDR);
    Serial.println("%");

    Serial.print("Temperatura: ");
    Serial.print(Temp);
    Serial.println("ºC");
    
    Serial.print("Humedad: ");
    Serial.print(Hum);
    Serial.println("ºC");
    
    client.publish("GreenHouse/Sensors/LDR", LDR);
    client.publish("GreenHouse/Sensors/Temp", Temp);
    client.publish("GreenHouse/Sensors/Humi", Hum);
  }
}
void sensores() {
  /* ------------------ Sensores --------------------*/
  // Seleccionar el canal del multiplexor (0-15)
  for(int i = 0; i < 8; i++) {
   
    digitalWrite(s0, i & 0x1);
    digitalWrite(s1, i & 0x2);
    digitalWrite(s2, i & 0x4);
    //digitalWrite(s3, i & 0x8);
     
    // Sensores de suelo 
    if (i < 6){
      int moisture = map(analogRead(A0), 0, 1023, 100, 0);
      Datos.moisture[i] = moisture;
    }
    // Sensor de temperatura y humedad del ambiente
    else if (i == 6){
      int luz = analogRead(A0);
      Datos.luz = luz;
    }
    else if (i == 7){
      float humi  = dht.readHumidity();
      float tempC = dht.readTemperature();
      Datos.humi  = humi;
      Datos.tempC = tempC;
    }
  }
}
