/*
* Project pentacorder
* Description:
* Author:
* Date:
*/

#include "Adafruit_DHT.h"

struct ObservationType {
    String name;
    String unit;
    int unitMax;
    int minMin;
};

class Observations {
  public:
    Observations(ObservationType *);
};

Observations::Observations(ObservationType *types) {

}

struct DoubleSampledObservations {
  String name;
  String unit;
  double first;
  double last;
  double min;
  double max;
  double average;
  double alertThreshold;
  int sampleCount; // count of sucessful samples computed into average
  int errorCount; // error count when sampling sensor
};

struct IntSampledObservations {
  String name;
  String unit;
  int first;
  int last;
  int min;
  int max;
  double average; // or course, average doesn't have to be an int
  int alertThreshold;
  int sampleCount; // count of sucessful samples computed into average
  int errorCount; // error count when sampling sensor
};

void addIntSample(struct IntSampledObservations *samples, int new_value) {
    if (samples->sampleCount == 0) {
      // first sample
      samples->first = new_value;
      samples->max = new_value;
      samples->min = new_value;
      samples->average = new_value;
    }
    else {
      // we have more than one sample so we can compute the
      // average, max and min
      if (new_value > samples->max) {
        samples->max = new_value;
      }

      if (new_value < samples->min) {
        samples->min = new_value;
      }

      samples->average =
        ((samples->average * samples->sampleCount) + new_value)
                      /
          (samples->sampleCount + 1);

    }

    samples->sampleCount += 1;
    samples->last = new_value;
}

void addDoubleSample(struct DoubleSampledObservations *samples, double new_value) {
    if (samples->sampleCount == 0) {
      // first sample
      samples->first = new_value;
      samples->max = new_value;
      samples->min = new_value;
      samples->average = new_value;
    }
    else {
      // we have more than one sample so we can compute the
      // average, max and min
      if (new_value > samples->max) {
        samples->max = new_value;
      }

      if (new_value < samples->min) {
        samples->min = new_value;
      }

      samples->average =
        ((samples->average * samples->sampleCount) + new_value)
                      /
          (samples->sampleCount + 1);

    }

    samples->sampleCount += 1;
    samples->last = new_value;
}


//String * IntObservationToJSON(String *working_buffer)
//  sprintf(working_buffer,
//    "{\"metric\": \"luminosity\", \"id\"=\"0\", \"value\": \"%i\", \"unit\": \"Byte\"}",
//    gLuminosity.last);



// ******************************************

time_t gFirstObservation = 0;
time_t gLastObservation = 0;

// BUTTON
#define BUTTON_UP 0
#define BUTTON_DOWN 1
int BUTTON_PIN = D6;
int gButtonState = BUTTON_UP;
// TODO millis() will overflow after 49 days; use Software Timers instead!!
unsigned long gBegunPressAt = 0;

// PHOTORECEPTOR
int PHOTORESISTOR_PIN = A0;
int PHOTORESISTOR_POWER = A5;
// int gLuminosity = 0;

IntSampledObservations gLuminosity = {
  "luminosity",
  "%%",
  0, 0, 0, 0,
  MAXINT, 0, 0
};

// WATER LEVEL
int WATER_LEVEL_PIN = A4;
int gWaterLevel = = {
  "waterlevel",
  "waterlevel",
  0, 0, 0, 0,
  100, // Default Threshold for water is 100; TODO TEST IN REAL WORLD AGAIN
  0, 0
};

// DHT (Temperature and Humidity)
// TODO PUT DHT Sensor info here
#define DHTPIN D4
#define DHTTYPE DHT22

DHT dht(DHTPIN, DHTTYPE);

double gTemperature = NAN;
double gHumidity = NAN;

// LED
#define LED_ON 1
#define LED_OFF 0
int LED_PIN = D1;
int gLEDState = LED_OFF;

int setLED(String command) {
  if (command == "on") {
    // turn the LED On
    digitalWrite(LED_PIN, HIGH);
    gLEDState = LED_ON;
    return 1;
  }
  else if (command == "off") {
    // turn the LED Off
    digitalWrite(LED_PIN, LOW);
    gLEDState = LED_OFF;
    return 1;
  }
  return 0;
}

