#include "application.h"
#line 1 "/Users/psahota/dev/particle/led-test/src/led-test.ino"
/*
 * Project led-test
 * Description:
 * Author:
 * Date:
 */


/*
 * *************************
 */
void setup();
void loop();
#line 12 "/Users/psahota/dev/particle/led-test/src/led-test.ino"
class LED {
    private:
      boolean _power;
      uint8_t _pin;
    
    public:
     LED(uint8_t pin);
     void begin(void);
     void toggle(void);
     void power(boolean power);
};

LED::LED(uint8_t pin) {
    _pin = pin;
}

void LED::begin(void) {
  pinMode(_pin, OUTPUT);
}

void LED::power(boolean power) {
  _power = power;
  // digitalWrite(_pin, _power ? HIGH : LOW);
  analogWrite(_pin, 255);
}

/*
 * ************************
 */

//#include "Adafruit_DHT.h"
#include "PietteTech_DHT.h" 

// Put all the PIN defitions in one place so you don't accidently change the wrong one when messing around with prototypes
#define DHT_PIN D1
#define LED_PIN D2

#define DHT_TYPE DHT22

LED led1(LED_PIN);
//DHT dht(DHT_PIN, DHT22);
PietteTech_DHT DHT(DHT_PIN, DHT_TYPE);

double gTemperature = NAN;
double gHumidity = NAN;

int result = DHT.acquireAndWait(1000); // wait up to 1 sec (default indefinitely)


// setup() runs once, when the device is first turned on.
void setup() {
  // Put initialization like pinMode and begin functions here.
  led1.begin();

  // DHT (Digital Humidity & Temperature) Setup
  Particle.variable("temperature", &gTemperature, DOUBLE);
  Particle.variable("humidity", &gHumidity, DOUBLE);

  // delay to let the sensors settle in
  delay(1000);
  //dht.begin();
  
}

// loop() runs over and over again, as quickly as it can execute.
void loop() {
  float humidity = NAN,
        temperature = NAN;

  led1.power(1);

  int result = DHT.acquireAndWait(1000); // wait up to 1 sec (default indefinitely)

  //humidity = dht.getHumidity();
  humidity = DHT.getHumidity();
  delay(50);
  //temperature = dht.getTempCelcius();
  temperature = DHT.getCelsius();

  // update particle variables
  gTemperature = (double) temperature;
  gHumidity = (double) humidity;
}