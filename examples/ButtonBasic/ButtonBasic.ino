#include <ZzzButton.h>

ZzzButton button(18);

/*
//Single pin button
ZzzButtonDriverPin<INPUT, LOW> buttonDriverPin(18);

//Multiple buttons: 2 buttons in INPUT mode and LOW=press on PIN 18 and PIN 19
ZzzButtonDriverMultiPins<INPUT, LOW, 18,19> buttonDriverMultiPins;

ZzzButton button(buttonDriver);
*/


void buttonChanged(size_t buttonIndex, unsigned int buttonState) {
	Serial.print("Button change: #");
	Serial.print(buttonIndex);
	Serial.print(" state: ");
  switch(buttonState) {
    case ZzzButton::STATE_PRESS:
	    Serial.print("PRESS");
      break;
    case ZzzButton::STATE_PRESS_LONG:
      Serial.print("PRESS LONG");
      break;
    case ZzzButton::STATE_RELEASE:
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
