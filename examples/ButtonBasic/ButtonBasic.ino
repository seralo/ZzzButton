#include <ZzzButton.h>

//Single pin button
//ZzzButton < ZzzButtonDriverPin<18, INPUT, LOW> > button;

//Multiple buttons: 2 buttons in INPUT mode and LOW=press on PIN 18 and PIN 19
ZzzButton < ZzzButtonDriverMultiPins<2, INPUT, LOW, 18,19> > button;



 mo {
	Serial.print("Button change: #");
	Serial.print(buttonIndex);
	Serial.print(" state: ");
  switch(buttonState) {
    case ZZZ_BUTTON_STATE_PRESS:
	    Serial.print("PRESS");
      break;
    case ZZZ_BUTTON_STATE_PRESS_LONG:
      Serial.print("PRESS LONG");
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
