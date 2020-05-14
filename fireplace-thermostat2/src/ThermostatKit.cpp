
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
  targetTemperature = 20.0;
  changingTargetTemperature = FALSE;

  this->sensor = sensor;
  this->display = display;
  heatingRelay = heating_relay;
  coolingRelay = cooling_relay;
  fanRelay = fan_relay;  
}

void Thermostat::setup() {
  // READ Target Temp from EEPROM
  callingForHeat = FALSE;
  mode = ALL_OFF;

  // Set up any displays
  // we do this first so that a user can see something
  // while the rest fo the setup is happening.
  // This is the opposite of the loop behaviour where we
  // update the display very last.
  Serial.println("D1");
  sensor->setupSensor();
  Serial.println("D2");
  display->displaySetup(this);
  Serial.println("D4");
  // TODO ASSERTION to make sure we don't put heating and cooling on at the same time
  if (heatingRelay != NULL) {
    // TODO need proper logic in here for non-heating use cases
    heatingRelay->setupRelay();
    heatingRelay->closeRelay(callingForHeat);
  }
  // TODO handling Fan and Cooling cases
}

system_tick_t nextUpdate = millis() + 10000;

void Thermostat::loop() {
  // check for controller inputs (POT rotation, button presses)
  display->controllerLoop(this);

  // get sensor readings
  SensorReading * readings = sensor->getReadings();
  if (!readings[0].error) {
    temperature = readings[0].value;
    humidity = readings[1].value;
    error = FALSE;
  }
  else {
    Serial.println("error reading temp");
    return;
  }

  // TODO need refactoring to support proper bitfield return of operating state
  // TODO handling Fan and Cooling cases
  int computed_state = computeTargetOperatingState();
  if (computed_state != callingForHeat) {
    callingForHeat = computed_state;
    // update the relay
    if (heatingRelay != NULL) {
      heatingRelay->closeRelay(callingForHeat);
    }
    if (callingForHeat) {
      mode = HEAT_ON;
    }
    else {
      mode = ALL_OFF;        
    }
    
    Serial.println("changing relay state");
  }

  // lastly update any displays
  display->drawDisplay(this);

}