int gObservationFrequency = 20 * 1000; // default frequency: 20 seconds
time_t gNextObservation = millis() - 1; // time at/after which to make the next observation

int gPublishing = 1;
time_t gNextEdition = 0;  // time at/after which to publish the next edition of
                          // observations.
int gFirstPublish = 0;
int gPublishingFrequency = 60 * 60 * 1000; // default 1 hour

int publishingControl(String command) {
   if (command == "start") {
        gPublishing = 1;
        // make sure that we publish an observation on the next loop
        gNextObservation = millis() - 1;
        Particle.publish("com-cranbrooke-info", "starting observation publication");
        return 1;
   }
   else if (command == "stop") {
        Particle.publish("com-cranbrooke-info", "stopping observation publication");
        gPublishing = 0; // set the flag to not collect observations
        return 1;
   }

    Serial.println("publish:Error (illegal CMD).");
    Particle.publish("error", "illegal command");

    return 0;
}

int setObservationFrequency(String frequency) {
  int new_frequency = atoi(frequency);
  if (new_frequency == 0) {
    // 0 is not a valid frequency
    // to effectively turn off set to MAXINT
    return 0;
  }

  gObservationFrequency = new_frequency;

  // make sure that we publish an observation on the next loop
  gNextObservation = millis() - 1;
  return 1;
}

void setup() {
  Particle.variable("button", &gButtonState, INT);

  // BUTTON SETUP
  // For input, we define the
  // pushbutton as an input-pullup
  // this uses an internal pullup resistor
  // to manage consistent reads from the device
  pinMode(BUTTON_PIN, INPUT_PULLUP); // sets pin as input

  // LED SETUP
  pinMode(LED_PIN , OUTPUT); // sets pin as output
  // Add ability to control the led from the cloud
  Particle.function("set-led", setLED);
  Particle.variable("led", &gLEDState, INT);

  // PHOTORESISTOR (Luminosity) SETUP
  // Our photoresistor pin is input (reading the photoresistor)
  pinMode(PHOTORESISTOR_PIN, INPUT);
  // The pin powering the photoresistor is output (sending out consistent power)
  pinMode(PHOTORESISTOR_POWER, OUTPUT);
  // Next, write one pin of the photoresistor to be the maximum possible,
  // so that we can use this for power.
  digitalWrite(PHOTORESISTOR_POWER, HIGH);
  // We are going to declare a Particle.variable() here so that we can access
  // the value of the photoresistor from the cloud.
  Particle.variable("luminosity", &gLuminosity.last, INT);

  // DHT (Humidity & Temperature) SETUP
  Particle.variable("temperature", &gTemperature, DOUBLE);
  Particle.variable("humidity", &gHumidity, DOUBLE);
  dht.begin();

  // WATER LEVEL SETUP
  pinMode(WATER_LEVEL_PIN, INPUT);
  Particle.variable("waterlevel", &gWaterLevel.last, INT);

  // PUBLISHING
  Particle.function("publish", publishingControl);
  Particle.variable("publishing", &gPublishing, INT);

  Particle.function("set-freq", setObservationFrequency);
  Particle.variable("frequency", &gObservationFrequency, INT);
}

void makeObservations() {
  int luminosity_now = 0;
  float humidity_now = 0,
    temperature_now = 0;
  float waterlevel_now = 0;

  // TODO implemnet sampling structure, counter struct,
  // TODO first/last, sample count, etc

  // make new observations; i.e. read sensors
  luminosity_now = analogRead(PHOTORESISTOR_PIN);

  humidity_now = dht.getHumidity();
  delay(50);
  temperature_now = dht.getTempCelcius();

  waterlevel_now = analogRead(WATER_LEVEL_PIN);

  // update globals
  //gLuminosity = luminosity_now;
  addIntSample(&gLuminosity, luminosity_now);
  gTemperature = (double) temperature_now;
  gHumidity = (double) humidity_now;
  addIntSample(&gWaterLevel, waterlevel_now);
}


