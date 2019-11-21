/*
  Pass images from processing to thermal printer.

  For https://www.adafruit.com/product/2751

  Install Adafruit Thermal Printer

  Edit Adafruit_Thermal.cpp, change baudrate to value on printout?

  Protocol:
    -> "Thermal\r" # at startup, arduino is ready for first image. nb \r

    -> I # ready for image
    <- WnnnHnnnn\n # height and width of image in hex
    -> R # ready for 1st row
    <- nn...\n # a row of hex bytes
    -> R # ready for next row
    <- nn...\n # a row of hex bytes
    ...
    <- nn...\n # a row of hex bytes
    -> X # last row seen

    All other strings are debugging and should be ignored/shown-on-console

*/

#include "tired_of_serial.h"
#include "Adafruit_Thermal.h"

const int SerialBaud = 115200;
const String ArduinoReady = "Thermal\r"; // for next image
const char ReadyForRow = 'R'; // each row
const char ReadyForImage = 'I';
const char ImageAccepted = 'X'; // at end of image

enum States { InStartImage, InReadWH, 
  InReadW, InReadWidth, InReadH, InReadHeight, InReadWHEOL, 
  InErrorStop
  };

States state = InStartImage;

void setup() {
  Serial.begin(SerialBaud);
  println("start"); // make non-protocol all lower case

  print(ArduinoReady);
  }

#define next_state( f, failstate, okstate ) \
      switch(f) { \
        case -1: \
          state = failstate; \
          break; \
        case 0: \
          break; /* no change */ \
        case 1: \
          state = okstate; \
      }

void loop() {
  static int height;
  static int width;

  switch(state) {

    case InStartImage:
      next_state( start_image(), InErrorStop, InReadWH );
      break;
    
    // WnnnHnnnn\n
    case InReadWH :
      state = InReadW;

    case InReadW :
      next_state( expect_char('W'), InErrorStop, InReadWidth );
      break;

    case InReadWidth :
      next_state( expect_hex(3, width), InErrorStop, InReadH );
      break;

    case InReadH :
      next_state( expect_char('H'), InErrorStop, InReadHeight );
      break;

    case InReadHeight :
      next_state( expect_hex(4, height), InErrorStop, InReadWHEOL );
      break;

    case InErrorStop :
      // FIXME
      break;
  }

  // FIXME: print time for state
}

// Functions for states
// Return 0 to stay in the state
// Return 1 to signal done with state
// Return -1 to signal state failed

int start_image() {
  // FIXME: reset,...
  print(ReadyForImage);
  return 1;
}

int expect_char(char which) {
  return 1;
  }

int expect_hex(int digits, int &value) {
  // writes to the value so you can use this in the state-machine style
  return 1;
}


