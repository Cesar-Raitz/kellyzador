//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
// AnalogButtons.h - A class to deal with analog buttons present in the LCD
// 	Keypad Arduino shield. It is actually quite similar to the AnalogButtons
//    library on the Arduino repository.
//
// Author: Cesar Raitz
// Date: Sep. 2, 2024
//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
#define SAMPLING_INTERVAL 20    // ms

#define PRESS    1
#define RELEASE  2
#define HOLD     3

#define RIGHT    1
#define UP       2
#define DOWN     3
#define LEFT     4
#define SELECT   5

typedef void (*ButtonHandler)(uint8_t, uint8_t);

#ifdef USE_BUTTON_SNITCH
	void buttonSnitch(uint8_t event, uint8_t button) {
		char *names[] = {"Right", "Up", "Down", "Left", "Select"};
		Serial.print(names[button-1]);
		if (event == PRESS)  Serial.println(" pressed");
		else if (event == RELEASE) Serial.println(" released");
		else if (event == HOLD) Serial.println(" held");
	}
#else
	ButtonHandler buttonSnitch = nullptr;
#endif

class AnalogButtons {
	uint8_t analogPin;
	uint32_t time = 0;

	uint8_t pressed = 0;
	uint8_t current = 0;
	uint8_t counter = 0;
	uint8_t holdCounter = 0;

	ButtonHandler handler = buttonSnitch; 

public:
	byte event = 0,
	     button = 0;

	AnalogButtons(uint8_t analogPin) {
		this->analogPin = analogPin;
	}

	void setHandler(ButtonHandler handler) {
		this->handler = handler;
	}

	void check() {
		uint32_t currTime = millis();
		if (currTime - time > SAMPLING_INTERVAL) {
			time = currTime;
			int val = analogRead(analogPin);
			uint8_t btn = 0;
			// if (val < 50) btn = 1;       // right
			// else if (val < 200) btn = 2; // up
			// else if (val < 350) btn = 3; // down
			// else if (val < 600) btn = 4; // left
			// else if (val < 700) btn = 5; // select
			
			if (val < 80) btn = 1;          // right
			else if (val < 200) btn = 2;    // up
			else if (val < 300) btn = 3;    // down
			else if (val < 500) btn = 4;    // left
			else if (val < 700) btn = 5;    // select

			if (pressed == 0) {
				// Test for a button being pressed
				if (btn) {
					if (btn != current) {
						current = btn;
						counter = 1;
					}
					else if (++counter == 3) {
						event=PRESS; button=current;
						if (handler) handler(PRESS, current);
						pressed = current;
						holdCounter = 0;
					}
				}
			}
			else {
				// Test for a button being held
				if (++holdCounter == 10) {
					event=HOLD; button=pressed;
					if (handler) handler(HOLD, pressed);
					holdCounter = 0;
				}

				// Test for a button being relesed
				if (current != pressed) {
					if (++counter == 2) {
						event=RELEASE; button=pressed;
						if (handler) handler(RELEASE, pressed);
						pressed = 0;
						return;
					}
				}

				if (btn != current) {
					current = btn;
					counter = 1;
				}
			}
		}
	}
	
	bool isPressed() {
		if (event == PRESS) {
			event = 0;
			return true;
		}
		return false;
	}

	bool isReleased() {
		if (event == RELEASE) {
			event = 0;
			return true;
		}
		return false;
	}
};