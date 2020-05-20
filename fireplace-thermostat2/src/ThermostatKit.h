#ifndef CRANBROOKE_THERMOSTATKIT__H
#define CRANBROOKE_THERMOSTATKIT__H

#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Debounce.h"
#include <FreeSans18pt7b.h>
#include <FreeSans12pt7b.h>
#include <FreeSans9pt7b.h>

#include <PietteTech_DHT.h>

#include <Encoder.h>

namespace Cranbrooke {
namespace ThermostatKit {

enum DisplayState {
  CURRENT_TEMPERATURE = 0,
  TARGET_TEMPERATURE = 1,
  CURRENT_HUMIDITY = 2,
  ERROR = 3
};

enum Mode {
  ALL_OFF = 0,
  HEAT_ON = 1
//  COOLING_ON = 2,
//  FAN_ON = 3,
//  FAN_CIRCULATE = 4
};

class Component; // forward reference
class DHTSensor; // forward reference
class AdafruitOLEDDisplay; // forward reference

class Relay {
  public:
    virtual void setupRelay();
    virtual void closeRelay(bool closed);
};

class Thermostat {

 public:
  Thermostat(DHTSensor * sensor, AdafruitOLEDDisplay * display,
        Relay * heating_relay, Relay * cooling_relay, Relay * fan_relay);

  static constexpr double TEMP_MAX = 30.0f;
  static constexpr double TEMP_MIN = 5.0f;

  // delay before transitory mode or target temperature settings take effect
  static constexpr int CHANGE_DELAY = 5000; // ms

  enum Mode mode;

  enum DisplayState displayState = CURRENT_TEMPERATURE;

  void setup();
  void loop();

  double stepTargetTemperature(int steps);
  double calculateTemperatureStep(int steps);
  double setTargetTemperature(double newTarget);
  void cycleMode();
  void setMode(Mode new_mode);

  // we need the heat to be on to reach the target temperature
  bool callingForHeat;
  // we need the cooling to be on to reach the target temperature
  bool callingForCooling;
  // current temperature at the thermostat
  double temperature;
  // current humidity at the thermostat
  double humidity;

  // temperature that we are heating or cooling to
  double targetTemperature;

  // working uncommited temperature when adjusting the target temperature
  double transientTargetTemperature;
  // working uncommited temperature when adjusting the target temperature
  Mode transientMode;
  // keep track if changes are in progress and when they began
  unsigned long transientValueChanged = 0;

 private:
  int computeTargetOperatingState();
  DHTSensor * sensor;
  Component * display;
  Relay * heatingRelay;
  Relay * coolingRelay;
  Relay * fanRelay;

};

class SensorReading {
  public: 
    int sensorId; // internal id of the sensor within the component
    // TODO add support for name in constructor
    String name; // human readable name of sensor
    enum Type {
      TEMPERATURE = 0,
      HUMIDITY = 1,
      PRESSURE = 2,
      WATER = 3,
      LUMINOSITY = 4,
      PARTICULATES = 5
    } type;
    String typeName[6] = {
      "Temperature",
      "Humidity",
      "Pressure",
      "Water",
      "Luminosity",
      "Particulates"
    }; 
    double value = 0.0;
    boolean error = TRUE;
};

class Component {
  public:
    // Use bits fields instead of inheritance to define bevaviour for efficiency
    struct Types {
      int sensorCount : 4, hasControllers : 1, hasDisplay : 1;
    } types;

    virtual void sensorSetup();
    virtual SensorReading * getReadings(); // return a list of sensors and their most current readings

    virtual void controllerSetup(Thermostat * thermostat);
    virtual void controllerLoop(Thermostat * thermostat);

    virtual void setupRelay(); // TODO do we need thermo passed in?
    virtual void setRelay(bool closed); // called from setup or loop

    virtual void displaySetup(Thermostat * thermostat);
    virtual void drawDisplay(Thermostat * thermostat);
};

class AdafruitOLEDDisplay : private Adafruit_SSD1306, public Component {

  public:
    AdafruitOLEDDisplay();

    virtual void displaySetup(Thermostat* thermostat);
    virtual void drawDisplay(Thermostat* thermostat);
 
  private:
    static int splitTemperature(double value, int * fracValue);
    double lastTempDisplayed = -99;
    double lastTransientTargetTempDisplayed = -99;
    double lastTransientModeDisplayed = -99;
};

class NonLatchingRelay : public Relay {
  public:
    NonLatchingRelay(pin_t pin);
    virtual void setupRelay();
    virtual void closeRelay(bool closed);

  private:
    pin_t pin;
    bool isClosed = FALSE;
};

class DHTSensor : public Component {
  public:
    DHTSensor(pin_t pin, uint8_t type);
    void setupSensor();
    SensorReading* getReadings();

  private:
    PietteTech_DHT dht;
    SensorReading readings[2];
    bool read = FALSE; // past tense of read, "red"
    unsigned int nextReading = 0;
    bool sampleStarted;
    
};


}
}

#endif