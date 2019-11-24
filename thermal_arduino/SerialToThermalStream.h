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
    <- nn...\r # a row of hex bytes, upper case 0-9A-F
    -> R # ready for next row
    <- nn...\r # a row of hex bytes
    ...
    <- nn...\r # a row of hex bytes
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

  // we provide for several max-width rows, but that could end up being more rows if the actual width is less
  static const int MaxBufferSize = 500; // will "round" to multiple of BytePerRow (usu. of 48)
  static const int BitWidth = 384; // according to https://learn.adafruit.com/mini-thermal-receipt-printer?view=all 
  static const int BytesPerRow = BitWidth / 8;
  static const int BufferSize = ( MaxBufferSize / BytesPerRow) * BytesPerRow ; // multiple of bytes per row <= 500
  static const int RowsPerBuffer = BufferSize / BytesPerRow; // if you use the whole width

  static const char ReadyForRow = 'R'; // each row
  static const char ReadyForImage = 'I';
  static const char ImageAccepted = 'X'; // at end of image
  static const char Error = 'E'; // error happened

  enum States { 
    InStartImage, 
      InReadWH, InReadW, InReadWidth, InReadH, InReadHeight, InReadWHEOL, 
      InStartRow, 
        InRowByte, InNextRowByte, 
      InEndRow, InNextRow,
      InFlushImage,
    InImageReceived,
    InErrorStop
    };

  States state = InStartImage;
  Adafruit_Thermal &printer;
  byte image_rows[ BufferSize ];

  int width;
  int height;
  int row_i_total, row_i, col_i; // as we write into the image_rows buffer

  public:
    static constexpr const char* Ready = "Thermal\r"; // for next image

    SerialToThermalStream(Adafruit_Thermal &printer) : printer(printer) {}
    boolean begin() {
      print(F("thermal buff "));print(BufferSize);println();
      return true;
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

  int at( int r_i, int c_i) {
    // calculate the flat index
    return c_i * width * r_i;
    }

  void handle() {
    do {
      machine();
      }
    while ( ! ( state == InStartImage || state == InErrorStop ) );
  }

  void machine() {
    static unsigned long last_state_time = millis();
    static States last_state = -1;

    switch(state) {

      case InStartImage:
        row_i_total = row_i = col_i = 0;
        next_state( start_image(), InErrorStop, InReadWH );
        break;
      
      // WnnnHnnnn\n
      case InReadWH :
        state = InReadW;
        break;

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
        col_i = 0;
        state = InRowByte;
        break;

      case InRowByte :
        next_state( expect_hex(2, image_rows[ at(row_i, col_i) ]), InErrorStop, InNextRowByte );
        break;

      case InNextRowByte :
        // next byte, or next row
        col_i += 1;

        if ( col_i >= width ) {
          // next row
          state = InEndRow;
        }

        else {
          // next byte
          state = InRowByte;
        }

        break;

      case InEndRow :
        next_state( expect_eol(), InErrorStop, InNextRow );
        break;

      case InNextRow :
        row_i += 1;
        row_i_total += 1;

        if ( row_i_total >= height ) {
          state = InImageReceived;
          }
        else if ( at(row_i, col_i) + width > BufferSize ) {
          // if the next row would overflow...
          state = InFlushImage; // we flush one buffer full, then continue
          }
        else {
          state = InStartRow;
          }

        break;

      case InFlushImage :
        // this one blocks while printing
        print(F("flush image at "));print(row_i);print(F(","));print(col_i);print(F(" "));print(at(row_i,col_i));println();
        printer.printBitmap(width, height, image_rows);
        col_i = 0;
        // adjust col_i/row_i, keep track fo actual rows, go on to InNextRow?

        if (row_i_total >= height) {
          state = InImageReceived;
        }
        else {
          state = InStartRow;
          }
        break;

      case InImageReceived :
        print(ImageAccepted);
        break;

      case InErrorStop :
        print(Error);
        // prob just reset
        break;
    }

    if (state != last_state) {
      print(F("d "));print(last_state);print(F(" "));print(state);print(F(" ")); println(millis()-last_state_time);
      last_state_time = millis();
      }

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
    if (Serial.available() > 0 ) {
      char in = Serial.read();
      if (in == which) {
        return 1;
        }
      else {
        print(F("bad char, expected "));print(which);print(F(" saw "));println(in);
        return -1; // failed
        }
      }

    else {
      return 0; // wait
      }
    }

  template <typename T>
  int expect_hex(int digits, T &value) {
    // consume n hex-digits, set &value
    //   value can be any numeric type, e.g. int or byte
    // writes to the value so you can use this in the state-machine style
    static int digit_ct = 0;

    if (Serial.available() > 0 ) {

      // init value to 0
      if (digit_ct == 0) value = 0;

      char in = Serial.read();
      if (in >= '0' && in <= '9') {
        value << 4;
        value += in - '0';
        }
        
      if (in >= 'A' && in <= 'F') {
        value << 4;
        value += in - 'A' + 10;
        }
      else {
        print(F("bad hex digit, saw "));println(in);
        digit_ct = 0;
        return -1; // failed
      }
      
      // are we done?
      if (digit_ct > digits) {
        digit_ct = 0;
        return 1;
        }
    }
    else {
      return 0; // wait
      }
  }

  int expect_eol() {
    // only \r
    if (Serial.available() > 0 ) {
      char in = Serial.read();
      if (in == '\r' ) {
        return 1;
      }
      else {
        print(F("bad eol, expected \\r saw "));print((int)in);print(F("/"));println(in);
        return -1;
        }
      }

    return 0;
    }

};
