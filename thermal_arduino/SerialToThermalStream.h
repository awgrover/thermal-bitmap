/*
  Pass images from processing to thermal printer.
    SerialToThermalStream bitmap_transport;
    void setup() {
      Serial.begin(115200); // faster than 19200!
      bitmap_transport.start();
    }

    void loop() {
      ...
      bitmap_transport
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

  Thermal library
    bitmap writes in chunks of 255 (possibly with flow control)
    baud is 19200, which is 1920 chars/sec, which is 0.5 msec/char, i.e. 2 chars/msec
    So, should be good to read from Serial-usb at 115200, and provide a stream
*/

#pragma once

#include "tired_of_serial.h"

class SerialToThermalStream {

  static const char ReadyForRow = 'R'; // each row
  static const char ReadyForImage = 'I';
  static const char ImageAccepted = 'X'; // at end of image

  enum States { 
    InStartImage, 
      InReadWH, InReadW, InReadWidth, InReadH, InReadHeight, InReadWHEOL, 
      InStartRow, 
        InRowByte, InNextRowByte, 
      InEndRow,
    InImageReceived,
    InErrorStop
    };

  States state = InStartImage;
  Adafruit_Thermal &printer;

  public:
    static constexpr const char* Ready = "Thermal\r"; // for next image

    SerialToThermalStream(Adafruit_Thermal &printer) : printer(printer) {}
    void begin() {
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

  void handle() {
    do {
      machine();
      }
    while ( ! ( state == InStartImage || state == InErrorStop ) );
  }

  void machine() {
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

      case InReadWHEOL :
        next_state( expect_eol(), InErrorStop, InStartRow );
        break;

      case InStartRow :
        print('R');
        state = InRowByte;
        break;

      case InRowByte :
        next_state( read_row_byte(), InErrorStop, InNextRowByte );
        break;

      case InNextRowByte :
        // if end of row: 
          state = InEndRow;
        // else
          state = InRowByte;
        break;

      case InEndRow :
        // if end of image
          state = InImageReceived;
        // else
          state = InStartRow;
        break;

      case InErrorStop :
        // FIXME
        // prob just reset
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

  int expect_eol() {
    // tolerate \r \n sequences
    return 1;
    }

  int read_row_byte() {
    // 2 hex nybbles
    static short nybble = 0;

    // read into proper location of the row
    return 1;
    }
};
