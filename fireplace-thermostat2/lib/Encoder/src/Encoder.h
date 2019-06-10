/* Encoder Library, for measuring quadrature encoded signals
 * http://www.pjrc.com/teensy/td_libs_Encoder.html
 * Copyright (c) 2011,2013 PJRC.COM, LLC - Paul Stoffregen <paul@pjrc.com>
 *
 * Version 1.2 - fix -2 bug in C-only code
 * Version 1.1 - expand to support boards with up to 60 interrupts
 * Version 1.0 - initial release
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */


#ifndef Encoder_h_
#define Encoder_h_

#include "Particle.h"

#define IO_REG_TYPE                     uint32_t
#if defined(HAL_PLATFORM_NRF52840) && HAL_PLATFORM_NRF52840
# define PIN_TO_BASEREG(pin)             (0)
# define PIN_TO_BITMASK(pin)             (pin)
# define DIRECT_PIN_READ(base, mask)     pinReadFast(mask)
#else
#define PIN_TO_BASEREG(pin)             (&(HAL_Pin_Map()[pin].gpio_peripheral->IDR))
#define PIN_TO_BITMASK(pin)             (HAL_Pin_Map()[pin].gpio_pin)
#define DIRECT_PIN_READ(base, mask)     (((*(base)) & (mask)) ? 1 : 0)
#endif

class Encoder
{
public:
	Encoder(pin_t pin1, pin_t pin2) {
		pinMode(pin1, INPUT_PULLUP);
		pinMode(pin2, INPUT_PULLUP);

		pin1_register = PIN_TO_BASEREG(pin1);
		pin1_bitmask = PIN_TO_BITMASK(pin1);
		pin2_register = PIN_TO_BASEREG(pin2);
		pin2_bitmask = PIN_TO_BITMASK(pin2);
		position = 0;
		// allow time for a passive R-C filter to charge
		// through the pullup resistors, before reading
		// the initial state
		delayMicroseconds(2000);
		uint8_t s = 0;
		if (DIRECT_PIN_READ(pin1_register, pin1_bitmask)) s |= 1;
		if (DIRECT_PIN_READ(pin2_register, pin2_bitmask)) s |= 2;
		state = s;
		attachInterrupt(pin1, &Encoder::interruptHandler, this, CHANGE);
		attachInterrupt(pin2, &Encoder::interruptHandler, this, CHANGE);
	}

	inline int32_t read() {
		noInterrupts();
		int32_t ret = position;
		interrupts();
		return ret;
	}
	inline void write(int32_t p) {
		noInterrupts();
		position = p;
		interrupts();
	}
private:
	volatile IO_REG_TYPE * pin1_register;
	volatile IO_REG_TYPE * pin2_register;
	IO_REG_TYPE            pin1_bitmask;
	IO_REG_TYPE            pin2_bitmask;
	uint8_t                state;
	int32_t                position;

//                           _______         _______       
//               Pin1 ______|       |_______|       |______ Pin1
// negative <---         _______         _______         __      --> positive
//               Pin2 __|       |_______|       |_______|   Pin2

		//	new	new	old	old
		//	pin2	pin1	pin2	pin1	Result
		//	----	----	----	----	------
		//	0	0	0	0	no movement
		//	0	0	0	1	+1
		//	0	0	1	0	-1
		//	0	0	1	1	+2  (assume pin1 edges only)
		//	0	1	0	0	-1
		//	0	1	0	1	no movement
		//	0	1	1	0	-2  (assume pin1 edges only)
		//	0	1	1	1	+1
		//	1	0	0	0	+1
		//	1	0	0	1	-2  (assume pin1 edges only)
		//	1	0	1	0	no movement
		//	1	0	1	1	-1
		//	1	1	0	0	+2  (assume pin1 edges only)
		//	1	1	0	1	-1
		//	1	1	1	0	+1
		//	1	1	1	1	no movement
/*
	// Simple, easy-to-read "documentation" version :-)
	//
	void update(void) {
		uint8_t s = state & 3;
		if (digitalRead(pin1)) s |= 4;
		if (digitalRead(pin2)) s |= 8;
		switch (s) {
			case 0: case 5: case 10: case 15:
				break;
			case 1: case 7: case 8: case 14:
				position++; break;
			case 2: case 4: case 11: case 13:
				position--; break;
			case 3: case 12:
				position += 2; break;
			default:
				position -= 2; break;
		}
		state = (s >> 2);
	}
*/

private:
	void interruptHandler() {
		uint8_t p1val = DIRECT_PIN_READ(pin1_register, pin1_bitmask);
		uint8_t p2val = DIRECT_PIN_READ(pin2_register, pin2_bitmask);
		uint8_t newState = state & 3;
		if (p1val) newState |= 4;
		if (p2val) newState |= 8;
		state = (newState >> 2);
		switch (newState) {
			case 1: case 7: case 8: case 14:
				position++;
				return;
			case 2: case 4: case 11: case 13:
				position--;
				return;
			case 3: case 12:
				position += 2;
				return;
			case 6: case 9:
				position -= 2;
				return;
		}
	}
};

#endif
