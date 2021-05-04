#include <ZzzButton.h>

//3x4 keypad
#define MY_NB_ROWS   3

#define MY_PIN_ROW_0 17
#define MY_PIN_ROW_1 25
#define MY_PIN_ROW_2 26
#define MY_PIN_COL_0 16
#define MY_PIN_COL_1 27
#define MY_PIN_COL_2 12
#define MY_PIN_COL_3 14

ZzzButtonDriverKeyPadMatrix<MY_NB_ROWS,
   MY_PIN_ROW_0, MY_PIN_ROW_1, MY_PIN_ROW_2,
   MY_PIN_COL_0, MY_PIN_COL_1, MY_PIN_COL_2, MY_PIN_COL_3
  > buttonDriver;

/* */


#include <Wire.h>

#define MY_NB_COLS   4
ZzzButtonDriverI2CKeyPadPCF8574<MY_NB_ROWS, MY_NB_COLS, TwoWire> buttonDriver2(&Wire);

ZzzButton button(buttonDriver);

void buttonChanged(size_t buttonIndex, unsigned int buttonState) {
  Serial.print("Button change: #");
  Serial.print(buttonIndex);

  Serial.print(" Row:");
  Serial.print(buttonIndex % MY_NB_ROWS);
  Serial.print(" Col:");
  Serial.print(buttonIndex / MY_NB_ROWS);

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
  }
  Serial.println();
}

void setup() {
	Serial.begin(115200);
	delay(250); //to ensure correctly initialized

	Serial.println("Button keypad demo");

  Serial.print("Keypad rows:");
  Serial.print(MY_NB_ROWS);
  int nbButtons=button.size();
  Serial.print("x");
  Serial.print(nbButtons/MY_NB_ROWS);
  Serial.print("=");
  Serial.print(nbButtons);

	button.setCallback(buttonChanged);
}

void loop() {
	button.update();
}
