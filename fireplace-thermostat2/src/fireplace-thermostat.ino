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

int setMode(String mode) {
  if (!strcmp(mode, "off")) {
    Thermostat->setMode(Cranbrooke::ThermostatKit::ALL_OFF);
    return 0;
  }
  if (!strcmp(mode, "heat")) {
    Thermostat->setMode(Cranbrooke::ThermostatKit::HEAT_ON);
    return 0;
  }

  return 1;  
}

void setup() {
  Serial.begin(9600);            // open the serial port at 9600 bps:
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

  Particle.variable("targetTemperature", &Thermostat->targetTemperature, DOUBLE);
  Particle.variable("temperature", &Thermostat->temperature, DOUBLE);
  Particle.variable("humidity", &Thermostat->humidity, DOUBLE);
  Particle.variable("callingForHeat", &Thermostat->callingForHeat, BOOLEAN);

  Particle.function("setTargetTemperature", &setTargetTemperature);
  Particle.function("setMode", &setMode);
  
  };

// TODO Blink LED on ERROR

//
// Main Event loop
//
unsigned int nextUpdate = 0;

void loop() {
  Thermostat->loop();
}

