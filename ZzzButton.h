/***************************************************************
  ZzzButton.h
***************************************************************/
#ifndef ZZZ_BUTTON_H
#define ZZZ_BUTTON_H

/* Use define since ZzzButton is a template class (ZzzButton::STATE_PRESS result in compile errors) */
#define ZZZ_BUTTON_STATE_PRESS       1
#define ZZZ_BUTTON_STATE_PRESS_LONG  2
#define ZZZ_BUTTON_STATE_RELEASE     4

/* Default values */
#define ZZZ_BUTTON_DEFAULT_INTERVAL_US     5000

#define ZZZ_BUTTON_DEFAULT_DEBOUNCE_MS     50
#define ZZZ_BUTTON_DEFAULT_LONG_PRESS_MS   1000


/*
 TODO keypad matrix over i2c (ie: pcf8574)
 TODO i2c controlled buttons driver (M5Stack PbHub B, pcf8574...)
*/

/** Callback to receive button change notifications */
typedef void(*ZzzButtonCallback)(size_t buttonIndex, unsigned int buttonState);

/** Abstract button driver */
template <size_t SIZE=1, int INTERVAL_US=ZZZ_BUTTON_DEFAULT_INTERVAL_US> class ZzzButtonDriver {
	public:
		ZzzButtonDriver() {
		}
		
		/** Number of buttons managed by this driver */
		size_t size() {
			return SIZE;
		}

		/** Return the interval in microseconds between two state requests to avoid too much requests. */
		unsigned long getIntervalUs() {
			return INTERVAL_US;
		}

		/** Return buttons states. Bitmask of all managed buttons. 1 is pressed, 0 is released. */
		unsigned long getPressedStates();
};


/**
 * Driver for a single classic button.
 * @param PIN specify the pin where the button is connected.
 * @param PIN_MODE Default PIN_MODE=INPUT_PULLUP is for internal pull up: One side of the button is connected to the given pin the other side to GND
 *         PIN_MODE=INPUT for external pull up: One side of the button is connected to a voltage divider (two resistors) the other side to GND
 * @param PRESS_VALUE Default PRESS_VALUE=LOW
 */
template <int PIN, int PIN_MODE=INPUT_PULLUP, int PRESS_VALUE=LOW, int INTERVAL_US=ZZZ_BUTTON_DEFAULT_INTERVAL_US> class ZzzButtonDriverPin : public ZzzButtonDriver<1, INTERVAL_US> {
	public:
		ZzzButtonDriverPin() {
			pinMode(PIN, PIN_MODE);
		}


		/** Return buttons states. Bitmask of all managed buttons. 1 is pressed, 0 is released. */
		unsigned long getPressedStates() {
			return (digitalRead(PIN)==PRESS_VALUE) ? 1 : 0;
		}
};


/**
 * Driver for multiple buttons. Maximum is unsigned long bit size (64 buttons in general) 
 * //size_t NB_PINS 
 */
template <int PIN_MODE, int PRESS_VALUE, int INTERVAL_US, int ... PINS> class ZzzButtonDriverMultiPins : public ZzzButtonDriver<sizeof...(PINS), INTERVAL_US>  {
	protected:
		const int _pins[sizeof...(PINS)]={PINS...};
	public:
		ZzzButtonDriverMultiPins() {
			for (size_t i=0;i<sizeof...(PINS);i++){
				pinMode(_pins[i], PIN_MODE);
			}
		}

		/** Return buttons states. Bitmask of all managed buttons. 1 is pressed, 0 is released. */
		unsigned long getPressedStates() {
			unsigned long result=0;
			for (size_t i=0;i<sizeof...(PINS);i++){
				if ((digitalRead(_pins[i])==PRESS_VALUE)) {
					result|=(1<<i);
				}
			}
			return result;
		}
};


/**
 * FIXME Test
 * Driver to connect multiple buttons on a single analog pin. (For wiring example check this https://www.instructables.com/How-to-Multiple-Buttons-on-1-Analog-Pin-Arduino-Tu/)
 * Could also be used to trigger threshold on an analog sensor.
 * PIN the number of the analog input pin to read from
 * MARGIN margin to consider the values as valid (ie: 50)
 * VALUES to identify pressed buttons based on analogRead returnes value (0-1023). (ie: 100, 300)
 */
