//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
// kellyzador.ino - Firmware code for the Kellyzador Fraction Collector project
//    developed by Arley and Nathan for the SuperNANO laboratory at
//    Universidade Federal do Rio de Janeiro.
//
// Author: Cesar Raitz
// Date: Sep. 2, 2024
//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
#include <LiquidCrystal.h>
#include <AccelStepper.h>
#include "Machine.h"
#include "icons.h"
#include "hmsTime.h"

// #define USE_BUTTON_SNITCH
#include "AnalogButtons.h"

#define VERSION  "1.01"

// Small 28BYJ-48 Motor (Sample Carrousel)
#define BYJ48_STEPS        (2*2054L)  // steps/turn
#define BYJ48_STEP_PIN     11    // red wire
#define BYJ48_DIR_PIN      3     // blue wire
#define BYJ48_MAX_SPEED    1000  // steps/s
#define BYJ48_ENABLE_PIN   34

// Nema 17 Motor (Linear Actuator Axis)
#define NEMA17_STEPS       200   // steps/turn
#define NEMA17_STEP_PIN    13    // green wire
#define NEMA17_DIR_PIN     12    // blue wire
#define NEMA17_MAX_SPEED   1000  // steps/s

#define LIMIT_SWITCH_PIN   22
#define BEEPER_PIN         31

#define MAX_SAMPLES        80
#define AXIS_RANGE         100   // mm
#define CARR_RANGE         1080  // °

#define AXIS_STEPS_PER_MM    NEMA17_STEPS/8

// #define CARR_REV_STEPS  ((79L*BYJ48_STEPS+9)/18)

/// Carroussel steps per revolution
const long carrRevSteps = (79L*BYJ48_STEPS+9)/18;


// GLOBAL VARIABLES
//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

// The NEMA17 motor driven by an A4988
AccelStepper axisMotor(1, NEMA17_STEP_PIN, NEMA17_DIR_PIN);
// The 28BYJ-48 motor driven by an A4988
AccelStepper carrMotor(1, BYJ48_STEP_PIN, BYJ48_DIR_PIN);

AnalogButtons buttons(A0);

// Fraction Collector's related stuff
struct fcStuff {
	byte firstSample = 1;
	byte lastSample = MAX_SAMPLES;
	byte currSample = firstSample;
	
	int secPerSample = 10;
	int minSampleSecs = 10;
	int secToSwap = 5;
	hmsTime total;
} fc;

// UTILITY FUNCTIONS
//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
inline void beep() { digitalWrite(BEEPER_PIN, LOW); }
inline void noBeep() { digitalWrite(BEEPER_PIN, HIGH); }

inline void enableCarr() { 
	digitalWrite(BYJ48_ENABLE_PIN, LOW);
}
inline void disableCarr() {
	digitalWrite(BYJ48_ENABLE_PIN, HIGH);
}

inline void lcd_printAt(byte x, byte y, char c) {
	lcd.setCursor(x, y); lcd.print(c);
}

inline void lcd_writeAt(byte x, byte y, byte c) {
	lcd.setCursor(x, y); lcd.write(c);
}

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
#define ROW_START_1  1
#define ROW_START_2  33
#define ROW_START_3  54
#define ROW_START_4  69

/**
 * Move the Fraction Collector to another sample holder.
*/
void goToSample(byte num, bool move=true) {
	int row;
	if (num < ROW_START_2) row = 0;
	else if (num < ROW_START_3) row = 1;
	else if (num < ROW_START_4) row = 2;
	else row = 3;

	int newAxisPos = row * AXIS_STEPS_PER_MM * 19;
	long newCarrPos;

	// Slight adjustments for the carrousel position
	const long dy2 = carrRevSteps*125/128,
	           dy3 = 2*carrRevSteps,
	           dy4 = (36-1)*carrRevSteps/12;

	// Calculate the carrousel position for a given row and sample
	switch (row) {
		case 0: newCarrPos = carrRevSteps * (num-ROW_START_1)/32; break;
		case 1:
			if (num < 50)
				newCarrPos = carrRevSteps * (num-ROW_START_2)/22 + dy2;
			else
				newCarrPos = carrRevSteps * (num+1-ROW_START_2)/22 + dy2;
			break;
		case 2: newCarrPos = carrRevSteps * (num-ROW_START_3)/16 + dy3; break;
		case 3: newCarrPos = carrRevSteps * (num-ROW_START_4)/12 + dy4; break;
	}
	newCarrPos &= ~1L;  // prevent stopping on a half-step

			// long t = CARR_REV_STEPS;
			// Serial.print(t); Serial.print(", ");
			// t *= num - ROW_START_2;
			// Serial.print(t); Serial.print(", ");
			// t += dy2;
			// Serial.print(t); Serial.println();
	
	Serial.print("Go to sample #"); Serial.print(num);
	Serial.print("  row:"); Serial.print(row);
	Serial.print("  axs:"); Serial.print(newAxisPos);
	Serial.print("  car:"); Serial.println(newCarrPos);
	if (move) {
		axisMotor.moveTo(newAxisPos);
		carrMotor.moveTo(newCarrPos);
	} else {
		axisMotor.setCurrentPosition(newAxisPos);
		carrMotor.setCurrentPosition(newCarrPos);
	}
}

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

