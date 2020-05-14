/*
 * Project thermostat-photon-non-latching
 * Description:
 * Author:
 * Date:
 */
#include <Adafruit_DHT.h>

#define DHT_PIN D5
#define RELAY_CLOSE_PIN D6

nt g_debug = TRUE;

enum Mode {
  Off = 0,
  Heat = 1
};

struct State {
    Mode mode;
    int callingForHeat;
    double targetTemperature;
} g_state;

DHT g_DHT(DHT_PIN, DHTTYPE);
double g_humidity = NAN;     // Particle variable
double g_temperature = NAN;  // Particle variable

double g_targetTemp = 18.0; // TODO read from EEPROM on startup

int computeTargetOperatingState() {
  if (g_targetTemp == NAN) {
    // don't have a valid temperature
    if (g_debug) Serial.println("\nAssertion Failure: Don't have a valid target temperature");
    return FALSE;
  }
  
  if (g_debug) {
     char buffer [1024];
     sprintf(buffer, "computeOS: temperature = %f, targetTemp: %f", g_temperature, g_targetTemp);
    Serial.println(buffer);
  }

  // On/Off Range
  // We don’t want the fireplace turning on and off all the time so we let the
  // temperature go a little over before we turn it off and a little under
  // before we turn it on again. +/-0.5ºC is the minimum granularity based on
  // the sensitivity of the sensor we are using so we’ll use that.
  if (g_temperature >= (g_targetTemp + 0.5)) {
    if (g_debug) Serial.println("debug: computing OperatingState to Off");
    return FALSE;
  }
  else if (g_temperature <= (g_targetTemp - 0.5) ) {
    if (g_debug) Serial.println("debug: computing OperatingState to On");
    return TRUE;
  }
  else {
    // leave it at whatever the current state is
    if (g_debug) Serial.println("debug: leaving the operating state the way it is");
    return g_state.callingForHeat;
  }
} 

void setOperatingState(int callForHeat) {
  // TODO add logic to check state before changin the pin

  int pin_state;
  if (callForHeat) {
    digitalWrite(pin, HIGH);
  }
  else {
    digitalWrite(pin, LOW);
  }
}

// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  if (g_debug) {
      Serial.begin(9600);            // open the serial port at 9600 bps:
      Serial.println("\nSetup...");  // prints another carriage return
  }
 
  // READ Target Temp from EEPROM
  g_state.callingForHeat = FALSE;
  g_state.mode = Heat;

  Particle.variable("temperature", &g_temperature, DOUBLE);
  Particle.variable("humidity", &g_humidity, DOUBLE);
  g_DHT.begin();
        
  Particle.variable("Calling4Heat", &g_state.callingForHeat, INT);

  pinMode(RELAY_CLOSE_PIN, OUTPUT);
  digitalWrite(RELAY_CLOSE_PIN, LOW);

  //
  // TODO load state from EEPROM
  //



}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  int failure = 0;
    
  float humidity = g_DHT.getHumidity();
  delay(50);
  float temperature = g_DHT.getTempCelcius();

  if (temperature == NAN) {
      if (g_debug) Serial.println("FAILED to read Temperature");
      failure |= ERROR_READING_TEMPERATURE;
  }
  if (humidity == NAN) {
      if (g_debug) Serial.println("FAILED to read Humidity");
      failure |= ERROR_READING_HUMIDITY;
  }

  //TODO write changes in targetTemp to EEPROM

  // update global (particle) variables
  g_temperature = (double)temperature;
  g_humidity = (double)humidity;

  //time_t now = millis();

  int callingForHeat = computeTargetOperatingState();
  if (callingForHeat != g_state.callingForHeat) {
    g_state.callingForHeat = callingForHeat;
    // set latching relay based operating state
    setLatchingOperatingState(g_state.callingForHeat);
    // g_intOpState = (int) g_operatingState;
  }



}