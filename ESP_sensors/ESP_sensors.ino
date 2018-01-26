#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include "DHT.h"

#define DHTPIN 16     // what pin we're connected to
#define DHTTYPE DHT21   // DHT 21 (AM2301)
#define BMP_SCK 14
#define BMP_MISO 15
#define BMP_MOSI 12
#define BMP_CS 13

DHT dht(DHTPIN, DHTTYPE, 15);

Adafruit_BMP280 bmp(BMP_CS, BMP_MOSI, BMP_MISO,  BMP_SCK);

void setup() {
  Serial.begin(9600);
  
  if (!bmp.begin()) {  
    Serial.println(F("Could not find a sensor, check wiring!"));
    while (1);
  }
  dht.begin();
}

void loop() {
    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.print(" *C\t");
    
    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure() / 133.333);
    Serial.println(" Pa");

    Serial.print("Humidity: "); 
    Serial.print(dht.readHumidity() / 1.11);
    Serial.print(" %\t");
    Serial.print("Temperature: "); 
    Serial.print(dht.readTemperature());
    Serial.print(" *C ");
    
    Serial.println();
    delay(2000);
}