class Over: public Machine {
public:
	void begin() {
		Serial.println("Over");
		lcd.clear();
		lcd.print("Experiment Over!");
		lcd.setCursor(0, 1);
		lcd.print("Total =");
		lcd.print(fc.total.hm_str);
		lcd.print(fc.total.s_str);
	}

	void middle() {
		if (buttons.isPressed())
			next();
	}
};

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

class Experiment: public Machine {
	int remaining;
	bool moving;
	byte iconState = 0;
	const byte iconX=15, iconY=1;

public:
	void printTotal() {
		if (fc.total.print_hm) {
			lcd.setCursor(6, 0);
			lcd.print(fc.total.hm_str);
		}
		lcd.setCursor(13, 0);
		lcd.print(fc.total.s_str);
	}

	void printRemaining() {
		lcd.setCursor(5, 2);
		lcd.print(remaining);
		lcd.print("s ");
	}

	void drawPage() {
		lcd.clear();
		lcd.print("S#");
		lcd.print(fc.currSample);

		lcd.setCursor(0, 1);
		lcd.print("next ");
		lcd.print(remaining);
		lcd.print("s ");

		printTotal();
	}

	void begin() {
		Serial.println("Experiment");
		goToSample(fc.currSample, false);
		remaining = fc.secPerSample;
		fc.total.reset();
		drawPage();
		moving = false;
		repeat(1000, TIMER_1); // to update information on screen and
		                       // move the carrousel to the next sample
	}

	void updateScreen() {
		fc.total.inc();
		if (--remaining == 0) {
			if (fc.currSample == fc.lastSample) {
				disableCarr();
				moving = false;
				next();
				return;
			}
			// We (hopefully) arrived at the next location. Therefore,
			// update the sample number on screen so that the user knows
			lcd.setCursor(2, 0);
			lcd.print(++fc.currSample);
			remaining = fc.secPerSample;
		}
		if (remaining == fc.secToSwap) {
			// We start moving the carrousel before the sample time
			goToSample(fc.currSample+1);
			enableCarr();
			moving = true;
		}
		if (moving) {
			// Small animation to indicate motors' movement
			lcd_printAt(iconX, iconY, iconState? '|': '-');
			iconState = ~iconState;
		}
		printRemaining();
		printTotal();
	}

	// void updateSample() {
	// 	lcd.setCursor(2, 0);
	// 	lcd.print(++fc.currSample);
	// 	remaining = fc.secPerSample;
	// 	printRemaining();
	// }

	void middle() {
		switch (timer()) {
			case TIMER_1: updateScreen(); break;
			case TIMER_2: noBeep(); break;
		}

		if (moving) {
			bool a = carrMotor.run();
			bool b = axisMotor.run();
			if (not (a or b)) {
				disableCarr();
				moving = false;
				lcd_printAt(iconX, iconY, ' ');
				beep();
				countdown(200);
			}
		}
	}
};

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

class Settings: public Machine {
	byte option;
	byte page;
	byte currSample;
	bool moving;
	const char* strReady  = " Select to go!  ";
	const char* strMoving = " Getting ready  ";
	byte x = 14;

public:
	void drawPage() {
		lcd.clear();
		page = option>>1;
		if (page == 0) {
			lcd.print(" First: ");
			lcd.print(fc.firstSample);
			lcd.setCursor(0, 1);
			lcd.print("  Last: ");
			lcd.print(fc.lastSample);
		}
		else {
			lcd.print(" Sec/S: ");
			lcd.print(fc.secPerSample);
			lcd.setCursor(0, 1);
			lcd.print((moving)? strMoving: strReady);
		}
		lcd_writeAt(0, option&1, ICON_RIGHT_ARROW);
	}

