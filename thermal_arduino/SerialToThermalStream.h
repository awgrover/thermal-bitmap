/*
  Pass images from processing to thermal printer.
  For https://www.adafruit.com/product/2751

  Install Adafruit Thermal Printer

  Edit Adafruit_Thermal.cpp, change baudrate to value on printout
  Edit Adafruit_Thermal.h, change firmware to value on printout
  Construct a serial object with the baudrate for us.

  #define DEBUGLEVEL=1 for toplevel debug, 2 for detail (slows things down)

  Protocol:
    -> "Thermal\r" # at startup, arduino is ready for first image. nb \r

    -> I # ready for image
    <- WnnnHnnnn\r # height and width of image in hex
    -> R # ready for 1st row
    <- nn...\r # a row of hex bytes, upper case 0-9A-F
    -> R # ready for next row
    <- nn...\r # a row of hex bytes
    ...
    <- nn...\r # a row of hex bytes
    -> X # last row seen

    All other strings are debugging and should be ignored/shown-on-console

    example sequence
    W001H0001
    01

    W002H0002
    0110
    0220

    W030H000B # 48 bytes, 11 rows, 1 more than buffer size
    800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001
    10 times, then it should flush
    800000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000001
    and done

  Thermal library
    bitmap writes in chunks of 255 (possibly with flow control)
    baud is 19200, which is 1920 chars/sec, which is 0.5 msec/char, i.e. 2 chars/msec
    assuming I can break the image at a row, and continue w/ "next" image
*/

#pragma once

#include "tired_of_serial.h"

#ifndef DEBUGLEVEL
  #define DEBUGLEVEL 0
#endif

#define debug(n, statements) if (n<=DEBUGLEVEL) {statements}

class SerialToThermalStream {

  // we provide for several max-width rows, but that could end up being more rows if the actual width is less
  static const int MaxBufferSize = 500; // will "round" to multiple of BytePerRow (usu. of 48)
  static const int BitWidth = 384; // according to https://learn.adafruit.com/mini-thermal-receipt-printer?view=all 
  static const int BytesPerRow = BitWidth / 8;
  static const int BufferSize = ( MaxBufferSize / BytesPerRow) * BytesPerRow ; // multiple of bytes per row <= 500
  static const int RowsPerBuffer = BufferSize / BytesPerRow; // if you use the whole width

  // one char "ack". and not a hex digit
  static const char ReadyForRow = 'R'; // each row
  static const char ReadyForImage = 'I';
  static const char ImageAccepted = 'X'; // at end of image
  static const char Error = 'Z'; // error happened

  enum States { 
    InStartImage, 
      InReadWH, InReadW, InReadWidth, InReadH, InReadHeight, InReadWHEOL, 
      /*7*/ InStartRow, 
        InRowByte, InNextRowByte, 
      /*10*/ InEndRow, InNextRow,
      InFlushImage,
    InImageReceived,
    /*14*/ InErrorStop
    };

  States state = InStartImage;
  Adafruit_Thermal &printer;
  byte image_rows[ BufferSize ];

  int width; // bytes, rounded-up
  int height; // rows
  int width_bits;
  int row_i_total, row_i, col_i; // as we write into the image_rows buffer

  int digit_ct = 0; // for hex digits


  public:
    static constexpr const char* Ready = "Thermal\r"; // for next image

