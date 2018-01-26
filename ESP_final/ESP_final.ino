//Time
#include <WiFiUdp.h>

//ThingSpeak
#include "ThingSpeak.h"
#include <ESP8266HTTPClient.h>

//Telegram
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>

//Sensors
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "DHT.h"

//Wifi connection to the router
const char ssid[] = "****************";     // network SSID (name)
const char password[] = "****************"; // network key

//Time
unsigned int localPort = 2390;      // local port to listen for UDP packetsconst
char* ntpServerName = "ua.pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
unsigned long startTime;
unsigned long lastTimeAsked;
unsigned long realTime;
int timeDelay = 86400;
const unsigned long seventyYears = 2208988800UL;
unsigned long epoch;
int utc = 2;

//ThingSpeak
unsigned long TSChannelNumber = 00000000;
const char * writeKey = "WRITE_KEY";

//TelegramBot
int Bot_mtbs = 250; //mean time between scan messages
long Bot_lasttime;   //last time messages' scan has been done

// Initialize Telegram BOT
#define BOTtoken "BOT_TOKEN" // Bot Token
#define weather_channel_id "CHANNEL_ID"

//Initialize DHT
#define DHTPIN 5     // what pin we're connected to
#define DHTTYPE DHT21   // DHT 21 (AM2301)
//Initialize BMP
#define BMP_SCK 14
#define BMP_MISO 15
#define BMP_MOSI 12
#define BMP_CS 13

//Time
IPAddress timeServerIP; // time.nist.gov NTP server address
WiFiUDP udp;

//Bot
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

//Sensors
DHT dht(DHTPIN, DHTTYPE, 15);
Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);

//Phrases
const int night_am = 3;
const int morning_am = 3;
String NIGHT[night_am] = {
  "Доброй ночи, господа инженеры. И пусть Сикорский оберегает ваш сон.\n\n",
  "Second Phrase.\n\n",
  "Third Phrase.\n\n"
};

String MORNING[morning_am] = {
  "First Phrase.\n\n",
  "Second Phrase.\n\n",
  "Third Phrase.\n\n"
};

//Data
float streetTemp;
float roomTemp;
float pressure;
float humidity;
boolean channelUpdated = false;
boolean thingSpeakUpdated = false;

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  randomSeed(analogRead(0));

  // Set WiFi to station mode and disconnect from an AP if it was Previously
  // connected
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  // Attempt to connect to Wifi network:
  Serial.print("\nConnecting Wifi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("\tIP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("\tLocal port: ");
  Serial.println(udp.localPort());
  WiFi.hostByName(ntpServerName, timeServerIP);

  //Start sensors
  if (!bmp.begin()) {  
    Serial.println(F("Could not find a sensor, check wiring!"));
    while (1);
  }
  dht.begin();

  ThingSpeak.begin(client);

  startTime = getEpoch();
  lastTimeAsked = startTime;

  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  blink(3, 100, 300);
  readData();
}

void loop() {
  realTime = startTime + millis()/1000;
  epoch = realTime - seventyYears;
  //printTime();
  
  //Update Data Cycle
  if (getMinute() % 15 == 0)  {
    readData();
  }

  //Bot Cycle
  if (millis() > Bot_lasttime + Bot_mtbs)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    Bot_lasttime = millis();
    blink(1, 50, 10);
  }
    
  //Channels Cycle
  if (((getHour() == 6 || getHour() == 8 || getHour() == 12 || getHour() == 16 || getHour() == 20 || getHour() == 22) && getMinute() == 0) && !channelUpdated) {
    updateChannel();
    channelUpdated = true;
  }
  if ((getMinute() == 0 || getMinute() == 30) && !thingSpeakUpdated) {
    updateThingSpeak();
    thingSpeakUpdated = true;
  }
  if (thingSpeakUpdated && (getMinute() != 0 && getMinute() != 30)) {
    thingSpeakUpdated = false;
    channelUpdated = false;
  }  
}

void handleNewMessages(int numNewMessages) {
  for(int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    Serial.println(text);
    if (text == "/weather") {
      bot.sendMessage(chat_id, generateWeather(), "");
    }
    if (text == "/start") {
      String welcome = "Welcome from Weather_bot\n";
      welcome = welcome + "/weather : to get real-time weather on the KPI\n";
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void blink(int times, int turnedOn, int turnedOff) {
  for(int i = 0; i < times; i++) {
      digitalWrite(LED_BUILTIN, LOW);
      delay(turnedOn);
      digitalWrite(LED_BUILTIN, HIGH);
      delay(turnedOff);
  }
}

void readData() {
    streetTemp = dht.readTemperature();
    roomTemp = bmp.readTemperature();
    pressure = bmp.readPressure() / 133.333;
    humidity = dht.readHumidity();
}

int getHour() {
  return ((epoch + utc * 3600L)  % 86400L) / 3600;
}

int getMinute() {
  return (epoch  % 3600) / 60;
}

int getSeconds() {
  return epoch % 60;
}

String getStrTime() {
  String result = "";
  int hours = getHour();
  if (hours == 24) hours = 0;
  int minutes = getMinute();
  if (hours < 10) result = "0";
  result = result + hours + ":";
  if (minutes < 10) result = result + "0";
  result = result + minutes;

  return result;
}

void printTime() {
  Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
  Serial.print(getHour());
  Serial.print(':');
  Serial.print(getMinute());
  Serial.print(':');
  Serial.println(getSeconds()); // print the second
}

unsigned long getEpoch() {
  int cb;
  do{
      sendNTPpacket(timeServerIP); // send an NTP packet to a time server
      // wait to see if a reply is available
      delay(2000);
    
      cb = udp.parsePacket();
    }while(!cb);

    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, extract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;

    return secsSince1900;
}

unsigned long sendNTPpacket(IPAddress& address) {
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void updateThingSpeak() {
  HTTPClient http;
  http.begin("http://api.thingspeak.com/update?api_key=" + (String)writeKey + 
              "&field1=" + String(streetTemp) +   //
              "&field2=" + String(pressure) +     //
              "&field3=" + String(humidity) +     //
              "&field4=" + String(roomTemp)       //
              );
  http.GET();
  delay(500);
  http.end();
}

String generateWeather() {
  String res = "По данным на " + getStrTime() + " температура воздуха на территории студгородка: " + String(int(streetTemp)) + "°C" +
                  "\nВлажность воздуха: " + int(humidity) + "%" +
                  "\nДавление: " + int(pressure) + " мм.рт.ст.";
  return res;
}

void updateChannel() {
  String result = "";
  if (getHour() == 22) result = result + NIGHT[random(night_am)];
  if (getHour() == 6) result = result + MORNING[random(morning_am)];
  result = result + generateWeather();

  bot.sendMessage(weather_channel_id, result, "");
  }