	void begin() {
		Serial.println("Settings");
		currSample = fc.firstSample;
		moving = false;
		option = 0;
		page = 0;
		drawPage();
	}

	void goToOption(byte newOpt) {
		newOpt = newOpt&3;
		byte newPage = newOpt>>1;
		if (newPage != page) {
			option = newOpt;
			drawPage();
		}
		else {
			lcd_printAt(0, option&1, ' ');
			lcd_writeAt(0, newOpt&1, ICON_RIGHT_ARROW);
			option = newOpt;
		}
	}

	void incValue(int inc) {
		int v = 0;
		switch (option) {
			case 0:
				v = (int)fc.firstSample + inc;
				if (v > fc.lastSample) v = fc.lastSample;
				else if (v < 1) v = 1;
				if (v == fc.firstSample) return;
				fc.firstSample = v;

				// Move carroussel to a different sample holder
				goToSample(fc.firstSample);
				if (not moving) {
					enableCarr();
					moving = true;
					repeat(500);
				}
				break;
				
			case 1:
				v = (int)fc.lastSample + inc;
				if (v > MAX_SAMPLES) v = MAX_SAMPLES;
				else if (v < fc.firstSample) v = fc.firstSample;
				if (v == fc.lastSample) return;
				fc.lastSample = v;
				break;
			
			case 2:
				v = fc.secPerSample + inc;
				if (v > 30*60) v = 30*60;
				else if (v < fc.minSampleSecs) v = fc.minSampleSecs;
				if (v == fc.secPerSample) return;
				fc.secPerSample = v;
				break;
			
			default:
				return;
		}
		// Change the option's value on screen
		lcd.setCursor(8, option&1);
		lcd.print(v);
		lcd.print("        ");
	}

	void middle() {
		// Verify motor's movement
		if (moving) {
			bool a = carrMotor.run();
			bool b = axisMotor.run();
			if (not (a or b)) {
				if (page == 1) {
					lcd.setCursor(1, 1);
					lcd.print(strReady+1);
				}
				disableCarr();
				fc.currSample = fc.firstSample;
				moving = false;
				endTimer();
			}
		}

		// Verify key presses
		if (buttons.event == PRESS) {
			switch (buttons.button) {
				case UP:    goToOption(option-1); break;
				case DOWN:  goToOption(option+1); break;
				case RIGHT: incValue(+1); break;
				case LEFT:  incValue(-1); break;
				case SELECT:
					if (option != 3) {
						option = 3;
						drawPage();
					}
					else if (not moving) next();
			}
		}
		else if (buttons.event == HOLD) {
			switch (buttons.button) {
				case RIGHT: incValue(+10); break;
				case LEFT:  incValue(-10); break;
			}
		}
		buttons.event = 0;  // mark as handled

		// Verify timer events
		if (timer()) {
			if (page == 1) {
				// Small animation for getting ready
				lcd_printAt(x, 1, ' ');
				x = (x == 14)? 15: 14;
				lcd_printAt(x, 1, '.');
			}
			// Serial.print("              ");
			// Serial.print(axisMotor.distanceToGo());
			// Serial.print(" - ");
			// Serial.println(carrMotor.distanceToGo());
		}
	}
};

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

class SpeedTest: public Machine {
	uint32_t prev;
	char x1, x2;
	const char xi=10;
	bool moving;
	bool forth;

public:
	void animate() {
		if (x1 >= xi) lcd_printAt(x1, 0, ' ');
		if (x2 >= xi) lcd_printAt(x2, 0, '.');
		if (++x1 > xi+3) x1 = xi-5;
		if (++x2 > xi+3) x2 = xi-5;
	}

	void begin() {
		const char *title = "Speed Test";
		Serial.println(title);
		lcd.clear();
		lcd.print(title);
		repeat(200);
		x1 = xi-3;
		x2 = xi;
		moving = true;
		forth = true;
		goToSample(1, false);
		enableCarr();
		goToSample(3);
		prev = millis();
	}

