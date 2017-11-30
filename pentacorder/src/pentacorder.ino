/*
* Project pentacorder
* Description:
* Author:
* Date:
*/

// BUTTON
#define BUTTON_UP 0
#define BUTTON_DOWN 1
int BUTTON_PIN = D6;
int gButtonState = BUTTON_UP;
time_t gBegunPressAt = 0;

// PHOTORECEPTOR
int PHOTORESISTOR_PIN = A0;
int PHOTORESISTOR_POWER = A5;
int gLuminosity = 0;

// DHT (Temperature and Humidity)
// TODO PUT DHT Sensor info here
// int gTemperature = NAN;
// int gHumidity = NAN;

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

int gPublishing = 0;
int gFirstPublish = 0;
int gFrequency = 5 * 60 * 1000; // default frequency: 5 minutes
time_t gNextObservation = 0; // time at/after which to make the next observation

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

int setFrequency(String frequency) {
  int new_frequency = atoi(frequency);
  if (new_frequency == 0) {
    // 0 is not a valid frequency
    // to effectively turn off set to MAXINT
    return 0;
  }

  gFrequency = new_frequency;
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
  Particle.variable("luminosity", &gLuminosity, INT);

  // DHT (Humidity & Temperature) SETUP
  // TODO

  // PUBLISHING
  Particle.function("publish", publishingControl);
  Particle.variable("publishing", &gPublishing, INT);

  Particle.function("set-freq", setFrequency);
  Particle.variable("frequency", &gFrequency, INT);
}

void makeObservations() {
  int luminosity_now = 0;
  char working_buffer[64];

  // make new observations; i.e. read sensors
  luminosity_now = analogRead(PHOTORESISTOR_PIN);

  if (gPublishing) {
    String data;
    data = "[";
    // TODO code simple behaviour
    sprintf(working_buffer,
        "{metric: \"luminosity\", value: \"%i\", unit: \"Byte\"}",
        luminosity_now);
    data += working_buffer;

    //data += ", ";
    //sprintf(working_buffer,
    //    "{metric: \"temperature\", value: \"%.01f\", unit: \"Celsius\"}",
    //    temperature_now);
    //data += working_buffer;

    //data += ", ";
    //sprintf(working_buffer,
    //    "{metric: \"humidity\", value: \"%.01f\", unit: \"%%\"}",
    //    humidity_now);
    //data += working_buffer;

    data += "]";
    Particle.publish("com-cranbrooke-observations", data);

    //if (abs(luminosity_now - gLuminosity) >= 25) {
      // if the photo resistor value has changed by more than 10% (~25) then
      // publish an event. This is to avoid unnecessary noise in the publishing
      // such as clouds, trees, people passing in front of a the sensor, etc.
    //  sprintf(data, "{new: %i, old: %i}", luminosity_now, gLuminosity);
    //  Particle.publish("com-cranbrooke-luminosity-change", data);
    //}
  }

  // update globals
  gLuminosity = luminosity_now;
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
      sprintf(data, "{event: \"pressed\"}", gBegunPressAt);
      Particle.publish("com-cranbrooke-button", data);
    }
  }
  else {
    // BUTTON UP
    if (gButtonState == BUTTON_DOWN) {
      // button was down so throw a button up event
      // TODO add time stamp and duration
      // TODO double click handling?

      sprintf(data, "{event: \"released\", duration: %u}",
                    millis() - gBegunPressAt);
      Particle.publish("com-cranbrooke-button", data);

      // clear the button state
      gButtonState = BUTTON_UP;
    }
  }

  time_t now = millis();
  if (now >= gNextObservation) {
    makeObservations();
    // increment based on the scheduled time of the previous to keep the
    // observations evenly spaced without collection time drift.
    gNextObservation += gFrequency;
    if (gNextObservation < now) {
      // a delay occurred and since we don't want/need two observations close
      // together so push out the nextObservation time
      gNextObservation = millis() + gFrequency;
    }
  }

  delay(10);
}
