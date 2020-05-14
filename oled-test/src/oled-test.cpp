#include "application.h"
#line 1 "/Users/psahota/dev/particle/oled-test/src/oled-test.ino"
/*
 * Project oled-test
 * Description:
 * Author:
 * Date:
 */

#include <oled-wing-adafruit.h>

void setup();
void loop();
#line 10 "/Users/psahota/dev/particle/oled-test/src/oled-test.ino"
OledWingAdafruit display;

void setup() {
	display.setup();

	display.clearDisplay();
	display.display();
}

void loop() {
	display.loop();

	if (display.pressedA()) {
		display.clearDisplay();

		display.setTextSize(1);
		display.setTextColor(WHITE);
		display.setCursor(0,0);
		display.println("Hello, world!");
		display.display();
	}

	if (display.pressedB()) {
	}

	if (display.pressedC()) {
	}
}