	void middle() {
		if (moving) {
			bool a = carrMotor.run();
			// bool b = axisMotor.run();
			// if (not (a or b)) {
			if (not a) {
				if (forth) {
					forth = false;
					goToSample(1);
					fc.secToSwap = (millis() - prev + 999)/1000;
				}
				else {
					disableCarr();
					moving = false;
					lcd.clear();
					lcd.print("Speed Test");
					lcd.setCursor(0, 1);
					lcd.print(fc.secToSwap);
					lcd.print("s delay. Done!");
					fc.minSampleSecs = fc.secToSwap + 1;
					fc.secPerSample = fc.minSampleSecs;
					endTimer();
				}
			}
		}
		else if (buttons.isPressed()) {
			next();
		}
		if (timer()) animate();
	}
};

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

class CarrouselTest2: public Machine {
	bool moving;
	byte nextSample;

public:
	void begin() {
		const char *title = "Carrousel Test 2";
		Serial.println(title);
		nextSample = fc.currSample;
		moving = false;

		lcd.clear();
		lcd.print(title);
		lcd.setCursor(0, 1);
		lcd.print("Sample #");
		lcd.print(nextSample);
		lcd.print(" ");
	}

	void middle() {
		if (moving) {
			bool a = carrMotor.run();
			bool b = axisMotor.run();
			if (!a and !b) {
				disableCarr();
				moving = false;
				fc.currSample = nextSample;
				countdown(200);
				beep();
				lcd.setCursor(8, 1);
				lcd.print(nextSample);
				lcd.print(" ");
			}
		}
		else if (buttons.isPressed()) {
			switch (buttons.button) {
			case UP:
				nextSample = min(fc.currSample+1, MAX_SAMPLES);
				break;
			
			case DOWN:
				nextSample = max(fc.currSample-1, 1);
				break;

			case LEFT:
				     if (fc.currSample > ROW_START_4) nextSample = ROW_START_4;
				else if (fc.currSample > ROW_START_3) nextSample = ROW_START_3;
				else if (fc.currSample > ROW_START_2) nextSample = ROW_START_2;
				else nextSample = ROW_START_1;
				break;
			
			case RIGHT:
				     if (fc.currSample < ROW_START_2) nextSample = ROW_START_2;
				else if (fc.currSample < ROW_START_3) nextSample = ROW_START_3;
				else if (fc.currSample < ROW_START_4) nextSample = ROW_START_4;
				else nextSample = MAX_SAMPLES;
				break;
			}
			if (nextSample != fc.currSample) {
				goToSample(nextSample);
				moving = true;
				enableCarr();
			}
		}
		if (timer()) noBeep();
	}
};

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

class CarrouselTest1: public Machine {
	bool moving;
	byte nextSample;

public:
	void goToNextSample() {
		nextSample++;
		enableCarr();
		moving = true;
		goToSample(nextSample);
		lcd.setCursor(10, 1);
		lcd.print(nextSample);
	}

	void begin() {
		const char *title = "Carrousel Test 1";
		Serial.println(title);

		lcd.clear();
		lcd.print(title);
		lcd.setCursor(0, 1);
		lcd.print("Going to #");
		nextSample = fc.currSample;
		goToSample(nextSample, false);
		goToNextSample();
	}

	void middle() {
		if (moving) {
			bool a = carrMotor.run();
			bool b = axisMotor.run();
			if (!a and !b) {
				disableCarr();
				moving = false;
				fc.currSample = nextSample;
				if (nextSample < MAX_SAMPLES)
					countdown(1000);
			}
		}

		if (timer()) goToNextSample();
	}
};

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

class Alignment: public Machine {
	bool moveCarr;
	bool moveAxis;
	long prevPos;

public:
	void begin() {
		Serial.println("Alignment");
		lcd.setCursor(0, 0); lcd.print("Use UD to align ");
		lcd.setCursor(0, 1); lcd.print("sample #1 w/axis");
		moveAxis = moveCarr = false;
	}

