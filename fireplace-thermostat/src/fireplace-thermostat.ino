/*
 * Project fireplace-thermostat
 * Description:
 * Author:
 * Date:
 */

#include <Adafruit_DHT.h>

#define DHT_PIN D3

#define LED_PIN D0

#define POT_PIN A4

#define THERMOSTAT_ON_PIN D4
#define THERMOSTAT_OFF_PIN D5

#define DHTTYPE DHT22 

enum OperatingState {
  Off = 0,    // Relay is open; fireplace off
  On = 1,     // Relay is closed; fireplace on
  Unknown = 2,
  OperatingError = -1  // Error condition; relay closed (blink LED rapidly)
};

OperatingState g_operatingState = Unknown;

enum ControlState {
  // On/Off state controlled by temerature and dial settings; LED ON
  Local = 0,

  // On/Off state relative to a remotely set temperature
  // (dial is ignored); pulse LED
  RemoteTemperature = 1,

  // On/Off state directly controlled by remote setting; pulse LED
  RemoteControl = 2
};

ControlState g_controlState = Local;

#define ERROR_READING_HUMIDITY 1
#define ERROR_READING_TEMPERATURE 2
#define ERROR_ILLEGAL_COMMAND 4

// LED
#define LED_ON 1
#define LED_OFF 0

DHT DHT(DHT_PIN, DHTTYPE);
double g_humidity = NAN;     // Particle variable
double g_temperature = NAN;  // Particle variable

double g_dialTemp = NAN;
double g_targetTemp = NAN;
time_t g_lastDialChange = -1;

double g_rotation = 0;

// variables for controlling the pulse led effect through the loops
int g_ledPulseValue = 255;
int g_ledPulseIncrement = 1;
//
// Run the LED value up and down between 0 and 255
//
void pulse() {
  analogWrite(LED_PIN, 255);
  // analogWrite(LED_PIN, (float) g_ledPulseValue);
  if (g_ledPulseIncrement == 1 && g_ledPulseValue == 255) {
    // start going back down
    g_ledPulseIncrement = -1;
  } else if (g_ledPulseIncrement == -1 && g_ledPulseValue == 0) {
    g_ledPulseIncrement = 1;
  }
  g_ledPulseValue += g_ledPulseIncrement;
}

OperatingState computeTargetOperatingState() {
  if (g_controlState == RemoteControl) {
    // return check remote on/off state
    // TODO    
  }

  if (g_targetTemp == NAN) {
    // don't have a valid temperature
    return OperatingError;
  }
  
  // On/Off Range
  // We don’t want the fireplace turning on and off all the time so we let the
  // temperature go a little over before we turn it off and a little under
  // before we turn it on again. +/-0.5ºC is the minimum granularity based on
  // the sensitivity of the sensor we are using so we’ll use that.
  if (g_temperature >= (g_targetTemp + 0.5))
    return Off;
  else if (g_temperature <= (g_targetTemp - 0.5) || (g_operatingState == OperatingError))
    return On;
  else
    // leave it at whatever the current state is
    return g_operatingState;
}  

void setLatchingOperatingState(OperatingState newState) {
  int pin;
  switch (newState) {
  case OperatingError:
    // don't need to do anything else here since we will read and act on the globable errors state latter
    // an ideal spot for an assertion since this shouldn't every happen
    Serial.println("AssertionFailure: passing in operating error");
    return;    
  case On:
    pin = THERMOSTAT_ON_PIN;
    break;
  case Off:
    pin = THERMOSTAT_OFF_PIN;
    break;
  }

  // pluse the appropriate pin to toggle the latching relay
  digitalWrite(pin, HIGH);
  delay(12);
  digitalWrite(pin, LOW);
}

// setup() runs once, when the device is first turned on.
// Put initialization like pinMode and begin functions here.
void setup() {

  // OUTPUT to USB
  pinMode(LED_PIN, OUTPUT);
  delay(2000);
  Serial.begin(9600);            // open the serial port at 9600 bps:
  Serial.println("\nSetup...");  // prints another carriage return

  Particle.variable("temperature", &g_temperature, DOUBLE);
  Particle.variable("humidity", &g_humidity, DOUBLE);
  DHT.begin();

  // TODO - remove publishing these after eveything is debugged
  Particle.variable("rotation", &g_rotation, DOUBLE);
  Particle.variable("dialTemp", &g_dialTemp, DOUBLE);

  // Particle.variable("state", &g_state, INT);

  // TODO need to publish remoteoverride and add state
  // Particle.function("publish", publishCommand);

  pinMode(POT_PIN, INPUT);

  pinMode(THERMOSTAT_ON_PIN, OUTPUT);
  digitalWrite(THERMOSTAT_ON_PIN, LOW);

  // initialize latching relay
  pinMode(THERMOSTAT_OFF_PIN, OUTPUT);
  digitalWrite(THERMOSTAT_OFF_PIN, HIGH);
  delay(12);
  digitalWrite(THERMOSTAT_OFF_PIN, LOW);
  g_operatingState = Off;
}

// TODO Blink LED on ERROR

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  int failure = 0;
  
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
  float humidity = DHT.getHumidity();
  // shouldn't need this since the library prevents extra reads
  // delay(50);
  float temperature = DHT.getTempCelcius();

  //
  // measure the rotation of the potentiometer to determine the target
  // temperature
  //
  float rotation = analogRead(POT_PIN);

  // Mapping the Potentiometer reading to a temperature between
  // Given a desired dial range of 6ºC and 30ºC with .5 increments
  // We round to the nearest 0.5 ºC to match the granularity of the sensor
  // 4100 Max Reading of the potentiometer
  // 25º (between 6º and 30º) but only 49 stops [6, 6.5, 7, 7.5...28.5, 29.5,
  // 30] 4100 / 49 -> Interval of 164 or 82 per half degree
  double unrounded_dial_temp = rotation / 83;
  // unrounded_dial_temp is (about) twice what it should be since we divided by
  // the steps instead of but we want to round it before we divide by 2 to get
  // it to line up on even .5 degree increments add 6 to base the range at a
  // minimum f 6ºC.
  double dial_temp = round(unrounded_dial_temp) / 2 + 6;
  time_t now = millis();
  // map the potentionmeter to a dial temperature

  // if the dial temperture has not changed in a second then update the "target
  // temperature"
  if (g_dialTemp == NAN || (now - g_lastDialChange) > 1000) {
    // dial hasn't moved in more than a second so reset the targetTemperature
    g_targetTemp = dial_temp;
  } else if (dial_temp != g_dialTemp) {
    // dial has moved since the last loop so reset timer
    g_lastDialChange = now;
  }

  g_dialTemp = dial_temp;

  // TODO wrap with error state
  //pulse();

  if (temperature == NAN) {
    Serial.println("FAILED to read Temperature");
    failure |= ERROR_READING_TEMPERATURE;
  }
  if (humidity == NAN) {
    Serial.println("FAILED to read Humidity");
    failure |= ERROR_READING_HUMIDITY;
  }

  // update global (particle) variables
  g_temperature = (double)temperature;
  g_humidity = (double)humidity;

  // TODO remove once we have debugged
  g_rotation = (double)rotation;
  g_dialTemp = dial_temp;

  OperatingState newOperatingState = computeTargetOperatingState();
  if (newOperatingState != g_operatingState) {
    setLatchingOperatingState(newOperatingState);
  }


}
