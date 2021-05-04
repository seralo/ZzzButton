#include <ZzzButton.h>

//Analog pin to use
#define MY_ANALOG_PIN  18

//Check value every 10000 us=10 ms=0.01 seconds
#define MY_INTERVAL    10000
#define MY_MARGIN      50
//Multiple buttons connected to Analog pin
//Define 3 buttons at values: 300, 500, 900
//ZzzButton < ZzzButtonDriverAnalog<MY_ANALOG_PIN, MY_INTERVAL, MY_MARGIN, 300, 500, 900> > button;

//For debug: Single button with high margin to detect all changes between (23 and 1023)
#define MY_MARGIN_DEBUG   500
ZzzButton < ZzzButtonDriverAnalog<MY_ANALOG_PIN, MY_INTERVAL, MY_MARGIN_DEBUG, 523> > button;


void buttonChanged(size_t buttonIndex, unsigned int buttonState) {
  Serial.print("Button change: #");
  Serial.print(buttonIndex);
  Serial.print(" state: ");
  switch(buttonState) {
    case ZZZ_BUTTON_STATE_PRESS:
      Serial.print("PRESS");
      break;
    case ZZZ_BUTTON_STATE_PRESS_LONG:
      Serial.print("PRESS LONG");

      Serial.print(" Read value=");
      Serial.print(analogRead(MY_ANALOG_PIN));
      
      break;
    case ZZZ_BUTTON_STATE_RELEASE:
      Serial.print("RELEASE");
      break;
    default:
      Serial.print(buttonState);
      break;
  }

  Serial.print(" Pressed states:");
  for (size_t i=0;i<button.size();i++) {
    Serial.print(button.isPressed(i));
    Serial.print(",");    
  }

  Serial.println();
}

void setup() {
	Serial.begin(115200);
	delay(250); //to ensure correctly initialized

	Serial.println("Button demo");

	button.setCallback(buttonChanged);
}

void loop() {
	button.update();
}