	void middle() {
		if (moveCarr) carrMotor.runSpeed();
		else if (moveAxis) axisMotor.runSpeed();

		if (buttons.isPressed()) {
			byte btn = buttons.button;
			if (btn == UP or btn == DOWN) {
				const float speed = 200.0;
				moveCarr = true;
				prevPos = carrMotor.currentPosition();
				carrMotor.setSpeed((btn == UP)? speed: -speed);
				enableCarr();
			}
			else if (btn == LEFT or btn == RIGHT) {
				const float speed = 100.0;
				moveAxis = true;
				prevPos = axisMotor.currentPosition();
				axisMotor.setSpeed((btn == LEFT)? speed: -speed);
			}
			else {
				carrMotor.setCurrentPosition(0);
				axisMotor.setCurrentPosition(0);
				next();
			}
		}
		else if (buttons.isReleased()) {
			long currPos = (moveCarr)?
				carrMotor.currentPosition():
				axisMotor.currentPosition();
			Serial.print(currPos - prevPos);
			Serial.println(" steps");
			moveAxis = moveCarr = false;
			disableCarr();
		}
	}
};

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

class Zeroing: public Machine {
	byte screen;
	char x1, x2;
	const char xi=9;

public:
	void animate() {
		if (x1 >= xi) lcd_printAt(x1, 0, ' ');
		if (x2 >= xi) lcd_printAt(x2, 0, '.');
		if (++x1 > xi+3) x1 = xi-5;
		if (++x2 > xi+3) x2 = xi-5;
	}

	void begin() {
		Serial.println("Zeroing");
		screen = 0;
		lcd.clear();
		lcd.print(" Ready to zero?");
		x1 = xi-3;
		x2 = xi;
	}

	void middle() {
		if (screen) {
			int ls = digitalRead(LIMIT_SWITCH_PIN);
			if (ls == LOW or buttons.isPressed()) {
				axisMotor.setCurrentPosition(0);
				next();
				return;
			}
			if (timer()) animate();
			// axisMotor.run();
			axisMotor.runSpeed();
		}
		else if (buttons.isPressed()) {
			screen++;
			lcd.clear();
			lcd.print("  Zeroing");
			// axisMotor.move(-1000*AXIS_STEPS_PER_MM);
			axisMotor.setSpeed(-200.0);
			repeat(200);
		}
	}
};

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
class Splash: public Machine {
public:
	void begin() {
		Serial.println("Splash");
		// First screen
		lcd.setCursor(0, 0);
		lcd.print("Kellyzador " VERSION);
		lcd.write(ICON_SMILEY);
		lcd.setCursor(0, 1);
		lcd.print("By Nathan/Arley");
		digitalWrite(BEEPER_PIN, LOW);
		countdown(200);
	}

	void middle() {
		axisMotor.run();
		if (buttons.isPressed()) {
			switch (buttons.button) {
				case LEFT: axisMotor.move(NEMA17_STEPS); break;
				case RIGHT: axisMotor.move(-NEMA17_STEPS); break;
				default: axisMotor.setCurrentPosition(0); next();
			}
		}
		if (timer()) digitalWrite(BEEPER_PIN, HIGH);
	}
};

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬

void setup() {
	Serial.begin(115200);
	// Configure the pins
	pinMode(LIMIT_SWITCH_PIN, INPUT_PULLUP);
	pinMode(BEEPER_PIN, OUTPUT);
	noBeep();
	pinMode(BYJ48_ENABLE_PIN, OUTPUT);
	disableCarr();
	// carrMotor.setEnablePin(BYJ48_ENABLE_PIN);
	// carrMotor.setPinsInverted(false, false, true);
	// carrMotor.enableOutputs();

	// LCD Initialization
	lcd.begin(16, 2);
	init_icons(lcd);
	lcd.clear();
	
	// Configure the motors' speed and acceleration
	carrMotor.setMaxSpeed(BYJ48_MAX_SPEED);
	axisMotor.setMaxSpeed(NEMA17_MAX_SPEED);
	carrMotor.setAcceleration(BYJ48_MAX_SPEED/5);
	axisMotor.setAcceleration(NEMA17_MAX_SPEED/5);

	// MACHINE STATES are created from last to first
	// so that they know which one to go next
	Machine
		*over = new Over,
		*experiment = new Experiment,
		*settings = new Settings,
		// *speedTest = new SpeedTest,
		*alignment = new Alignment,
		*zeroing = new Zeroing,
		*splash = new Splash;

	// from the last state, go back to settings menu
	over->setNext(zeroing);

	// Test each sample's positionning
	// Machine *carrTest = new CarrouselTest1;
	// alignment->setNext(carrTest);  // test

	// Machine::ChangeTo(splash);
	Machine::ChangeTo(alignment);  // fast jump
}

//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
void loop() {
	buttons.check();
	Machine::Run();
}