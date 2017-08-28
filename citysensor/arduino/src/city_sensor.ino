/** 
 *  Birmingham City Sensor 
 *  
 *  Arduino Code for the Birmingham City
 *  Sensor board, rev1.0
 *  
 *  Code for Birmingham
 *  http://www.codeforbirmingham.org/
 */

#include "Arduino.h"
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

// how many samples we take before determining
// the noise level
#define NOISE_SAMPLES 1024

// how long between reading from the low-priority
// sensors, in milliseconds
#define LOW_PRIORITY_PERIOD 10000

// how often we write output, in milliseconds
#define OUTPUT_PERIOD 5000

DHT dht(DHTPIN, DHTTYPE);
float temperature = 0.0;
float humidity = 0.0;
float noise = 0;
unsigned int light = 0;
float particle = 0;
float noiseBuffer[NOISE_SAMPLES];
unsigned int noiseIndex;

// store the last time we read from the low-priority
// sensors, so that we only need to read from them
// so often.
unsigned long lastLowPriorityRead = 0;
unsigned long lastOutput = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  dht.begin();

  pinMode(NOISEPIN, INPUT);
  pinMode(LIGHTPIN, INPUT);
  pinMode(DUST_LED_PIN, OUTPUT);
  digitalWrite(DUST_LED_PIN, HIGH);
}

bool isAfter(unsigned long startTime, unsigned long currentTime, unsigned long period)
{
  return currentTime - startTime < period;
}

void readTempHumidity()
{
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
}

void readNoise()
{
  if (noiseIndex >= NOISE_SAMPLES) {
    calculateNoiseLevel();
    noiseIndex = 0;
  }
  if (noiseIndex < NOISE_SAMPLES)
    noiseBuffer[noiseIndex++] = analogRead(NOISEPIN);
}

void calculateNoiseLevel() {
  unsigned int sample;
  unsigned int sampleMax = 0;
  unsigned int sampleMin = 1024;
  for (int i=0; i<NOISE_SAMPLES; i++) {
    sample = noiseBuffer[i];
    if (sample < 1024) {
      if (sample > sampleMax)
        sampleMax = sample;
      if (sample < sampleMin)
        sampleMin = sample;
    }
  }
  // output noise peak-to-peak in volts
  noise = (sampleMax - sampleMin) * 5.0 / 1024;
}

void readLight()
{
  light = analogRead(LIGHTPIN) * 5.0 / 1024;
}

void readParticle()
{
  digitalWrite(DUST_LED_PIN,LOW); // power on the LED
  delayMicroseconds(DUST_SAMPLE_TIME);
  float voMeasured = analogRead(DUST_VOUT_PIN); // read the dust value
  delayMicroseconds(DUST_DELTA_TIME);
  digitalWrite(DUST_LED_PIN,HIGH); // turn the LED off
  delayMicroseconds(DUST_SLEEP_TIME);
  
  // 0 - 3.3V mapped to 0 - 1023 integer values
  // recover voltage
  float calcVoltage = voMeasured * (5.0 / 1024);
 
  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  particle = 0.17 * calcVoltage - 0.1;  
}

void readLowPriority()
{
  unsigned long t = millis();
  if (isAfter(lastLowPriorityRead, t, LOW_PRIORITY_PERIOD)) {
    readTempHumidity();
    readParticle();
    readLight();
    lastLowPriorityRead = t;
  }
}

void readHighPriority()
{
  readNoise();
}

void readSensorData()
{
  readLowPriority();
  readHighPriority();
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

  unsigned long t = millis();
  if (isAfter(lastOutput, t, OUTPUT_PERIOD)) {
    outputSensorData();
    lastOutput = t;
  }
}