void publishObservations() {
  //char working_buffer[2048];
  char *working_buffer = (char *) malloc(2048);
  // TODO rewrite to new format

  String data;
  // start time
  // end time
  // number of samples
  data = "[";
  // TODO code simple behaviour

  //
  // SENSORS (Sampled)
  //

  // average value, last value, max value, min value
  sprintf(working_buffer,
      "{\"metric\": \"luminosity\", \"id\"=\"0\", \"value\": \"%i\", \"unit\": \"Byte\"}",
      gLuminosity.last);
  data += working_buffer;

  // TODO collect WIFI Strength WiFi.RSSI()

  data += ", ";
  sprintf(working_buffer,
      "{\"metric\": \"temperature\", \"id\"=\"0\", \"value\": \"%.01f\", \"unit\": \"Celsius\"}",
      gTemperature);
  data += working_buffer;

  data += ", ";
  sprintf(working_buffer,
      "{\"metric\": \"humidity\", \"id\"=\"0\", \"value\": \"%.01f\", \"unit\": \"%%\"}",
      gHumidity);
  data += working_buffer;

  //
  // Managed State
  //

  data += ", ";
  sprintf(working_buffer,
      "{\"metric\": \"led\", \"id\"=\"0\", \"value\": \"%i\", \"unit\": \"Boolean\"}",
      gLEDState);
  data += working_buffer;

  data += ", ";
  sprintf(working_buffer,
      "{\"metric\": \"pushbutton\", \"id\"=\"0\", \"value\": \"%i\", \"unit\": \"Boolean\"}",
      gButtonState);
  data += working_buffer;

  data += ", ";
  sprintf(working_buffer,
      "{\"metric\": \"collection-frequency\", \"value\": \"%i\", \"unit\": \"Duration\"}",
      gObservationFrequency);
  data += working_buffer;

  data += "]";
  Particle.publish("com-cranbrooke-observations", data);
  //if (abs(luminosity_now - gLuminosity) >= 25) {
    // if the photo resistor value has changed by more than 10% (~25) then
    // publish an event. This is to avoid unnecessary noise in the publishing
    // such as clouds, trees, people passing in front of a the sensor, etc.
  //  sprintf(data, "{new: %i, old: %i}", luminosity_now, gLuminosity);
  //  Particle.publish("com-cranbrooke-luminosity-change", data);
  //}

  free(working_buffer);
}



void loop() {

  // find out if the button is pushed
  // or not by reading from it.
  int buttonState = digitalRead(BUTTON_PIN);
  char data[64];

  // remember that we have wired the pushbutton to
  // ground and are using a pulldown resistor
  // that means, when the button is pushed,
  // we will get a LOW signal
  // when the button is not pushed we'll get a HIGH

  // let's use that to set our LED on or off

  if (buttonState == LOW) {
    // BUTTON DOWN
    if (gButtonState == BUTTON_UP) {
      // switch the global state
      gButtonState = BUTTON_DOWN;
      gBegunPressAt = millis();

      // publish a button down event
      // TODO add time stamp to data
      sprintf(data, "{\"id\": \"0\"}", gBegunPressAt);
      Particle.publish("com-cranbrooke-pushbutton-pressed", data);
    }

  }
  else {
    // BUTTON UP
    if (gButtonState == BUTTON_DOWN) {
      // button was down so throw a button up event
      // TODO add time stamp and duration
      // TODO double click handling?

      sprintf(data, "{\"id\": \"0\", duration: %u}",
                    millis() - gBegunPressAt);
      Particle.publish("com-cranbrooke-pushbutton-released", data);

      // clear the button state
      gButtonState = BUTTON_UP;
    }
  }

  unsigned long now = millis();
  //
  // Make Observations when the time is right
  //
  if (now >= gNextObservation) {
    makeObservations();
    // increment based on the scheduled time of the previous to keep the
    // observations evenly spaced without collection time drift.
    gNextObservation += gObservationFrequency;
    if (gNextObservation < now) {
      // a delay occurred and since we don't want/need two observations close
      // together so push out the nextObservation time
      gNextObservation = millis() + gObservationFrequency;
    }
  }

  //
  // Publish Observations when the time is right, if publishing is enabled
  //
  if (gPublishing && (now >= gNextEdition)) {
    publishObservations();
    // increment based on the scheduled time of the previous to keep the
    // editions evenly spaced without collection time drift.
    gNextEdition += gPublishingFrequency;
    if (gNextEdition < now) {
      // a delay occurred and since we don't want/need two publications close
      // together so push out the nextObservation time
      gNextEdition = millis() + gPublishingFrequency;
    }
  }

  delay(10);
}
