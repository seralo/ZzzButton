# ZzzButton
Simple Arduino / C++ library to control buttons


The library consist of a single header file (ZzzButton.h) containing template classes.


### Constructor

```cpp
ZzzButton <DRIVER> button(debounceMs=50, longPressMs=1000); //Constructor need a driver class as template param
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

ZzzButton < ZzzButtonDriverPin<1> > button;

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

