#ifndef CRANBROOKE_THERMOSTATKIT__H
#define CRANBROOKE_THERMOSTATKIT__H

#include <oled-wing-adafruit.h>
#include <FreeSans18pt7b.h>
#include <FreeSans12pt7b.h>
#include <FreeSans9pt7b.h>
#include <Adafruit_DHT_Particle.h>
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
  HEAT_ON = 1,
  COOLING_ON = 2,
  FAN_ON = 3,
  FAN_CIRCULATE = 4
};

class Component; // forward reference

class Thermostat {
 public:
  Thermostat(Component ** components);

  static constexpr double TEMP_MAX = 30.0f;
  static constexpr double TEMP_MIN = 5.0f;

  enum Mode mode;

  enum DisplayState displayState;

  void setup();
  void loop();

  double stepTargetTemperature(int steps);

  // we need the heat to be on to reach the traget temperature
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
  double transientTemperature;
  // a control is in the middle of an uncommitted change to the target temperature
  bool changingTargetTemperature;

 private:
  int computeTargetOperatingState();
  Component ** components;


 
};

class SensorReading {
  public: 
    int sensorId; // internal id of the sensor within the component
    enum Type {
      TEMPERATURE = 0,
      HUMIDITY = 1,
      PRESSURE = 2,
      WATER = 3,
      LUMINOSITY = 4,
      PARTICULATES = 5
    } type;
    double value;
};

class Component {
  public:
    // Use bits fields instead of inheritance to define bevaviour for efficiency
    struct Type {
      bool hasSensors : 1, isController : 1, isRelay : 1, isDisplay : 1;
    } type;

    virtual void sensorSetup();
    virtual SensorReading * getReadings(); // return a list of sensors and their most current readings

    virtual void controllerSetup(Thermostat * thermostat);
    virtual void controllerLoop(Thermostat * thermostat);

    virtual void setupRelay(); // TODO do we need thermo passed in?
    virtual void setRelay(bool closed); // called from setup or loop

    virtual void displaySetup(Thermostat * thermostat);
    virtual void drawDisplay(Thermostat * thermostat);
};

class AdafruitOLEDDisplay : private OledWingAdafruit, public Component {

  public:
    AdafruitOLEDDisplay();

    virtual void controllerLoop(Thermostat* thermostat);

    virtual void displaySetup(Thermostat* thermostat);
    virtual void drawDisplay(Thermostat* thermostat);
};

class RotaryController : private Encoder, public Component {
  public:
    RotaryController(pin_t pin1, pin_t pin2);
    virtual void controllerSetup(Thermostat* thermostat);
    virtual void controllerLoop(Thermostat* thermostat);
    long referencePosition;
    time_t lastMod;
    double currentPosition;

  private:
    double computeTargetTemperature(Thermostat * thermostat);
};

class LatchingRelay : public Component {
  public:
    LatchingRelay(pin_t on_pin, pin_t off_pin, long relay_latch_pulse_time);
    virtual void setupRelay();
    virtual void setRelay(bool closed);

  private:
    pin_t onPin, offPin;
    bool isClosed;
    long relayLatchPulseTime;
};

class DHTSensor : private DHT, public Component {
  public:
    DHTSensor(pin_t pin, uint8_t type);
    void sensorSetup();
    SensorReading* getReadings();

  private:
    SensorReading readings[2];
    time_t lastReading;
    
};

}
}

#endif