// TODO change return value to include bits for clling for heart/cool/fan
int Thermostat::computeTargetOperatingState() {
  if (targetTemperature == NAN) {
    // don't have a valid temperature
    if (DEBUG) Serial.println("\nAssertion Failure: Don't have a valid target temperature");
    return FALSE;
  }
  
  //if (DEBUG) {
  //   char buffer [1024];
  //   sprintf(buffer, "computeOS: temperature = %f, targetTemp: %f", temperature, targetTemperature);
  //  Serial.println(buffer);
  //}

  // On/Off Range
  // We don’t want the fireplace turning on and off all the time so we let the
  // temperature go a little over before we turn it off and a little under
  // before we turn it on again. +/-0.5ºC is the minimum granularity based on
  // the sensitivity of the sensor we are using so we’ll use that.
  if (temperature >= (targetTemperature + 0.5)) {
    //if (DEBUG) Serial.println("debug: computing OperatingState to Off");
    return FALSE;
  }
  else if (temperature <= (targetTemperature - 0.5) ) {
    //if (DEBUG) Serial.println("debug: computing OperatingState to On");
    return TRUE;
  }
  else {
    // leave it at whatever the current state is
    //if (DEBUG) Serial.println("debug: leaving the operating state the way it is");
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

double Thermostat::stepTargetTemperature(int steps) {
  // TODO HANDLE FARENHEIGHT step by 1 degree instead of 0.5
  return setTargetTemperature(calculateTemperatureStep(steps));
}

double Thermostat::setTargetTemperature(double new_target) {
  targetTemperature = max(min(new_target, TEMP_MAX), TEMP_MIN);
  return targetTemperature;
}

//
// Concrete Instances
//

struct GFXBoundingBox {
  int16_t x1, y1;
  uint16_t width, height;
};

AdafruitOLEDDisplay::AdafruitOLEDDisplay() {
  types.sensorCount = 0;
  types.hasDisplay = TRUE;
  types.hasControllers = TRUE;
};

void AdafruitOLEDDisplay::controllerLoop(Thermostat* thermostat) {
  // call loop from the OledWingAdafruit class to debounce its buttons
  loop();
}

void AdafruitOLEDDisplay::displaySetup(Thermostat* thermostat) {
  setup();
  clearDisplay();
  setTextSize(1);
  setTextColor(WHITE);
  setCursor(15, 24);
  setRotation(2); // Rotate to flip
	println("Festeworks");
	display();
}

void AdafruitOLEDDisplay::drawDisplay(Thermostat * thermostat) {
  char tempIntBuffer[10];
  char tempFractionBuffer[10];
  char humidityBuffer[10];
  char targetIntBuffer[10];
  char targetFractionBuffer[10];

  clearDisplay();
	setTextColor(WHITE);

  // seperate temperature integer and decimal components in order to draw the
  // decimal in a smaller font.
  sprintf(tempIntBuffer, "%2.0f.", thermostat->temperature);
  int intValue = (int) thermostat->temperature;  // lose the decimal
  sprintf(tempFractionBuffer, "%d",
        (int)((thermostat->temperature - (double)intValue) * 10.0));
 
  sprintf(humidityBuffer, "%2.0f%%", thermostat->humidity);

  sprintf(targetIntBuffer, "%2.0f.", thermostat->targetTemperature);
  intValue = (int) thermostat->targetTemperature;  // lose the decimal
  sprintf(targetFractionBuffer, "%d",
        (int)((thermostat->targetTemperature - (double)intValue) * 10.0));

  setFont(&FreeSans18pt7b);
  setCursor(0, 29);
  print(tempIntBuffer);
  setFont(&FreeSans12pt7b);
  print(tempFractionBuffer);

  setFont();
  print("  ");
  print(humidityBuffer);

  setFont();
  setCursor(85, 0);
  print("Heat to");

  setFont(&FreeSans9pt7b);
  GFXBoundingBox ttBounds;
  getTextBounds(targetIntBuffer, 0, 32, &ttBounds.x1, &ttBounds.y1,
                          &ttBounds.width, &ttBounds.height);
  setCursor(127 - ttBounds.width - 8, 29); // remove an addition 6 for single char in default font
  print(targetIntBuffer);
  setFont();
  print(targetFractionBuffer);

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
  Serial.print("relay pin: ");
  Serial.print(pin);
}

void NonLatchingRelay::closeRelay(bool closed) {
  if (isClosed == closed) {
    // we are already in the target state so exit
    return;
  }

  isClosed = closed;
  if (isClosed) {
    digitalWrite(pin, HIGH);
    Serial.println("setting relay pin high");
    Serial.print("   pin: ");
    Serial.println(pin);
  }
  else {
   digitalWrite(pin, LOW);
    Serial.println("setting relay pin low");
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
      Serial.println("starting to aquire");
      dht.acquire();
      sampleStarted = TRUE;
      read = FALSE;
    }
    else {
      Serial.println("new sample already started");
    }

    if (!dht.acquiring()) {
      int status = dht.getStatus();

      if (status == DHTLIB_OK) {
        readings[0].value = dht.getCelsius();
        readings[1].value = dht.getHumidity();
        readings[0].error = FALSE;
        readings[1].error = FALSE;
        Serial.println(readings[0].name);
        Serial.print(" ");
        Serial.println(readings[0].value);

        Serial.println(readings[1].name);
        Serial.print(" ");
        Serial.println(readings[1].value);
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
/**
  else {
      Serial.println(millis());
      Serial.print("    next: ");
      Serial.println(nextReading);
  }
**/

  return readings;
}


// namespace close
}
}