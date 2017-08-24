/** 
 *  Birmingham City Sensor 
 *  
 *  Arduino Code for the Birmingham City
 *  Sensor board, rev1.0
 *  
 *  Code for Birmingham
 *  http://www.codeforbirmingham.org/
 */

#include "ArduinoJson.h"
#include "DHT.h"

// pin definitions
#define DHTPIN 4
#define DHTTYPE DHT11
#define LIGHTPIN A5
#define NOISEPIN A4

// dust sensor pin defs
#define DUST_VOUT_PIN A0
#define DUST_LED_PIN 11

#define DUST_SAMPLE_TIME 280
#define DUST_DELTA_TIME 40
#define DUST_SLEEP_TIME 9680
#define DUST_SAMPLES 10

#define NOISE_SAMPLE_WINDOW 50

DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float humidity = 0.0;
float noise = 0;
int light = 0;
float particle = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht.begin();

  pinMode(NOISEPIN, INPUT);
  pinMode(LIGHTPIN, INPUT);
  pinMode(DUST_LED_PIN, OUTPUT);
  digitalWrite(DUST_LED_PIN, HIGH);
}

void readTempHumidity()
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void readNoise()
{
  unsigned int sample;
  unsigned int sampleMax = 0;
  unsigned int sampleMin = 1024;
  unsigned long start = millis();
  while (millis() - start < NOISE_SAMPLE_WINDOW) {
    sample = analogRead(NOISEPIN);
    if (sample < 1024) {
      if (sample > sampleMax)
        sampleMax = sample;
      if (sample < sampleMin)
        sampleMin = sample;
    }
  }
  // output noise in volts
  noise = (sampleMax - sampleMin) * 5.0 / 1024;
}

void readLight()
{
  light = analogRead(LIGHTPIN) * 5.0 / 1024;
}

void readParticle()
{
  int vo[DUST_SAMPLES];
  for (int i=0; i<DUST_SAMPLES; i++) {
    digitalWrite(DUST_LED_PIN,LOW); // power on the LED
    delayMicroseconds(DUST_SAMPLE_TIME);
    vo[i] = analogRead(DUST_VOUT_PIN); // read the dust value
    delayMicroseconds(DUST_DELTA_TIME);
    digitalWrite(DUST_LED_PIN,HIGH); // turn the LED off
    delayMicroseconds(DUST_SLEEP_TIME);
  }

  // take the average of all measurements
  float voMeasured = 0.0;
  for (int i=0; i<DUST_SAMPLES; i++)
    voMeasured += vo[i];
  voMeasured = voMeasured / DUST_SAMPLES;
  
  // 0 - 3.3V mapped to 0 - 1023 integer values
  // recover voltage
  float calcVoltage = voMeasured * (5.0 / 1024);
 
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  particle = 0.17 * calcVoltage - 0.1;  
}

void readSensorData()
{
  readTempHumidity();
  readLight();
  readParticle();
  readNoise();
}

void outputSensorData()
{
  StaticJsonBuffer<256> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  root["event"] = "sensor_data";
  JsonObject& data = root.createNestedObject("params");
  data["temp"] = temperature;
  data["humidity"] = humidity;
  data["noise"] = noise;
  data["light"] = light;
  data["particle"] = particle;
  root.printTo(Serial);
}

void loop() {
  // put your main code here, to run repeatedly:
  readSensorData();
  outputSensorData();
  delay(5000);
}