    SerialToThermalStream(Adafruit_Thermal &printer) : printer(printer) {}
    boolean begin() {
      debug(1, print(F("thermal buff "));print(BufferSize);println();)
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

  int at( int c_i, int r_i) {
    // calculate the flat index
    return r_i * width + c_i;
    }

  void handle() {
    // non-blocking, but needs to be called pretty often. don't do a delay()!
    machine();

    /* this would block: 
    do {
      machine();
      }
    while ( ! ( state == InStartImage || state == InErrorStop ) );
    */
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
        next_state( expect_hex(3, width_bits), InErrorStop, InReadH );
        break;
      case InReadH :
        next_state( expect_char('H'), InErrorStop, InReadHeight );
        break;

      case InReadHeight :
        next_state( expect_hex(4, height), InErrorStop, InReadWHEOL );
        break;

      case InReadWHEOL :
        next_state( expect_eol(), InErrorStop, InStartRow );
        if (state != InReadWHEOL) { 
          width = (width_bits / 8) + (width_bits % 8 ? 1 : 0);
          debug(1,
            print(F("  w 0x"));print(width,HEX);print(F("/"));print(width);print(F("(bits"));print(width_bits);print(F(")"));
            print(F(" h 0x"));print(height,HEX);print(F("/"));print(height);
            println();
            )
          }
        break;

      case InStartRow :
        print(ReadyForRow);
        col_i = 0;
        state = InRowByte;
        break;

      case InRowByte :
        next_state( expect_hex(2, image_rows[ at(col_i, row_i) ]), InErrorStop, InNextRowByte );
        break;

      case InNextRowByte :
        // next byte, or next row
        col_i += 1;
        debug(2, print(F("  col "));print(col_i);print(F("/"));println(width); )

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
        debug(1, print(F("  row "));print(row_i);print(F("/"));println(row_i_total); )

        if ( row_i_total >= height ) {
          debug(1, print(F("  end image "));print(row_i_total);print(F("/"));println(height); )
          state = InImageReceived;
          }
        else if ( at(0, row_i) + width > BufferSize ) {
          // if the next row would overflow...
          debug(1, print(F("end buffer "));print(at(col_i,row_i));print(F("/"));println(BufferSize); )
          state = InFlushImage; // we flush one buffer full, then continue
          }
        else {
          state = InStartRow;
          }

        break;

      case InFlushImage :
        // this one blocks while printing
        debug(1, print(F("flush image at "));print(row_i);print(F(","));print(col_i);print(F(" "));print(at(col_i,row_i));println(); )

        print_image();

        row_i = 0; // start at beginning of buffer again

        if (row_i_total >= height) {
          state = InImageReceived;
        }
        else {
          state = InStartRow;
          }
        break;

      case InImageReceived :
        print(ImageAccepted);
        print_image();
        // feed enough to tear off
        printer.println();
        printer.println();
        state = InStartImage;
        break;

      case InErrorStop :
        print(Error);
        // prob just reset
        state = InStartImage;
        break;
    }

    if (state != last_state) {
      if( 
        (DEBUGLEVEL == 1 && state != InStartRow && state != InRowByte && state != InNextRowByte) // those are two slow for 1
        || (DEBUGLEVEL > 1)
        )
        {
        print(F("d "));print(last_state);print(F(" "));print(state);print(F(" ")); println(millis()-last_state_time);
        }
      last_state = state;
      last_state_time = millis();
      }

  }

  void print_image() {
    printer.printBitmap(width_bits, height, image_rows, false); // not from progmem

    debug(1, 
      println();
      for(int r=0;r<row_i ;r++) { // only as many rows as we filled this time
        for(int c=0;c<width;c++) {
          printw( image_rows[ at(c, r) ], BIN );
        }
        println();
      }
    )
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
      debug(2, print(F("  < "));print((int)in);print(F(" "));println(in);)
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
    if (Serial.available() > 0 ) {

      // init value to 0
      if (digit_ct == 0) {
        debug(2, println(F("  start hex"));)
        value = 0;
        }

      char in = Serial.read();
      if (in >= '0' && in <= '9') {
        value <<= 4;
        value += in - '0';
        digit_ct += 1 ;
        debug(2, print(F("  @"));print(digit_ct);print(F(" "));print(in);print(F("="));printw(value,HEX);println();)
        }
        
      else if (in >= 'A' && in <= 'F') {
        value <<= 4;
        value += in - 'A' + 10;
        digit_ct += 1 ;
        debug(2, print(F("  @"));print(digit_ct);print(F(" "));print(in);print(F("="));print(value,HEX);println();)
        }

      else {
        print(F("bad hex digit at "));print(row_i_total);print(F(","));print(col_i);print(F(", saw "));print((int)in);print(F("/"));println(in);
        digit_ct = 0;
        return -1; // failed
      }
      
      // are we done?
      if (digit_ct >= digits) {
        debug(2,print(F("  hex "));print(F("@"));print((int)&value);print(F(" "));printw(value,HEX);println();)
        digit_ct = 0;
        return 1;
        }

      return 0; // need more digits
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
