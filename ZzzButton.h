/***************************************************************
  ZzzButton.h
***************************************************************/
#ifndef ZZZ_BUTTON_H
#define ZZZ_BUTTON_H

#define ZZZ_DEFAULT_PCF8574_ADDRESS   0x20
#define ZZZ_DEFAULT_PCF8574A_ADDRESS  0x38

/*
 TODO i2c controlled buttons driver (M5Stack PbHub B, pcf8574...)
*/

/** Callback to receive button change notifications */
typedef void(*ZzzButtonCallback)(size_t buttonIndex, unsigned int buttonState);

/** Abstract button driver. Class to override to implement a new button driver */
class ZzzButtonDriver {
	protected:
		ZzzButtonDriver() {
		}
	public:
		/** Number of buttons managed by this driver */
		virtual size_t size() const;

		/** Return buttons states. Bitmask of all managed buttons. 1 is pressed, 0 is released. */
		virtual unsigned long getPressedStates();
};


/**
 * Driver for a single classic button.
 * @param PIN_MODE Default PIN_MODE=INPUT_PULLUP is for internal pull up: One side of the button is connected to the given pin the other side to GND
 *         PIN_MODE=INPUT for external pull up: One side of the button is connected to a voltage divider (two resistors) the other side to GND
 * @param PRESS_VALUE Default PRESS_VALUE=LOW
 */
template <int PIN_MODE=INPUT_PULLUP, int PRESS_VALUE=LOW> class ZzzButtonDriverPin : public ZzzButtonDriver {
	protected:
		int _pin;
	public:
		/** @param pin specify the pin where the button is connected. */
		ZzzButtonDriverPin(int pin) {
			_pin=pin;
			pinMode(_pin, PIN_MODE);
		}

		virtual size_t size() const final override {
			return 1;
		}

		virtual unsigned long getPressedStates() override {
			return (digitalRead(_pin)==PRESS_VALUE) ? 1 : 0;
		}
};


/**
 * Driver for multiple buttons. Maximum is unsigned long bit size (64 buttons in general) 
 */
