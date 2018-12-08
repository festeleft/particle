/*
 * Project fireplace-thermostat
 * Description:
 * Author:
 * Date:
 */


#include "Adafruit_DHT.h"

#define DHTPIN D1 // CHANGEME: Digital Pin on your board that the DHT sensor is
                  // connected to

//#define DHTTYPE DHT11 // CHANGEME: To the type of DHT sensor you are using
#define DHTTYPE DHT22   // CHANGEME: To the type of DHT sensor you are using

#define ERROR_READING_HUMIDITY 1
#define ERROR_READING_TEMPERATURE 2
#define ERROR_ILLEGAL_COMMAND 4

#define POT_PIN A1

enum State {
  Off   = 0,
  On    = 1,
  Error = -1
} g_state = Off;

DHT dht(DHTPIN, DHTTYPE);

double g_humidity = NAN;     // Particle variable
double g_temperature = NAN;  // Particle variable

double g_dialTemp = NAN;
double g_targetTemp = NAN;

double g_rotation = 0;

// setup() runs once, when the device is first turned on.
// Put initialization like pinMode and begin functions here.
void setup() {
  Particle.variable("temperature", &g_temperature, DOUBLE);
  Particle.variable("humidity", &g_humidity, DOUBLE);

  // TODO - remove publishing these after eveything is debugged
  //Particle.variable("rotation", &g_rotation, DOUBLE);
  Particle.variable("dialTemp", &g_dialTemp, DOUBLE);

  //Particle.variable("state", &g_state, INT);

  // TODO need to publish remoteoverride and add state
  // Particle.function("publish", publishCommand);

  pinMode(POT_PIN, INPUT);

  dht.begin();
}

// TODO Blink LED on ERROR

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  float humidity;     // humidity
  float temperature;  // temperature
  int failure = 0;

  float rotation;
  double unrounded_dial_temp;
  double dial_temp;

  //
  // Measure all the inputs
  //

  //
  // measure the temperature and humidity
  //
  // TODO
  // Since temperature and humidity reading is slow we don't really have to
  // read then every time let's do it every 20s or so; a fireplace is not going
  // to change the heat in the room so much faster than that.
  //
  failure = 0;  // clear failure flag before attempt to read the sensors
  humidity = dht.getHumidity();
  delay(50);
  temperature = dht.getTempCelcius();

  //
  // measure the rotation of the potentiometer to determine the target temperature
  //
  rotation = analogRead(POT_PIN);

  // Mapping the Potentiometer reading to a temperature between
  // Given a desired dial range of 6ºC and 30ºC with .5 increments
  // We round to the nearest 0.5 ºC to match the granularity of the sensor
  // 4100 / 25 -> Interval of 164 or 82 per half degree
  unrounded_dial_temp = rotation / 82;
  // unrounded_dial_temp is twice what it should be since we divided by 82 instead of
  // 164 but we want to round it before we divide by 2 to get it to line up on
  // even .5 degree increments
  dial_temp = round(unrounded_dial_temp) / 2;

  // On/Off Range
  // We don’t want the fireplace turning on and off all the time so we let the
  // temperature go a little over before we turn it off and a little under
  // before we turn it on again. +/-0.5ºC is the minimum granularity based on
  // the sensitivity of the sensor we are using so we’ll use that.

  // map the potentionmeter to a dial temperature

  // if the dial temperture has not chaged in a second then update the "target temperature"

  // if the target temperature is higher than the current temperature then state = ON

  // if the temperature is the same or higher than the target temperature then state = OFF

  // if state = ON then latching relay is not closed then close it and make sure the LED is on

  // if state = OFF then latching relay is open then open it.

 

  if (temperature == NAN) {
    Serial.println("FAILED to read Temperature");
    failure |= ERROR_READING_TEMPERATURE;
  }
  if (humidity == NAN) {
    Serial.println("FAILED to read Humidity");
    failure |= ERROR_READING_HUMIDITY;
  }

  // update global (particle) variables
  g_temperature = (double) temperature;
  g_humidity = (double) humidity;
 
  // TODO remove once we have debugged
  g_rotation = (double) rotation;
  g_dialTemp = dial_temp;


}