template <int PIN, int INTERVAL_US, int MARGIN, int ... VALUES> class ZzzButtonDriverAnalog : public ZzzButtonDriver<sizeof...(VALUES), INTERVAL_US>  {
	protected:
		const int _values[sizeof...(VALUES)]={VALUES...};

	public:
		ZzzButtonDriverAnalog() {
		}

		/** Return buttons states. Bitmask of all managed buttons. 1 is pressed, 0 is released. */
		unsigned long getPressedStates() {
			int analogValue=analogRead(PIN);
			for (size_t i=0;i<sizeof...(VALUES);i++ ){
				int diff=(analogValue-_values[i]);
				if ((diff>(-MARGIN)) && (diff<MARGIN)) {
					return (1<<i); //Only one value can be detected
				}
			}
			return 0;
		}
};


/**
 * Driver to connect a matrix keypad.
 * NB_ROWS The number of rows the keypad have
 * PINS the number of the pin connected to the keypad. Rows first then columns
 * 	Example for a 3x4 keypad: ZzzButtonDriverKeyPadMatrix<5000, 3, PIN_ROW_0, PIN_ROW_1, PIN_ROW_2, PIN_COL_0, PIN_COL_1, PIN_COL_2, PIN_COL_3>
 * Buttons index for 2x3 2 rows:  0 1 2
 *                                3 4 5
 */
template <int INTERVAL_US, int NB_ROWS, int ... PINS> class ZzzButtonDriverKeyPadMatrix : public ZzzButtonDriver<NB_ROWS * (sizeof...(PINS)-NB_ROWS), INTERVAL_US>  {
	protected:
		const int _pins[sizeof...(PINS)]={PINS...};

	public:
		ZzzButtonDriverKeyPadMatrix() {
		    for (int r=0; r<NB_ROWS; r++) {
			pinMode(_pins[r], INPUT_PULLUP);
		    }
		}

		/** Return buttons states. Bitmask of all managed buttons. 1 is pressed, 0 is released. */
		unsigned long getPressedStates() {
			unsigned long result=0;
			for (int c=NB_ROWS; c<sizeof...(PINS); c++) {
				pinMode(_pins[c], OUTPUT);
				digitalWrite(_pins[c], LOW);
				for (int r=0; r<NB_ROWS; r++) {
					if (digitalRead(_pins[r])==LOW) { //press detected: set correct button index to 1
						bitWrite(result, NB_ROWS*(c-NB_ROWS) + r, 1);
					}
				}
				digitalWrite(_pins[c], HIGH);
				pinMode(_pins[c], INPUT);
			}
			return result;
		}
};


/**
 * FIXME Test
 * Driver for multiple drivers
 * NB_BUTTONS is the number of pins managed by all given drivers (might differ from number for drivers given)
 */
template <typename DRIVER1, typename DRIVER2, size_t NB_BUTTONS=2, int INTERVAL_US=ZZZ_BUTTON_DEFAULT_INTERVAL_US> class ZzzButtonDriverMulti2
		: public ZzzButtonDriver<NB_BUTTONS, INTERVAL_US> {
	protected:
		DRIVER1 _driver1;
		DRIVER2 _driver2;

	public:
		ZzzButtonDriverMulti2() {
		}

		/** Return buttons states. Bitmask of all managed buttons. 1 is pressed, 0 is released. */
		unsigned long getPressedStates() {
			unsigned long result=_driver1.getPressedStates();
			size_t offset=_driver1.size();

			unsigned long pressedStates=_driver2.getPressedStates();
			if (pressedStates>0) {
				result|=(offset<<pressedStates);
			}
			return result;
		}
};


/**
 * FIXME Test
 * Driver for multiple drivers
 * NB_BUTTONS is the number of pins managed by all given drivers (might differ from number for drivers given)
 */
template <typename DRIVER1, typename DRIVER2, typename DRIVER3, size_t NB_BUTTONS=3, int INTERVAL_US=ZZZ_BUTTON_DEFAULT_INTERVAL_US> class ZzzButtonDriverMulti3
		: public ZzzButtonDriver<NB_BUTTONS, INTERVAL_US> {
	protected:
		DRIVER1 _driver1;
		DRIVER2 _driver2;
		DRIVER2 _driver3;

	public:
		ZzzButtonDriverMulti3() {
		}

		/** Return buttons states. Bitmask of all managed buttons. 1 is pressed, 0 is released. */
		unsigned long getPressedStates() {
			unsigned long result=_driver1.getPressedStates();
			size_t offset=_driver1.size();

			unsigned long pressedStates=_driver2.getPressedStates();
			if (pressedStates>0) {
				result|=(offset<<pressedStates);
			}
			offset+=_driver2.size();

			pressedStates=_driver3.getPressedStates();
			if (pressedStates>0) {
				result|=(offset<<pressedStates);
			}
			return result;
		}
};