template <int PIN_MODE, int PRESS_VALUE, int ... PINS> class ZzzButtonDriverMultiPins : public ZzzButtonDriver {
	protected:
		const int _pins[sizeof...(PINS)]={PINS...};
	public:
		ZzzButtonDriverMultiPins() {
			for (size_t i=0;i<sizeof...(PINS);i++){
				pinMode(_pins[i], PIN_MODE);
			}
		}

		virtual size_t size() const final override {
			return sizeof...(PINS);
		}

		virtual unsigned long getPressedStates() override {
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
template <int PIN, int MARGIN, int ... VALUES> class ZzzButtonDriverAnalog : public ZzzButtonDriver {
	protected:
		const int _values[sizeof...(VALUES)]={VALUES...};

	public:
		ZzzButtonDriverAnalog() {
		}

		virtual size_t size() const final override {
			return sizeof...(VALUES);
		}

		virtual unsigned long getPressedStates() override {
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
template <uint8_t NB_ROWS, int ... PINS> class ZzzButtonDriverKeyPadMatrix : public ZzzButtonDriver {
	protected:
		const int _pins[sizeof...(PINS)]={PINS...};

	public:
		ZzzButtonDriverKeyPadMatrix() {
		    for (int r=0; r<NB_ROWS; r++) {
			pinMode(_pins[r], INPUT_PULLUP);
		    }
		}

		virtual size_t size() const final override {
			return NB_ROWS * (sizeof...(PINS)-NB_ROWS);
		}

		virtual unsigned long getPressedStates() override {
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
 * Driver to connect a matrix keypad through PCF8574 using I2C protocol
 * @param ADDRESS I2C address to access PCF8574 connected to the keypad
 * @param WIRE template parameter allow to define custom Wire library
 * @param NB_ROWS The number of rows the keypad have
 * @param NB_COLS The number of cols the keypad have
 *
 * Assuming the keypad is connected to the PCF8574 as follow: PIN0=ROW0, ... PINx=ROWx, PINx+1=COL0, PINx+y=COLy
 * PCF8574 controls 8 bits so NB_ROWS+NB_COLS<=8 
 * Buttons index for 2x3 2 rows:  0 1 2
 *                                3 4 5
 */
template <uint8_t NB_ROWS, uint8_t NB_COLS, typename WIRE, uint8_t ADDRESS=ZZZ_DEFAULT_PCF8574_ADDRESS> class ZzzButtonDriverI2CKeyPadPCF8574 : public ZzzButtonDriver {
	protected:
		WIRE *_pWire;
		uint8_t _rowMask;
		uint8_t _colMask;

	public:
		ZzzButtonDriverI2CKeyPadPCF8574(void* pParams) {
			_pWire=(WIRE*)pParams;
			_pWire->begin();

			_rowMask=(1<<NB_ROWS)-1; //1: 0b00000001, 2: 0b00000011, ... 5: 0b00011111 (2^5 - 1 = 32 - 1 = 31)
			uint16_t fullMask=(1<<(NB_ROWS+NB_COLS))-1;
			_colMask=(~_rowMask) & fullMask; //Invert mask to request column
		}

		virtual size_t size() const final override {
			return NB_ROWS * NB_COLS;
		}

		virtual unsigned long getPressedStates() override {
			if (_pWire==nullptr) {
				return 0; //need to set wire
			}

			if (NB_ROWS>5) {
				return 0; //not supported
			}

			//Set row to HIGH if press is detected read will return LOW
			_pWire->beginTransmission(ADDRESS);
  			_pWire->write(_rowMask);
  			if (_pWire->endTransmission() != 0) { //communication error
				return 0;
			}
			_pWire->requestFrom(ADDRESS, (uint8_t)1);
			uint8_t rowResponse=_pWire->read();
			
			if (rowResponse==_rowMask) { //no press detected
				return 0;
			}

			//Set col to HIGH if press is detected read will return LOW
			_pWire->beginTransmission(ADDRESS);
  			_pWire->write(_colMask);
  			if (_pWire->endTransmission() != 0) { //communication error
				return 0;
			}
			_pWire->requestFrom(ADDRESS, (uint8_t)1);
			uint8_t colResponse=_pWire->read();

			if (colResponse==_colMask) { //no press detected
				return 0;
			}

			int row=-1; //read row index with LOW value in the response
			for (int r=0;r<NB_ROWS; r++) {
				if (bitRead(rowResponse, r)==0) {
					row=r;
				}
			}
			int col=-1; //read col index with LOW value in the response
			for (int c=0;c<NB_COLS; c++) {
				if (bitRead(colResponse, NB_ROWS+c)==0) {
					col=c;
				}
			}
			if (row==-1 || col==-1) {
				//invalid data
				return 0;
			}

			//Use row and column reads to set correct button ID
			unsigned long pressedState=1 << (row*NB_COLS + col);
			return pressedState;
		}
};


/**
 * FIXME Test
 * Driver for multiple drivers
 * NB_DRIVER is the number of drivers to allocate. It should match constructor call otherwise result is unpredictable. (Max 5 drivers)
 */
template <size_t NB_DRIVER> class ZzzButtonDriverMulti
		: public ZzzButtonDriver {
	protected:
		ZzzButtonDriver *_pDriver[NB_DRIVER];
		size_t _nbButtons;
		void computeButtons() {
			_nbButtons=0;
			for (int i=0;i<NB_DRIVER;i++) {
				_nbButtons+=_pDriver[i]->size();
			}
		}

	public:
		/** At least 2 drivers (use directly the driver if only one driver) */
		ZzzButtonDriverMulti(ZzzButtonDriver &driver1, ZzzButtonDriver &driver2) {
			_pDriver[0]=&driver1;
			_pDriver[1]=&driver2;
			computeButtons();
		}

		ZzzButtonDriverMulti(ZzzButtonDriver &driver1, ZzzButtonDriver &driver2, ZzzButtonDriver &driver3) {
			_pDriver[0]=&driver1;
			_pDriver[1]=&driver2;
			_pDriver[2]=&driver3;
			computeButtons();
		}

		ZzzButtonDriverMulti(ZzzButtonDriver &driver1, ZzzButtonDriver &driver2, ZzzButtonDriver &driver3, ZzzButtonDriver &driver4) {
			_pDriver[0]=&driver1;
			_pDriver[1]=&driver2;
			_pDriver[2]=&driver3;
			_pDriver[3]=&driver4;
			computeButtons();
		}

		ZzzButtonDriverMulti(ZzzButtonDriver &driver1, ZzzButtonDriver &driver2, ZzzButtonDriver &driver3, ZzzButtonDriver &driver4, ZzzButtonDriver &driver5) {
			_pDriver[0]=&driver1;
			_pDriver[1]=&driver2;
			_pDriver[2]=&driver3;
			_pDriver[3]=&driver4;
			_pDriver[4]=&driver5;
			computeButtons();
		}

		virtual size_t size() const final override {
			return _nbButtons;
		}

		virtual unsigned long getPressedStates() override {
			unsigned long result=_pDriver[0]->getPressedStates();
			size_t offset=_pDriver[0]->size();

			for (int i=1;i<NB_DRIVER;i++) {
				unsigned long pressedStates=_pDriver[i]->getPressedStates();
				if (pressedStates>0) { //if at least one press detected update result
					result|=(offset<<pressedStates);
				}
				offset=_pDriver[i]->size();
			}
			return result;
		}
};


/**
 * Template class to manage a button or a set of buttons. The template need a Driver parameter to check the buttons states.
 * The driver class must implement getPressedStates(), size() and getIntervalUs().
 */
class ZzzButton {
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

		ZzzButtonDriver *_pDriver;

		/** Callback called on status change */
		ZzzButtonCallback _callback=nullptr;

	public:
		static const int STATE_PRESS=1;
		static const int STATE_PRESS_LONG=2;
		static const int STATE_RELEASE=4;
		
		/** Default interval in microseconds */
		static const unsigned long DEFAULT_INTERVAL_US=10000;
		/** Default debounce time in milliseconds */
		static const unsigned long DEFAULT_DEBOUNCE_MS=50;
		/** Default long press time in milliseconds */
		static const unsigned long DEFAULT_LONG_PRESS_MS=1000;


		/** Set the callback to call on each button state change. */
		void setCallback(ZzzButtonCallback callback) {
			_callback=callback;
		}

		/** Number of buttons managed */
		size_t size() {
			return _pDriver->size();
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
				unsigned long newStates=_pDriver->getPressedStates();
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
							for (size_t i=0;i<_pDriver->size();i++) {
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
									for (size_t i=0;i<_pDriver->size();i++) {
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

		/** Constructor 
		 * @param driver Underlying instance to access button(s).
		 * @param longPressMs time in milliseconds before long press is notified.
		 * @param debounceMs time in milliseconds to debounce.
		 * @param intervalUs minimum time in microseconds to wait before next driver requests.
		 */
		ZzzButton(ZzzButtonDriver &driver, unsigned long longPressMs=ZzzButton::DEFAULT_LONG_PRESS_MS,
			unsigned long debounceMs=ZzzButton::DEFAULT_DEBOUNCE_MS, unsigned long intervalUs=ZzzButton::DEFAULT_INTERVAL_US) {
			_pDriver=&driver;
			_debounceMs=debounceMs;
			_longPressMs=longPressMs;
			_intervalUs=intervalUs;
		}

		/** Helper for single pin INPUT_PULLUP button */
		ZzzButton(int pin, unsigned long longPressMs=ZzzButton::DEFAULT_LONG_PRESS_MS,
			unsigned long debounceMs=ZzzButton::DEFAULT_DEBOUNCE_MS, unsigned long intervalUs=ZzzButton::DEFAULT_INTERVAL_US) {
			_pDriver=new ZzzButtonDriverPin<>(pin);
			_debounceMs=debounceMs;
			_longPressMs=longPressMs;
			_intervalUs=intervalUs;
		}
};

#endif

