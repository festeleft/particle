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

#include "ThermostatKit.h"

// D0, D1 used for serial communcation to display
// D2 Button A on display
// D3 Button B on display
// D4 Button C on display

#define DHT_PIN D5

#define THERMOSTAT_ON_PIN D7
#define THERMOSTAT_OFF_PIN D6

// Rotary Encoder #377B (AdaFruit)
#define DIAL_PIN_1 A3 // rotation pin 1
#define DIAL_PIN_2 A4 // rotation pin 2
#define DIAL_BUTTON_PIN A5 // dial push button

#define DHTTYPE DHT11 

#define ERROR_READING_HUMIDITY 1
#define ERROR_READING_TEMPERATURE 2
#define ERROR_ILLEGAL_COMMAND 4

Cranbrooke::ThermostatKit::Component * components[] = {
  new Cranbrooke::ThermostatKit::DHTSensor(DHT_PIN, DHT22),
  new Cranbrooke::ThermostatKit::AdafruitOLEDDisplay(),
  new Cranbrooke::ThermostatKit::RotaryController(DIAL_PIN_1, DIAL_PIN_2),
  NULL
};

Cranbrooke::ThermostatKit::Thermostat *Thermostat
    = new Cranbrooke::ThermostatKit::Thermostat(components,
          new Cranbrooke::ThermostatKit::LatchingRelay(THERMOSTAT_ON_PIN, THERMOSTAT_OFF_PIN, 500),
          NULL, NULL);

int gDebug = FALSE;

DHT gDHT(DHT_PIN, DHTTYPE);
double gHumidity = NAN;     // Particle variable
double gTemperature = NAN;  // Particle variable

double gTargetTemp = 18.0; // TODO read from EEPROM on startup

// setup() runs once, when the device is first turned on.
// Put initialization like pinMode and begin functions here.
void setup() {

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

  // initialize latching relay
  //gState.callingForHeat = FALSE;
  //setLatchingOperatingState(FALSE);
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
void loop() {
  Thermostat->loop();

  int failure = 0;

  // TODO - maybe don't read these on every loop?    
  float humidity = gDHT.getHumidity();
  // delay(50); TODO put this back when we do the above
  float temperature = gDHT.getTempCelcius();

  if (temperature == NAN) {
      if (gDebug) Serial.println("FAILED to read Temperature");
      failure |= ERROR_READING_TEMPERATURE;
  }
  if (humidity == NAN) {
      if (gDebug) Serial.println("FAILED to read Humidity");
      failure |= ERROR_READING_HUMIDITY;
  }

  // update global (particle) variables
  gTemperature = (double)temperature;
  gHumidity = (double) humidity;

  //time_t now = millis();
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