/**
 * Template class to manage a button or a set of buttons. The template need a Driver parameter to check the buttons states.
 * The driver class must implement getPressedStates(), size() and getStepUs().
 */
template <typename DRIVER> class ZzzButton {
	protected:
		unsigned long _debounceMs;
		unsigned long _longPressMs;

		/** Interval between two driver states requests */
		unsigned long _intervalUs;
		unsigned long _lastRequestUs=0;

		/** Intermediate states to debounce */		
		unsigned long _lastStates=0;
		unsigned long _lastStatesMs=0;

		/** Last notified states */
		unsigned long _lastNotifiedStates=0;
		unsigned long _lastNotifiedStatesMs=0;

		/** To detect clic. Clic and double clic can only be detected one button at a time. */
		size_t lastClicButtonIndex=0;
		unsigned long lastClicMs=0;

		DRIVER _driver;

		/** Callback called on status change */
		ZzzButtonCallback _callback=nullptr;

	public:
		static const int STATE_PRESS=ZZZ_BUTTON_STATE_PRESS;
		static const int STATE_PRESS_LONG=ZZZ_BUTTON_STATE_PRESS_LONG;
		static const int STATE_RELEASE=ZZZ_BUTTON_STATE_RELEASE;

		/** Set the callback to call on each button state change. */
		void setCallback(ZzzButtonCallback callback) {
			_callback=callback;
		}

		/** Number of buttons managed */
		size_t size() {
			return _driver.size();
		}

		/**
		 * Return true if specified button is pressed.
		 * Default (if no parameter) check first button.
		 * Result is based on internal state (no direct verification, and debounced)
		 */
		bool isPressed(size_t buttonIndex=0) {
			unsigned long bitMask=(1<<buttonIndex);
			if ((_lastNotifiedStates & bitMask)!=0) {
				return true;
			}
			return false;
		}
	
		/** To call frequently (ie: in Arduino loop) */
		void update() {
			//check elapsed time (overflow proof)
			if (micros() - _lastRequestUs > _intervalUs) {
				unsigned long newStates=_driver.getPressedStates();
				_lastRequestUs=micros();

				if (newStates!=_lastStates) { //For debounce need at least 2 successive identical state
					_lastStates=newStates;
					_lastStatesMs=millis();
				} else {
					if (_lastStates!=_lastNotifiedStates) {
						//debounce
						if (millis() - _lastStatesMs > _debounceMs) {
							//set notified state before callback calls in case status (isPressed()) is requested in the callback
							unsigned long oldStates=_lastNotifiedStates;
							_lastNotifiedStates=_lastStates;
							_lastNotifiedStatesMs=millis();	
							//check what changed
							for (size_t i=0;i<_driver.size();i++) {
								unsigned long bitMask=(1<<i);
								if ((oldStates & bitMask)!=(_lastStates & bitMask)) {
									int state=(_lastStates & bitMask)!=0 ? STATE_PRESS : STATE_RELEASE;
									if (_callback!=nullptr) {
										_callback(i, state);
									}
									if (state==STATE_RELEASE) {
										lastClicButtonIndex=i;
										lastClicMs=millis();
									}
								}
							}
						}
					} else { //check long press
						//at least one button is pressed
						if (_lastNotifiedStates > 0) {
							if (millis() - _lastNotifiedStatesMs > _longPressMs) {
								if (_callback!=nullptr) {
									for (size_t i=0;i<_driver.size();i++) {
										unsigned long bitMask=(1<<i);
										if ((_lastNotifiedStates & bitMask)!=0) {
											_callback(i, STATE_PRESS_LONG);
										}
									}
								}
								_lastNotifiedStatesMs=millis();
							}
						}
					}
				}
			}
		}

		/** Constructor */
		ZzzButton(unsigned long debounceMs=ZZZ_BUTTON_DEFAULT_DEBOUNCE_MS, unsigned long longPressMs=ZZZ_BUTTON_DEFAULT_LONG_PRESS_MS) {
			_intervalUs=_driver.getIntervalUs();
			_debounceMs=debounceMs;
			_longPressMs=longPressMs;
		}
};

#endif

