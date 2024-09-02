//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
// Machine.h - A simple class to manage timing events and machine states.
//
// Author: Cesar Raitz
// Date: Sep. 2, 2024
//▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬▬
#ifndef MAX_TIMERS
	#define MAX_TIMERS 2
#endif

enum timerEnum {
	TIMER_1 = 1,
	TIMER_2,
	TIMER_3,
	TIMER_4,
	TIMER_5,
	TIMER_6,
};


struct timerStruct {
	byte isOn = 0;
	bool finished;
	uint32_t next, ms;
} timers[MAX_TIMERS];


class Machine {
	static Machine *current;
	Machine *_next;

public:
	Machine() {
		_next = current;
		current = this;
	}
	virtual void begin() {}
	virtual void middle() {}
	virtual void end() {}

	void setTimer(uint32_t ms, byte num, bool repeat) {
		if (num < 1 or num > MAX_TIMERS) return;
		timerStruct &t = timers[num-1];
		t.ms = ms;
		t.finished = false;
		t.isOn = repeat? 2: 1;
		t.next = millis() + ms;
	}

	int timer() {
		int i = 1;
		for (timerStruct &t: timers) {
			if (t.finished == true) {
				t.finished = false;
				return i;
			}
			i++;
		}
		return 0;
	}

   void endTimer(byte num=0) {
      if (num) timers[num-1].isOn = 0;
      else for (timerStruct &t: timers) {
         t.finished = false;
         t.isOn = 0;
      }
   }

   void repeat(uint32_t ms, byte num=TIMER_1) { setTimer(ms, num, true); }
   void countdown(uint32_t ms, byte num=TIMER_2) { setTimer(ms, num, false); }

	static void Run() {
		// Deal with countdown timer events
		for (timerStruct &t: timers) {
			if (t.isOn) {
				uint32_t time = millis();
				if (time >= t.next) {
					t.finished = true;
					if (t.isOn == 1) t.isOn = 0;
					t.next += t.ms;
				}
			}
		}
		if (current != nullptr) current->middle();
	}

	void setNext(Machine *state) {
		_next = state;
	}

	void next() {
		if (current->_next)
			ChangeTo(current->_next);
	}

	static void ChangeTo(Machine *next) {
		// Cease timing events
		current->endTimer();
      
		// Finalize the previous state
		if (current != nullptr) current->end();
		current = next;

		// Initialize the next state
		next->begin();
	}
};

Machine *Machine::current = nullptr;