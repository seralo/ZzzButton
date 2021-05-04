# ZzzButton
Simple Arduino / C++ library to control buttons


The library consist of a single header file (ZzzButton.h) containing template classes.

Supported features:
- Debounce
- Long press
- Callback
- Multiple button types support

The library can adapt to multiple buttons type using drivers.

Supported drivers:
- PIN (Internal pull up, External pull up, Logic inverted)
- Multiple PINS (Manage several PINS simultaneously to avoid parallel polling and limit resources)
- Keypad matrix (Flexible layout: 3x4, 4x4...)
- Analog buttons (Not fully tested)
- Keypad over I2C with PCF8574 (Not fully tested)
- Multiple drivers (Not fully tested)


To be implemented:
- "Pin" buttons over I2C M5Stack PbHub
- "Pin" buttons over I2C PCF8574

### Constructor

```cpp
ZzzButton button(pin, debounceMs=50, longPressMs=1000); //Simple mode with a single PIN in INPUT_PULLUP

ZzzButtonDriverMultiPins<INPUT, LOW, pin1, pin2> buttonDriver; //Define driver to manage 2 buttons on 2 pins in INPUT mode with press=LOW
ZzzButton button(buttonDriver, debounceMs=50, longPressMs=1000); //Instance to manage buttons
```

### Functions

```cpp
void update();        // To call in Arduino loop
void size();          // Number of buttons managed
void isPressed(buttonIndex=0); // Return true if specified button is pressed.
                               // Result is based on internal state (no direct verification, and debounced)
void setCallback();   // Set the callback to call on each button state change.
                      // (ie: void buttonChanged(size_t buttonIndex, unsigned int buttonState) )
```

### Included examples

- `ButtonBasic/ButtonBasic.ino` - Show basic usage with multiple buttons


### Simple code example for simple input pullup button on PIN 1

```cpp
#include <ZzzButton.h>

ZzzButton button(1);

void buttonChanged(size_t buttonIndex, unsigned int buttonState)
{

    ...

}

void setup()
{
    ...
    
    button.setCallback(buttonChanged);

    ...
}

void loop()
{
    ...

    button.update();

    ...
}
```

