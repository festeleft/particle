/*
 * Project dht-monitor
 * Description:
 * Author:
 * Date:
 */

#include "Adafruit_DHT.h"

#define DHTPIN D1    // CHANGEME: Digital Pin on your board that the DHT sensor is connected to
#define DHTTYPE DHT11 // CHANGEME: To the type of DHT sensor you are using

#define ERROR_READING_HUMIDITY 1
#define ERROR_READING_TEMPERATURE 2
#define ERROR_ILLEGAL_COMMAND 4

DHT dht(DHTPIN, DHTTYPE);

int g_observing = 1; // CHANGEME: set to 0 to have the unit not log after a restart/power failure
int g_sample_frequency = 60000; // CHANGEME: sample frequency set to 1 minute
  // TODO mechanism to dynamically update the sample frequency
  // TODO Idea of temporarily ramping up the frequency while someone is
  //      actively inspecting the data. Could be a simple poll instruction.


int pollControl(String command) {
   if (command == "start-observations") {
        g_observing = 1;
        bool success;
        success = Particle.publish("info", "starting observations");
        if (!success) {
            Serial.println("publish start info failed!!!!");
        }
        return 1;
   }
   else if (command == "stop-observations") {
        bool success;
        success = Particle.publish("info", "stopping observations");
        if (!success) {
          Serial.println("publish stop info failed!!!!");
        }
        Serial.println("stopping...");
        g_observing = 0; // set the flag to not collect observations
        return 1;
   }

    Serial.println("Error (illegal CMD).");
    bool success;
    success = Particle.publish("error", "illegal poll command");
    if (!success) {
      Serial.println("publish illegal poll command failed!!!!");
    }

    return 0;
}

void publishObservations(float humidity, float temperature) {
    char working_buffer[64];
    String data;
    data = "[";
    sprintf(working_buffer,
        "{metric: \"temperature\", value: \"%.01f\", unit: \"Celcius\"}",
        temperature);
    data += working_buffer;
    data += ", ";
    sprintf(working_buffer,
        "{metric: \"humidity\", value: \"%.01f\", unit: \"%%\"}",
        humidity);
    data += working_buffer;
    data += "]";
    Particle.publish("observations", data);
}


// setup() runs once, when the device is first turned on.
// Put initialization like pinMode and begin functions here.
void setup() {
    // OUTPUT to USB
    Serial.begin(9600);      // open the serial port at 9600 bps:
    Serial.println("\nSetup...");      // prints another carriage return

    // Register our Spark function here
    Spark.function("poll", pollControl);

    dht.begin();
}

unsigned long g_next_observation = 0; // time at which to make the next observation

// loop() runs over and over again, as quickly as it can execute.
void loop() {
    // The core of your code will likely live here.
    float humidity;    // humidity
    float temperature;    // temperature
    int failure = 0;
    String serial_data;
    char working_buffer[64];

    if (g_observing) {
        if (millis() <= g_next_observation) {
          return;
        }

        g_next_observation = millis() + g_sample_frequency;
        failure = 0; // clear failure flag before attempt to read the sensors
        humidity = dht.getHumidity();
        delay(50);
        temperature = dht.getTempCelcius();

        if (temperature == NAN) {
            Serial.println("FAILED to read Temperature");
            Particle.publish("error", "FAILED to read Temperature");
            failure |= ERROR_READING_TEMPERATURE;
        }
        if (humidity == NAN) {
            Serial.println("FAILED to read Humidity");
            Particle.publish("error", "FAILED to read Humidity");
            failure |= ERROR_READING_HUMIDITY;
        }

        // optional serial dump of the information
        serial_data = "";
        sprintf(working_buffer, "temperature=%2f, humidity=%2f\n",
                temperature, humidity);
        serial_data += working_buffer;
        Serial.println(serial_data);

        if (!failure) {
            // Success
            publishObservations(humidity, temperature);
        }
    }
    else {
        delay(10); // wait a few milliseconds before looping around again
    }
}
