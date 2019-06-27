
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

Thermostat::Thermostat(Component ** components, Relay * heating_relay,
                                                Relay * cooling_relay, Relay * fan_relay) {
  callingForHeat = FALSE;
  callingForCooling = FALSE;
  temperature = 0;
  humidity = 0;
  // TODO double pressure = 0;
  targetTemperature = 20.0;
  changingTargetTemperature = FALSE;

  this->components = components;
  heatingRelay = heating_relay;
  coolingRelay = cooling_relay;
  fanRelay = fan_relay;  
}

void Thermostat::setup() {
  // READ Target Temp from EEPROM
  callingForHeat = FALSE;
  mode = HEAT_ON;

  // Set up any displays
  // we do this first so that a user can see something
  // while the rest fo the setup is happening.
  // This is the opposite of the loop behaviour where we
  // update the display very last.
  for (Component *component = components[0]; component != NULL; component++) {
      if (component->type.isDisplay) {
        // TODO this needs to be more complicated for cooling vs heat case
        component->displaySetup(this);
      }
  }
 
  // Set up sensors
  for (Component *component = components[0]; component != NULL; component++) {
      if (component->type.hasSensors) {
        component->sensorSetup();
      }
  }

  // set up controllers
  for (Component *component = components[0]; component != NULL; component++) {
      if (component->type.isController) {
        component->controllerSetup(this);
      }
  }

  // TODO ASSERTION to make sure we don't put heating and cooling on at the same time
  if (heatingRelay != NULL) {
    // TODO need proper logic in here for non-heating use cases
    heatingRelay->setRelay(callingForHeat);
  }
  // TODO handling Fan and Cooling cases
}

void Thermostat::loop() {
  // get sensor readings
  for (Component *component = components[0]; component != NULL; component++) {
      if (component->type.hasSensors) {
        //READ sensor
        // component->getReadings();
      }
  }

  // TODO ASSERTION to make sure we don't put heating and cooling on at the same time
  // allow conterollers to update
  for (Component *component = components[0]; component != NULL; component++) {
      if (component->type.isController) {
        component->controllerLoop(this);
      }
  }

  // update relays (call for cold of heat)
  if (heatingRelay != NULL) {
    // TODO need proper logic in here for non-heating use cases
    heatingRelay->setRelay(callingForHeat);
  }
  // TODO handling Fan and Cooling cases

  // lastly update any displays
  for (Component *component = components[0]; component != NULL; component++) {
      if (component->type.isDisplay) {
        // TODO this needs to be more complicated for cooling vs heat case
        component->drawDisplay(this);
      }
  }

}

