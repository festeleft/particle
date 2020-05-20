
#include "ThermostatKit.h"

namespace Cranbrooke {
namespace ThermostatKit {

// switch to FALSE for production
static const bool DEBUG = TRUE;

void Component::sensorSetup() {};
SensorReading * Component::getReadings() { return NULL; };
void Component::controllerSetup(Thermostat * thermostat) {};
void Component::controllerLoop(Thermostat * thermostat) {};
void Component::setupRelay(){}; // TODO do we need thermo passed in?
void Component::setRelay(bool closed) {}; // called from setup or loop
void Component::displaySetup(Thermostat * thermostat) {};
void Component::drawDisplay(Thermostat * thermostat) {};

// TODO add ˚F handling
// temperature increment for F is per degree instead of per half degree for Celcius
// in case the temperature is high resolution perhaps the preferance needs to passed
// into the getReadings call

Thermostat::Thermostat(DHTSensor * sensor, AdafruitOLEDDisplay * display, Relay * heating_relay,
                                                Relay * cooling_relay, Relay * fan_relay) {
  callingForHeat = FALSE;
  callingForCooling = FALSE;
  temperature = 0;
  humidity = 0;
  // TODO double pressure = 0;

  this->sensor = sensor;
  this->display = display;
  heatingRelay = heating_relay;
  coolingRelay = cooling_relay;
  fanRelay = fan_relay;  
}

struct ThermostatPersistV1 {
  int version = 1;
  Mode mode;
  double targetTemperature;
};

void Thermostat::setup() {
  // Read any persisted values from the simulated EEPROM
  ThermostatPersistV1 stored;
  EEPROM.get(0, stored);
  if (stored.version != 1) {
    // first time use all bytes set to 0xFF; so it is safe to check for any
    // reasonable version number
    mode = ALL_OFF; // TODO load from NV RAM with target temperature
    targetTemperature = 21.0; // TODO load from NV RAM
    Serial.println("no stored data");
  }
  else {
      Serial.println("loading stored data");
      Serial.print("    mode: ");
      Serial.println(stored.mode);
      Serial.print("    target temperature: ");
      Serial.println(stored.targetTemperature);
      mode = stored.mode;
      targetTemperature = stored.targetTemperature;
  }

  transientValueChanged = 0;
  transientMode = mode;
  transientTargetTemperature = targetTemperature;

  callingForHeat = computeTargetOperatingState();

  // Set up any displays
  // we do this first so that a user can see something
  // while the rest fo the setup is happening.
  // This is the opposite of the loop behaviour where we
  // update the display very last.
  sensor->setupSensor();
  display->displaySetup(this);
  // TODO ASSERTION to make sure we don't put heating and cooling on at the same time
  if (heatingRelay != NULL) {
    heatingRelay->setupRelay();
    heatingRelay->closeRelay(callingForHeat);
  }
  // TODO handling Fan and Cooling cases
}

system_tick_t nextUpdate = millis() + 10000;

void Thermostat::loop() {
  // get sensor readings
  SensorReading * readings = sensor->getReadings();
  if (!readings[0].error) {
    temperature = readings[0].value;
    humidity = readings[1].value;
  }

  bool update_storage = FALSE;
  // transient variables are older than 5 seconds then commit them
  if (transientValueChanged) {
      if (millis()- transientValueChanged > 5000) {
        targetTemperature = transientTargetTemperature;
        mode = transientMode;
        transientValueChanged = 0;
        update_storage = TRUE;
      }
  }

  if (mode == HEAT_ON) {
    // TODO need refactoring to support proper bitfield return of operating
    // state
    // TODO handling Fan and Cooling cases
    int computed_state = computeTargetOperatingState();
    callingForHeat = computed_state;
    // update the relay
    if (heatingRelay != NULL) {
      heatingRelay->closeRelay(callingForHeat);
    }
  }
  else { // MODE = ALL_OFF
    if (heatingRelay != NULL) {
      // make sure the relay is closed
      heatingRelay->closeRelay(FALSE);
    }
  }
  // lastly update any displays
  display->drawDisplay(this);

  if (update_storage) {
    ThermostatPersistV1 store;
    store.mode = mode;
    store.targetTemperature = targetTemperature;
    Serial.println("storing stored data");
    Serial.print("    mode: ");
    Serial.println(store.mode);
    Serial.print("    target temperature: ");
    Serial.println(store.targetTemperature);
    EEPROM.put(0, store);
  }
}

// TODO change return value to include bits for clling for heart/cool/fan
int Thermostat::computeTargetOperatingState() {
  if (targetTemperature == NAN) {
    // don't have a valid temperature
    if (DEBUG) Serial.println("\nAssertion Failure: Don't have a valid target temperature");
    return FALSE;
  }
  
  // On/Off Range
  // We don’t want the fireplace turning on and off all the time so we let the
  // temperature go a little over before we turn it off and a little under
  // before we turn it on again. +/-0.5ºC is the minimum granularity based on
  // the sensitivity of the sensor we are using so we’ll use that.
  if (temperature >= (targetTemperature + 0.5)) {
    return FALSE;
  }
  else if (temperature <= (targetTemperature - 0.5) ) {
    return TRUE;
  }
  else {
    // leave it at whatever the current state is
    return callingForHeat;
  }
}

//
// step the targetTemperature up or down
// either 0.5 degrees C or 1 degree F
// by the specified number of steps
// if called from a push button this would be 1
// if from a dial or slider then this could be many
//
double Thermostat::calculateTemperatureStep(int steps) {
  return max(min(targetTemperature + steps * 0.5, TEMP_MAX), TEMP_MIN);
}

// returns the calculated steped temperatue which might not be the input because of max and min contraints
double Thermostat::stepTargetTemperature(int steps) {
  // TODO HANDLE FARENHEIGHT step by 1 degree instead of 0.5
  return setTargetTemperature(calculateTemperatureStep(steps));
}

// returns the actual set temperatue which might not be the input because of max and min contraints
double Thermostat::setTargetTemperature(double new_target) {
  transientTargetTemperature = max(min(new_target, TEMP_MAX), TEMP_MIN);
  transientValueChanged = millis();
  return transientTargetTemperature;
}

void Thermostat::setMode(Mode new_mode) {
  // todo - add commit timeout
  transientMode = new_mode;
  transientValueChanged = millis();
}

void Thermostat::cycleMode() {
  // todo - add commit timeout
  if (transientMode == ALL_OFF) {
    transientMode = HEAT_ON;
  }
  else {
    transientMode = ALL_OFF;
  }
}

//
// Concrete Instances
//
struct GFXBoundingBox {
  int16_t x1, y1;
  uint16_t width, height;
};

AdafruitOLEDDisplay::AdafruitOLEDDisplay() : Adafruit_SSD1306(128, 32) {
  types.sensorCount = 0;
  types.hasDisplay = TRUE;
  types.hasControllers = TRUE;
};

void AdafruitOLEDDisplay::displaySetup(Thermostat* thermostat) {
	// 128x32 = I2C addr 0x3C. Method inherited from Adafruit_SSD1306.
	begin(SSD1306_SWITCHCAPVCC, 0x3C);

  clearDisplay();
  setTextSize(1);
  setTextColor(WHITE);
  setRotation(2); // Rotate to flip
  setCursor(15, 24);
	println("Festeworks");
	display();
}

// For the purposed of formatting the output;
// extract integer and fractional parts of a double as integers
// return the integer part and put the integer representation of the first decimal place
// as an integer in fracValue
int AdafruitOLEDDisplay::splitTemperature(double value, int * fracValue) {
  double dInt = 0;
  *fracValue = (int) (modf(value, &dInt) * 10); // no overflow since 0 < intFrac < 10  
  // assume no overflow issues since temperatures well within int bounds
  return (int) dInt;
}

void AdafruitOLEDDisplay::drawDisplay(Thermostat * thermostat) {
  // only update the display if temperature, transient target temperature
  // or transient mode has changed
  if (thermostat->temperature == lastTempDisplayed
    && thermostat->transientTargetTemperature == lastTransientTargetTempDisplayed
    && thermostat->transientMode == lastTransientModeDisplayed) {
    return;
  }

  //Serial.println("Display update");

  char tempIntBuffer[10];
  //char tempFractionBuffer[10];
  char humidityBuffer[10];

  clearDisplay();
	setTextColor(WHITE);

  //
  // Current temperature
  //
  int tempFrac,
      tempInt = splitTemperature(thermostat->temperature, &tempFrac);

  sprintf(tempIntBuffer, "%d", tempInt);
  strcat(tempIntBuffer, ".");

  setFont(&FreeSans18pt7b);
  setCursor(0, 30);
  print(tempIntBuffer);
  setFont(&FreeSans12pt7b);
  //print(tempFractionBuffer);
  print(tempFrac);

  //
  // Humidity
  //
  setFont();
  print(" ");
  sprintf(humidityBuffer, "%2.0f%%", thermostat->humidity);
  print(humidityBuffer);

  if (thermostat->transientMode == HEAT_ON) {
    //
    // HEAT_ON
    //
    char targetIntBuffer[10];
    //char targetFractionBuffer[10];
    int targetInt, targetFrac;

    setFont();
    setCursor(85, 0);
    print("Heat to");

    targetInt = splitTemperature(thermostat->transientTargetTemperature, &targetFrac);
    sprintf(targetIntBuffer, "%d", targetInt);
    strcat(targetIntBuffer, ".");

    setFont(&FreeSans9pt7b);
    GFXBoundingBox ttBounds;
    getTextBounds(targetIntBuffer, 0, 24, &ttBounds.x1, &ttBounds.y1,
                          &ttBounds.width, &ttBounds.height);
    setCursor(127 - ttBounds.width - 10, 24); // remove an addition 6 for single char in default font
    print(targetIntBuffer);
    setFont();
    print(targetFrac);
  }
  else {
    //
    // MODE: ALL_Off
    //
    setFont(&FreeSans9pt7b);
    GFXBoundingBox offBounds;
    getTextBounds("Off", 0, 32, &offBounds.x1, &offBounds.y1,
                          &offBounds.width, &offBounds.height);
    setCursor(127 - offBounds.width, 12);
    print("Off");
  }

  lastTempDisplayed = thermostat->temperature;
  lastTransientTargetTempDisplayed = thermostat->transientTargetTemperature;
  lastTransientModeDisplayed = thermostat->transientMode;

	display();
}

NonLatchingRelay::NonLatchingRelay(pin_t latch_pin) {
  pin = latch_pin;
}

void NonLatchingRelay::setupRelay() {
  // for a latching relay we have a pin to to specifc change/
  // to each specific state
  pinMode(pin, OUTPUT);

  digitalWrite(pin, LOW);
  //Serial.print("relay pin: ");
  //Serial.print(pin);
}

void NonLatchingRelay::closeRelay(bool closed) {
  if (isClosed == closed) {
    // we are already in the target state so exit
    return;
  }

  isClosed = closed;
  if (isClosed) {
    digitalWrite(pin, HIGH);
    //Serial.println("setting relay pin high");
    //Serial.print("   pin: ");
    //Serial.println(pin);
  }
  else {
    digitalWrite(pin, LOW);
    //Serial.println("setting relay pin low");
  }
}

DHTSensor::DHTSensor(pin_t pin, uint8_t type) {
  types.sensorCount = 2;
  sampleStarted = FALSE;
  dht = PietteTech_DHT(pin, type);

  readings[0].name = "Temperature";
  readings[0].sensorId = 0;
  readings[0].type = SensorReading::TEMPERATURE;
  readings[0].value = -99;

  readings[1].name = "Humidity";
  readings[1].sensorId = 1;
  readings[1].type = SensorReading::HUMIDITY;
  readings[1].value = -99;
}

void DHTSensor::setupSensor() {
  // call begin from DHT base class
  dht.begin();
}

SensorReading * DHTSensor::getReadings() {
  if (millis() > nextReading) {
    // we are not currently reading the sensor
    // and it is past time to start another reading
    // so start a new reading if we are not in the middle of one

    if (!sampleStarted) {
      // we are not currently reading the sensor
      // and it is past time to start another reading
      // so start a new reading
      //Serial.println("starting to aquire");
      dht.acquire();
      sampleStarted = TRUE;
      read = FALSE;
    }
    //else {
    //  Serial.println("new sample already started");
    //}

    if (!dht.acquiring()) {
      int status = dht.getStatus();

      if (status == DHTLIB_OK) {
        readings[0].value = dht.getCelsius();
        readings[1].value = dht.getHumidity();
        readings[0].error = FALSE;
        readings[1].error = FALSE;
        //Serial.println(readings[0].name);
        //Serial.print(" ");
        //Serial.println(readings[0].value);

        //Serial.println(readings[1].name);
        //Serial.print(" ");
        //Serial.println(readings[1].value);
      }
      else {
        Serial.println("ERROR reading sensor");

        Serial.print("Read sensor: ");
        switch (status) {
          case DHTLIB_ERROR_CHECKSUM:
            Serial.println("Error\n\r\tChecksum error");
            break;
          case DHTLIB_ERROR_ISR_TIMEOUT:
            Serial.println("Error\n\r\tISR time out error");
            break;
          case DHTLIB_ERROR_RESPONSE_TIMEOUT:
            Serial.println("Error\n\r\tResponse time out error");
            break;
          case DHTLIB_ERROR_DATA_TIMEOUT:
            Serial.println("Error\n\r\tData time out error");
            break;
          case DHTLIB_ERROR_ACQUIRING:
            Serial.println("Error\n\r\tAcquiring");
            break;
          case DHTLIB_ERROR_DELTA:
            Serial.println("Error\n\r\tDelta time to small");
            break;
          case DHTLIB_ERROR_NOTSTARTED:
            Serial.println("Error\n\r\tNot started");
            break;
          default:
            Serial.println("Unknown error");
            break;
        }

        readings[0].error = TRUE;
        readings[0].value = -99;
        readings[1].error = TRUE;
        readings[1].value = -99;
      }
      sampleStarted = FALSE;
      nextReading = millis() + 5000;
    }
  }

  return readings;
}


// namespace close
}
}