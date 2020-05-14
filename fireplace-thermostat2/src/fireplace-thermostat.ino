/*
 * Project fireplace-thermostat
 * Description:
 * Author:
 * Date:
 */

// DisplayMode
// PRESS A     Heat to: <traget temp>
// PRESS B     <current temp>
// PRESS C     <current humidity>
// 
// Turn Knob > Mode A
// if not Default State then revert to default state after 5 seconds

#include "application.h"
#include "ThermostatKit.h"

// D0, D1 used for serial communcation to display
// D2 Button A on display
// D3 Button B on display
// D4 Button C on display
#define TEMPERATURE_DOWN D2 // Button A on display
#define MODE D3 // Button B on display
#define TEMPERATURE_UP D4 // Button C on display

#define DHT_PIN D5

#define THERMOSTAT_ON_PIN D6 // pin for non latching
#define THERMOSTAT_OFF_PIN D7

#define DHTTYPE DHT11

#define ERROR_READING_HUMIDITY 1
#define ERROR_READING_TEMPERATURE 2
#define ERROR_ILLEGAL_COMMAND 4

//Cranbrooke::ThermostatKit::DHTSensor * DHT = new Cranbrooke::ThermostatKit::DHTSensor(DHT_PIN, DHT22);
//Cranbrooke::ThermostatKit::AdafruitOLEDDisplay * Display = new Cranbrooke::ThermostatKit::AdafruitOLEDDisplay();
//new Cranbrooke::ThermostatKit::RotaryController(DIAL_PIN_1, DIAL_PIN_2);
//Cranbrooke::ThermostatKit::Relay * Relay = new Cranbrooke::ThermostatKit::NonLatchingRelay(D6);

Cranbrooke::ThermostatKit::Thermostat *Thermostat
    = new Cranbrooke::ThermostatKit::Thermostat(
          new Cranbrooke::ThermostatKit::DHTSensor(DHT_PIN, DHT22),
          new Cranbrooke::ThermostatKit::AdafruitOLEDDisplay(),
          new Cranbrooke::ThermostatKit::NonLatchingRelay(THERMOSTAT_ON_PIN),
          NULL, // no cooling relay
          NULL); // no fan relay

int gDebug = FALSE;

// setup() runs once, when the device is first turned on.
// Put initialization like pinMode and begin functions here.

int setTargetTemperature(String target_str) {
  double old_temp = Thermostat->targetTemperature,
    new_temp = old_temp;

  if (!strcmp(target_str, "up")) {
    new_temp = Thermostat->stepTargetTemperature(1);
  }
  else if (!strcmp(target_str, "down")) {
    new_temp = Thermostat->stepTargetTemperature(-1);
  }
  else {
    char **endptr = 0;
    double new_target = strtod(target_str, endptr);
    if (errno == ERANGE) {
      return 1; // error while parsing
    }
    new_temp = Thermostat->setTargetTemperature(new_target);
  }

  if (new_temp == old_temp) {
    return 1; // error/no change
  }

  return 0;
}

void setup() {
  Serial.begin(9600);            // open the serial port at 9600 bps:
  Serial.println("\nSetup...");  // prints another carriage return

  Thermostat->setup();

/*
  if (gDebug) {
      Serial.begin(9600);            // open the serial port at 9600 bps:
      Serial.println("\nSetup...");  // prints another carriage return
  }
  Particle.variable("temperature", &gTemperature, DOUBLE);
  Particle.variable("humidity", &gHumidity, DOUBLE);
*/
        
  //Particle.variable("Calling4Heat", &gState.callingForHeat, INT);

  delay(5000); // hold up to make sure that setup happens

  Particle.variable("targetTemperature", &Thermostat->targetTemperature, DOUBLE);
  Particle.variable("temperature", &Thermostat->temperature, DOUBLE);
  Particle.variable("humidity", &Thermostat->humidity, DOUBLE);
  Particle.variable("callingForHeat", &Thermostat->callingForHeat, BOOLEAN);

  Particle.function("setTargetTemperature", &setTargetTemperature);
  
  };

// TODO Blink LED on ERROR

// TODO
// Draws the following state examples:
//         22.5
// Off     22.5
// Heat to 24.0
//         58%
// Error

//
// Main Event loop
//
unsigned int nextUpdate = 0;

void loop() {
  Thermostat->loop();
  
  //int failure = 0;

  // update global (particle) variables
  // gTemperature = (double) temperature;
  // gHumidity = (double) humidity;
  // unsigned int now = millis();

/**
  if (now > nextUpdate) {
      // READ sensor
      Serial.println("attempting read");
      Cranbrooke::ThermostatKit::SensorReading * readings = DHT->getReadings();

      heatrelay->closeRelay(TRUE);
      //digitalWrite(D6, HIGH);

      nextUpdate = millis() + 5000;
  }
  **/
  /* 
  int callingForHeat = computeTargetOperatingState();
  if (callingForHeat != gState.callingForHeat) {
    gState.callingForHeat = callingForHeat;
    // set latching relay based operating state
    setLatchingOperatingState(gState.callingForHeat)
    ;
    // gintOpState = (int) goperatingState;
  }

  drawDisplay();
*/
}

