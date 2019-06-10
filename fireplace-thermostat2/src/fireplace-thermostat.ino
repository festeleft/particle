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

#include <oled-wing-adafruit.h>
#include <FreeSans18pt7b.h>
#include <FreeSans12pt7b.h>
#include <FreeSans9pt7b.h>
#include <Adafruit_DHT_Particle.h>
#include <Encoder.h>

// D0, D1 used for serial communcation to display
// D2 Button A on display
// D3 Button B on display
// D4 Button C on display

#define TEMP_MAX 30.0
#define TEMP_MIN 5.0

#define DHT_PIN D5

#define THERMOSTAT_ON_PIN D7
#define THERMOSTAT_OFF_PIN D6

// Rotary Encoder #377B (AdaFruit)
#define DIAL_PIN_1 A3 // rotation pin 1
#define DIAL_PIN_2 A4 // rotation pin 2
#define DIAL_BUTTON_PIN A5 // dial push button

#define RELAY_LATCH_PULSE_TIME 500

#define DHTTYPE DHT11 

#define ERROR_READING_HUMIDITY 1
#define ERROR_READING_TEMPERATURE 2
#define ERROR_ILLEGAL_COMMAND 4

OledWingAdafruit gDisplay;

Encoder gDial(DIAL_PIN_1, DIAL_PIN_2);

struct GFXBoundingBox {
  int16_t x1, y1;
  uint16_t width, height;
};

int gDebug = FALSE;

enum Mode {
  HEAT_OFF = 0,
  HEAT_ON = 1
};

enum DisplayState {
  CURRENT_TEMPERATURE = 0,
  TARGET_TEMPERATURE = 1,
  CURRENT_HUMIDITY = 2,
  ERROR = 3
};

struct State {
    Mode mode;
    DisplayState display;
    int callingForHeat;
    double targetTemperature;
} gState;

long gDialReferencePosition  = 0;
time_t gDialLastMod = 0;
double gDialCurrentPosition = 0.0;
int gChangingTargetTemperature = FALSE;

DHT gDHT(DHT_PIN, DHTTYPE);
double gHumidity = NAN;     // Particle variable
double gTemperature = NAN;  // Particle variable

double gTargetTemp = 18.0; // TODO read from EEPROM on startup

int computeTargetOperatingState() {
  if (gTargetTemp == NAN) {
    // don't have a valid temperature
    if (gDebug) Serial.println("\nAssertion Failure: Don't have a valid target temperature");
    return FALSE;
  }
  
  if (gDebug) {
     char buffer [1024];
     sprintf(buffer, "computeOS: temperature = %f, targetTemp: %f", gTemperature, gTargetTemp);
    Serial.println(buffer);
  }

  // On/Off Range
  // We don’t want the fireplace turning on and off all the time so we let the
  // temperature go a little over before we turn it off and a little under
  // before we turn it on again. +/-0.5ºC is the minimum granularity based on
  // the sensitivity of the sensor we are using so we’ll use that.
  if (gTemperature >= (gTargetTemp + 0.5)) {
    if (gDebug) Serial.println("debug: computing OperatingState to Off");
    return FALSE;
  }
  else if (gTemperature <= (gTargetTemp - 0.5) ) {
    if (gDebug) Serial.println("debug: computing OperatingState to On");
    return TRUE;
  }
  else {
    // leave it at whatever the current state is
    if (gDebug) Serial.println("debug: leaving the operating state the way it is");
    return gState.callingForHeat;
  }
} 

void setLatchingOperatingState(int callForHeat) {
  int pin;
  if (callForHeat) {
    pin = THERMOSTAT_ON_PIN;
  }
  else {
    pin = THERMOSTAT_OFF_PIN;
  }

  // pulse the appropriate pin to toggle the latching relay
  digitalWrite(pin, HIGH);
  delay(RELAY_LATCH_PULSE_TIME);
  digitalWrite(pin, LOW);
}

// setup() runs once, when the device is first turned on.
// Put initialization like pinMode and begin functions here.
void setup() {
  if (gDebug) {
      Serial.begin(9600);            // open the serial port at 9600 bps:
      Serial.println("\nSetup...");  // prints another carriage return
  }

  // READ Target Temp from EEPROM
  gState.callingForHeat = FALSE;
  gState.mode = HEAT_ON;

  Particle.variable("temperature", &gTemperature, DOUBLE);
  Particle.variable("humidity", &gHumidity, DOUBLE);
  gDHT.begin();
        
  Particle.variable("Calling4Heat", &gState.callingForHeat, INT);

  pinMode(THERMOSTAT_ON_PIN, OUTPUT);
  pinMode(THERMOSTAT_OFF_PIN, OUTPUT);

  digitalWrite(THERMOSTAT_ON_PIN, LOW);
  digitalWrite(THERMOSTAT_OFF_PIN, LOW);

  //
  // TODO load state from EEPROM
  //
  
  // initialize latching relay
  gState.callingForHeat = FALSE;
  setLatchingOperatingState(FALSE);

  gDisplay.setup();
  gDisplay.clearDisplay();
  gDisplay.setTextSize(1);
	gDisplay.setTextColor(WHITE);
	gDisplay.setCursor(15, 24);
	gDisplay.println("Festeworks");
	gDisplay.display();
}

// TODO Blink LED on ERROR

