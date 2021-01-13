#include <Arduino.h>
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/telegram-request-esp32-esp8266-nodemcu-sensor-readings/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

  Project created using Brian Lough's Universal Telegram Bot Library: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
*/

#ifdef ESP32
  #include <WiFi.h>
#else
  #include <ESP8266WiFi.h>
#endif
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h> // Universal Telegram Bot Library written by Brian Lough: https://github.com/witnessmenow/Universal-Arduino-Telegram-Bot
#include <ArduinoJson.h>
// #include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_NeoPixel.h>

// Replace with your network credentials
const char* ssid = "Your SSID";
const char* password = "Your Pasword";

// Use @myidbot to find out the chat ID of an individual or a group
// Also note that you need to click "start" on a bot before it can
// message you
#define CHAT_ID "Yur Chat ID"

// Initialize Telegram BOT
#define BOTtoken "1234567890: Your Bot Token here"  // your Bot Token (Get from Botfather)
#define MLOCK_HOLD_TIME 2000    // lock enrgise time should not be more than 8000 (8 sec)

WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

void LightNeoPixel(uint32_t neo_colour);

//Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;
unsigned long LockDeEnrgCntr;
const int MLockPin = 13;        // connected to GPIO 13
const int LEDLightPin = 14;     // connected to GPIO 14
const int MS_InputPin = 15;     // connected to GPIO 15
const int SndSensPin = 12;      // connected to GPIO 12
const int LEDSwitchPin = 0;     // connected to GPIO 0
const char helpMsg[] = "Use the following command \n \
1. /start [See this help again] \n \
2. /readings [for Sensor Status] \n \
3. /openmlock [to open lock] \n \
4. /dlightON [turn ON door light] \n \
5. /dlightOFF [turn OFF door light";


int NeoPixelPin = LEDLightPin;  // Which pin is connected to the NeoPixels?
int numPixels   = 1;            // Number of NeoPixels attached.

// NeoPixel color format & data rate. See the strandtest example for
// information on possible values.
int pixelFormat = NEO_GRB + NEO_KHZ800;

// Rather than declaring the whole NeoPixel object here, we just create
// a pointer for one, which we'll then allocate later...
Adafruit_NeoPixel *pixels;


// BME280 connect to ESP32 I2C (GPIO 21 = SDA, GPIO 22 = SCL)
// BME280 connect to ESP8266 I2C (GPIO 4 = SDA, GPIO 5 = SCL)
// Adafruit_BME280 bme;         

// Get BME280 sensor readings and return them as a String variable
String getReadings(){
  float temperature, humidity;
  temperature = 26.8; //bme.readTemperature();
  humidity = 92.3;  //bme.readHumidity();
  String message = "Temperature: " + String(temperature) + " ºC \n";
  message += "Humidity: " + String (humidity) + " % \n";
  return message;
}

//Handle what happens when you receive new messages
void handleNewMessages(int numNewMessages) {
  Serial.print("New Message/s: ");
  Serial.println(String(numNewMessages));

  for (int i=0; i<numNewMessages; i++) {
    // Chat id of the requester
    String chat_id = String(bot.messages[i].chat_id);
    if (chat_id != CHAT_ID){
      bot.sendMessage(chat_id, "Unauthorized user", "");
      continue;
    }
    
    // Print the received message
    String text = bot.messages[i].text;
    Serial.println(text);

    String from_name = bot.messages[i].from_name;

    if (text == "/start") {
      String welcome = "Welcome, " + from_name + ".\n";
      //welcome += "Use the following command.\n\n";
      //welcome += "/readings \n";
      bot.sendMessage(chat_id, welcome, "");
      bot.sendMessage(chat_id, helpMsg, "");
    }
    else if (text == "/readings") {
      String readings = getReadings();
      bot.sendMessage(chat_id, readings, "");
    }  
    else if (text == "/openmlock") {
      LockDeEnrgCntr = millis() + MLOCK_HOLD_TIME;    // de-energisse lock after 3 seconds
      digitalWrite(MLockPin, HIGH);                   // Send Signal to unlock door
      String readings = "Main Door Lock Opened";      // send Message 
      bot.sendMessage(chat_id, readings, "");
    } 
    else if (text == "/dlightON") {
      //digitalWrite(LEDLightPin, HIGH);              // turn ON door light
      LightNeoPixel(0x00FFFFFF);
      String readings = "Door Light ON";              // feed back message
      bot.sendMessage(chat_id, readings, "");
    } 
    else if (text == "/dlightOFF") {
      //digitalWrite(LEDLightPin, LOW);               // Turn OFF door light 
      LightNeoPixel(0x00000000);                      // Turn off pixel
      String readings = "Door Light OFF";             // Feed back message
      bot.sendMessage(chat_id, readings, "");
    } 
    else
    {
      /* code */
    bot.sendMessage(chat_id, helpMsg, "");
    }
  }
}

void LightNeoPixel(uint32_t neo_colour)
{
  pixels->clear(); // Set all pixel colors to 'off'
  for(int i=0; i<numPixels; i++) { // For each pixel...
    pixels->setPixelColor(i, neo_colour); // 24 bit colour value packed
    pixels->show();   // Send the updated pixel colors to the hardware.
  }
}

void setup() {
  Serial.begin(115200);

  #ifdef ESP8266
    client.setInsecure();
  #endif
  // configure sensor, actuator pins
  pinMode(MLockPin, OUTPUT);                  // Lock signal 
  pinMode(LEDLightPin, OUTPUT);               // Light o/p possible PWM
  pinMode(MS_InputPin, INPUT_PULLUP);         // Motion sensor input pin with pull-up
  pinMode(SndSensPin, INPUT);                 // Sound sensor input pin with pull-up
  pinMode(LEDSwitchPin, INPUT);               // LED Light ON/OFF pin

  // default value for actuators
  digitalWrite(MLockPin, LOW);                // do not drive or energise lock signal
  digitalWrite(LEDLightPin, LOW);             // 
    
  pixels = new Adafruit_NeoPixel(numPixels, NeoPixelPin, pixelFormat); // Create a new NeoPixel object
  pixels->begin();                             // start Neo pixel service

// Init BME280 sensor
/*  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
*/  
  // Connect to Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi..");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  // Print ESP32 Local IP Address
  Serial.println("\n --------------\n IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if(digitalRead(MLockPin) != LOW)  {
    if(millis() > LockDeEnrgCntr) {
      digitalWrite(MLockPin, LOW);
    }
  }
  if (millis() > lastTimeBotRan + botRequestDelay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }
}