int Thermostat::computeTargetOperatingState() {
  if (targetTemperature == NAN) {
    // don't have a valid temperature
    if (DEBUG) Serial.println("\nAssertion Failure: Don't have a valid target temperature");
    return FALSE;
  }
  
  if (DEBUG) {
     char buffer [1024];
     sprintf(buffer, "computeOS: temperature = %f, targetTemp: %f", temperature, targetTemperature);
    Serial.println(buffer);
  }

  // On/Off Range
  // We don’t want the fireplace turning on and off all the time so we let the
  // temperature go a little over before we turn it off and a little under
  // before we turn it on again. +/-0.5ºC is the minimum granularity based on
  // the sensitivity of the sensor we are using so we’ll use that.
  if (temperature >= (targetTemperature + 0.5)) {
    if (DEBUG) Serial.println("debug: computing OperatingState to Off");
    return FALSE;
  }
  else if (temperature <= (targetTemperature - 0.5) ) {
    if (DEBUG) Serial.println("debug: computing OperatingState to On");
    return TRUE;
  }
  else {
    // leave it at whatever the current state is
    if (DEBUG) Serial.println("debug: leaving the operating state the way it is");
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
double Thermostat::stepTargetTemperature(int steps) {
  // TODO HANDLE FARENHEIGHT step by 1 degree instead of 0.5
  return max(min(targetTemperature + steps * 0.5, TEMP_MAX), TEMP_MIN);
}

//
// Concrete Instances
//

struct GFXBoundingBox {
  int16_t x1, y1;
  uint16_t width, height;
};

AdafruitOLEDDisplay::AdafruitOLEDDisplay() {
  type.hasSensors = FALSE;
  type.isDisplay = TRUE;
  type.isController = TRUE;
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
	println("Festeworks");
	display();
}

void AdafruitOLEDDisplay::drawDisplay(Thermostat * thermostat) {
  char valueBuffer[5];
  char suffixBuffer[5];

  clearDisplay();
	setTextColor(WHITE);

  double displayValue;
  char const * displayText = NULL;

  switch (thermostat->displayState) {
  case TARGET_TEMPERATURE:
    displayValue = thermostat->transientTemperature; // TODO this call needs to be abstracted from dial
    if (thermostat->mode == HEAT_ON)
      displayText = "Heat to";
    else {
      displayText = "Cool to";
    }
    break;
  case CURRENT_HUMIDITY:
    displayValue = thermostat->humidity;
    break;
  default: // case CURRENT_TEMPERATURE:
    displayValue = thermostat->temperature;
    if (thermostat->mode == ALL_OFF)
      displayText = "Off";
    break;  
  }

  // layout current temperature
  // integer part of temperature or humidity in 24pt
  if (displayText) {
    // draw operating state
    setCursor(0, 12);
    setFont(&FreeSans9pt7b);
    setCursor(0, 31);
    print(displayText);
  }

  if (thermostat->displayState != ERROR) {
    // Draw current number
    // actually place this right justified and vertically centered on the 128x32
    // display
    // gDisplay.setCursor(127 - intBounds.width - decBounds.width, (32 -
    // intBounds.height) / 2 + intBounds.height);
    // extract the decimal so that we can draw it smaller
    sprintf(valueBuffer, "%2.0f", displayValue);
    int intValue = (int) displayValue;  // lose the decimal
    if (thermostat->displayState == TARGET_TEMPERATURE || thermostat->displayState == CURRENT_TEMPERATURE) {
      sprintf(suffixBuffer, ".%d",
          (int)((displayValue - (double)intValue) * 10.0));
    }
    else {
      // assert(gState.display == CURRENT_HUMIDITY);
      sprintf(suffixBuffer, "%%");
    }
    setFont(&FreeSans18pt7b);
    GFXBoundingBox intBounds;
    getTextBounds(valueBuffer, 0, 32, &intBounds.x1, &intBounds.y1,
                          &intBounds.width, &intBounds.height);

    setFont(&FreeSans12pt7b);
    GFXBoundingBox decBounds;
    getTextBounds(suffixBuffer, 0, 32, &decBounds.x1, &decBounds.y1,
                          &decBounds.width, &decBounds.height);

    setCursor(115 - intBounds.width - decBounds.width, 31);
    setFont(&FreeSans18pt7b);
    print(valueBuffer);
    setFont(&FreeSans12pt7b);
    print(suffixBuffer);
 
  }

	display();
}

// pass the pin1, pin2 call to the base class Encoder contructor
RotaryController::RotaryController(pin_t pin1, pin_t pin2) : Encoder(pin1, pin2) {
    referencePosition  = 0;
    lastMod = 0;
    currentPosition = 0.0;
    type.isController = TRUE;
}

void RotaryController::controllerLoop(Thermostat * thermostat) {
    // Handle Dial Rotation
  currentPosition = read();
  if (currentPosition != referencePosition) {
    // bump up or down by 0.5 for each two positions up or down
    // above logic makes sure that a partial click is ignored
    thermostat->changingTargetTemperature = TRUE;
    thermostat->displayState = TARGET_TEMPERATURE;
    lastMod = millis();
    // Serial.println(newPosition);
  }
  else if (thermostat->changingTargetTemperature && (millis() - lastMod > 3000)) {
    // dial has not been modified for more that 3 seconds
    thermostat->changingTargetTemperature = FALSE;

    // no target temperature above 30 degrees; no temperature below 5
    thermostat->targetTemperature = computeTargetTemperature(thermostat);
    thermostat->displayState = CURRENT_TEMPERATURE;
    referencePosition = currentPosition;
    //TODO write changes in targetTemp to EEPROM
  }

}

// Take a delta position and calculate a new target temperature
// based on 4 positions temperature step value
// if the temperature hits the min or max then we should reset the
// reference position so that spinning it the otherway will yield
// immediate benefit.
double RotaryController::computeTargetTemperature(Thermostat * thermostat) {
  // no target temperature above 30 degrees; no temperature below 5
  //return max(min(gTargetTemp + gtempDelta, 30.0), 5.0);
  double newTemp = thermostat->stepTargetTemperature((currentPosition - referencePosition) / 4);
  if (newTemp == Thermostat::TEMP_MAX || newTemp == Thermostat::TEMP_MIN) {
    // reset the reference position
    referencePosition = currentPosition;
  }
  return newTemp;
}

LatchingRelay::LatchingRelay(pin_t on_pin, pin_t off_pin, long relay_latch_pulse_time) {
  isClosed = FALSE;
 
  onPin = on_pin;
  offPin = off_pin;
  relayLatchPulseTime = relay_latch_pulse_time;
}

void LatchingRelay::setupRelay() {
  // for a latching relay we have a pin to to specifc change/
  // to each specific state
  pinMode(onPin, OUTPUT);
  pinMode(offPin, OUTPUT);

  digitalWrite(onPin, LOW);
  digitalWrite(offPin, LOW);
}

void LatchingRelay::setRelay(bool closed) {
  if (isClosed == closed) {
    // we are already in the target state so exit
    return;
  }

  int pin;
  if (closed) {
    pin = onPin;
  }
  else {
    pin = offPin;
  }

  // pulse the appropriate pin to toggle the latching relay
  digitalWrite(pin, HIGH);
  delay(relayLatchPulseTime);
  digitalWrite(pin, LOW);
}

DHTSensor::DHTSensor(pin_t pin, uint8_t type) : DHT(pin, type) {
  readings[0].sensorId = 1;
  readings[0].type = SensorReading::TEMPERATURE;
  readings[0].value = -99;

  readings[1].sensorId = 1;
  readings[1].type = SensorReading::HUMIDITY;
  readings[1].value = -99;
}

void DHTSensor::sensorSetup() {
  // call begin from DHT base class
  begin();
}

SensorReading* getReadings() {
  // TODO READING CODE

  return NULL;
}

// namespace close
}
}