// TODO
// Draws the following state examples:
//         22.5
// Off     22.5
// Heat to 24.0
//         58%
// Error

void drawDisplay() {
  char valueBuffer[5];
  char suffixBuffer[5];

  gDisplay.clearDisplay();
	gDisplay.setTextColor(WHITE);

  double displayValue;
  char const * displayText = NULL;

  switch (gState.display) {
  case TARGET_TEMPERATURE:
    displayValue = dialComputeTargetTemperature(); // TODO this call needs to be abstracted from dial
    displayText = "Heat to";
    break;
  case CURRENT_HUMIDITY:
    displayValue = gHumidity;
    break;
  default: // case CURRENT_TEMPERATURE:
    displayValue = gTemperature;
    if (gState.mode == HEAT_OFF)
      displayText = "Off";
    break;  
  }

  // layout current temperature
  // integer part of temperature or humidity in 24pt
  if (displayText) {
    // draw operating state
    gDisplay.setCursor(0, 12);
    gDisplay.setFont(&FreeSans9pt7b);
    gDisplay.setCursor(0, 31);
    gDisplay.print(displayText);
  }

  if (gState.display != ERROR) {
    // Draw current number
    // actually place this right justified and vertically centered on the 128x32
    // display
    // gDisplay.setCursor(127 - intBounds.width - decBounds.width, (32 -
    // intBounds.height) / 2 + intBounds.height);
    // extract the decimal so that we can draw it smaller
    sprintf(valueBuffer, "%2.0f", displayValue);
    int intValue = (int) displayValue;  // lose the decimal
    if (gState.display == TARGET_TEMPERATURE || gState.display == CURRENT_TEMPERATURE) {
      sprintf(suffixBuffer, ".%d",
          (int)((displayValue - (double)intValue) * 10.0));
    }
    else {
      // assert(gState.display == CURRENT_HUMIDITY);
      sprintf(suffixBuffer, "%%");
    }
    gDisplay.setFont(&FreeSans18pt7b);
    GFXBoundingBox intBounds;
    gDisplay.getTextBounds(valueBuffer, 0, 32, &intBounds.x1, &intBounds.y1,
                          &intBounds.width, &intBounds.height);

    gDisplay.setFont(&FreeSans12pt7b);
    GFXBoundingBox decBounds;
    gDisplay.getTextBounds(suffixBuffer, 0, 32, &decBounds.x1, &decBounds.y1,
                          &decBounds.width, &decBounds.height);

    gDisplay.setCursor(115 - intBounds.width - decBounds.width, 31);
    gDisplay.setFont(&FreeSans18pt7b);
    gDisplay.print(valueBuffer);
    gDisplay.setFont(&FreeSans12pt7b);
    gDisplay.print(suffixBuffer);
 
  }
	gDisplay.display();
}

//
// step the targetTemperature up or down
// either 0.5 degrees C or 1 degree F
// by the specified number of steps
// if called from a push button this would be 1
// if from a dial or slider then this could be many
//
double stepTargetTemperature(int steps) {
  // TODO HANDLE FARENHEIGHT step by 1 degree instead of 0.5
  return max(min(gTargetTemp + steps * 0.5, TEMP_MAX), TEMP_MIN);
}

// Take a delta position and calculate a new target temperature
// based on 4 positions temperature step value
// if the temperature hits the min or max then we should reset the
// reference position so that spinning it the otherway will yield
// immediate benefit.
double dialComputeTargetTemperature() {
  // no target temperature above 30 degrees; no temperature below 5
  //return max(min(gTargetTemp + gtempDelta, 30.0), 5.0);
  double newTemp = stepTargetTemperature((gDialCurrentPosition - gDialReferencePosition) / 4);
  if (newTemp == TEMP_MAX || newTemp == TEMP_MIN) {
    // reset the reference position
    gDialReferencePosition = gDialCurrentPosition;
  }
  return newTemp;
}

//
// Main Event loop
//
void loop() {
  // debounce display buttons
  gDisplay.loop();

  // Handle Dial Rotation
  gDialCurrentPosition = gDial.read();
  if (gDialCurrentPosition != gDialReferencePosition) {
    // bump up or down by 0.5 for each two positions up or down
    // above logic makes sure that a partial click is ignored
    gChangingTargetTemperature = TRUE;
    gState.display = TARGET_TEMPERATURE;
    gDialLastMod = millis();
    // Serial.println(newPosition);
  }
  else if (gChangingTargetTemperature && (millis() - gDialLastMod > 3000)) {
    // dial has not been modified for more that 3 seconds
    gChangingTargetTemperature = FALSE;

    // no target temperature above 30 degrees; no temperature below 5
    gTargetTemp = dialComputeTargetTemperature();
    gState.display = CURRENT_TEMPERATURE;
    gDialReferencePosition = gDialCurrentPosition;
    //TODO write changes in targetTemp to EEPROM
  }

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
  gHumidity = (double)humidity;

  //time_t now = millis();

  int callingForHeat = computeTargetOperatingState();
  if (callingForHeat != gState.callingForHeat) {
    gState.callingForHeat = callingForHeat;
    // set latching relay based operating state
    setLatchingOperatingState(gState.callingForHeat)
    ;
    // gintOpState = (int) goperatingState;
  }

  drawDisplay();

}

