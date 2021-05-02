/***************************************************************
  ZzzButton.h
***************************************************************/
#ifndef ZZZ_BUTTON_H
#define ZZZ_BUTTON_H

/* Use define since ZzzButton is a template class (ZzzButton::STATE_PRESS result in compile errors) */
#define ZZZ_BUTTON_STATE_PRESS       1
#define ZZZ_BUTTON_STATE_PRESS_LONG  2
#define ZZZ_BUTTON_STATE_RELEASE     4


/*
 TODO analog buttons driver
 TODO matrix buttons driver
 TODO i2c controlled buttons driver (M5Stack PbHub B, pcf8574...)
*/

/** Callback to receive button change notifications */
typedef void(*ZzzButtonCallback)(size_t buttonIndex, unsigned int buttonState);

/** Abstract template button driver */
template <size_t SIZE=1, int INTERVAL_US=5000> class ZzzButtonDriver {
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
};


/**
 * Driver for a single classic button.
 * @param PIN specify the pin where the button is connected.
 * @param PIN_MODE Default PIN_MODE=INPUT_PULLUP is for internal pull up: One side of the button is connected to the given pin the other side to GND
 *         PIN_MODE=INPUT for external pull up: One side of the button is connected to a voltage divider (two resistors) the other side to GND
 * @param PRESS_VALUE Default PRESS_VALUE=LOW
 */
template <int PIN, int PIN_MODE=INPUT_PULLUP, int PRESS_VALUE=LOW> class ZzzButtonDriverPin : public ZzzButtonDriver<1> {
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
 * Driver for multiple pull up buttons. Maximum is unsigned long bit size (64 buttons in general)  
 */
template <size_t NB_PINS, int PIN_MODE, int PRESS_VALUE, int ... PINS> class ZzzButtonDriverMultiPins : public ZzzButtonDriver<NB_PINS> {
	protected:
		const int _pins[NB_PINS]={PINS...};
	public:
		ZzzButtonDriverMultiPins() {
			for (size_t i=0;i<NB_PINS;i++){
				pinMode(_pins[i], PIN_MODE);
			}
		}

		/** Return buttons states. Bitmask of all managed buttons. 1 is pressed, 0 is released. */
		unsigned long getPressedStates() {
			unsigned long result=0;
			for (size_t i=0;i<NB_PINS;i++){
				if ((digitalRead(_pins[i])==PRESS_VALUE)) {
					result|=(1<<i);
				}
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
		ZzzButton(unsigned long debounceMs=50, unsigned long longPressMs=1000) {
			_intervalUs=_driver.getIntervalUs();
			_debounceMs=debounceMs;
			_longPressMs=longPressMs;
		}
};

#endif

