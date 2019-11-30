/*
 BitMap sender to matching arduion sketch
 For https://www.adafruit.com/product/2751
 
 Using code from https://github.com/adafruit/Adafruit-Thermal-Printer-Library
 
 Will skip printing if arduino times-out. You can reset arduino to try again.
 
 */

class PrintImage {

  Serial arduino_usb;

  final int PrinterWidth=384; // per adafruit. height is effectively unlimited
  final int BlackWhiteAt = 0xFFFFFF/2; // above is white, below is black
  final String ArduinoReady = "Thermal\r"; // for next image
  final int ArduinoAckTimeout = 100; // time enough to consume usual commands
  final int ReadyForRowTimeout = 10000; // time enough to consume row, and flush/print a batch. lots of rows & narrow == long print time
  final char ReadyForRow = 'R'; // each row
  final char ReadyForImage = 'I';
  final char ImageAccepted = 'X'; // at end of image

  boolean begin (Serial arduino_usb, int usb_baud_rate) {

    this.arduino_usb = arduino_usb;
    wait_for_arduino_ready(true ); // FIXME: set "seen"==true. on print, if !seen, try again to allow arduino reset
    return true;
  }

  void wait_for_arduino_ready( boolean echo_other) {
    // wait for the magic ready string
    // with timeout
    println(">Waiting for arduino...");

    if (arduino_usb != null) {
      int at = 0;

      while (true) {
        if (arduino_usb.available() > 0) {
          char in = arduino_usb.readChar();

          if (in == ArduinoReady.charAt(at)) {
            at += 1;

            if (at == ArduinoReady.length()) {
              println("\n>Arduino Ready: "+ArduinoReady);
              return;
            }
          } else {
            at = 0; // reset
            if (echo_other) { 
              print(in);
            }
          }
        } else {
          delay( 1 ); // don't burn cpu too much
        }
      }
    } else {
      println(">No port");
    }
  }

  boolean wait_for_image_start() {
    return wait_for_arduino(ReadyForImage, ReadyForRowTimeout ); // echos remainder till I
  }

  boolean wait_for_arduino( char expected) {
    return wait_for_arduino(expected, ArduinoAckTimeout);
  }

  boolean wait_for_arduino(char expected, int arduino_ack_timeout) {
    // wait for the char
    // with timeout
    println(">Waiting for " + String.valueOf(expected)+"...");

    int start = millis();

    if (arduino_usb != null) {


      while (true) {
        if (millis() - start > arduino_ack_timeout) {
          println(">Error: arduino timed out waiting for "+String.valueOf(expected) + " after " + String.valueOf(millis() - start));
          return false;
        }

        if (arduino_usb.available() > 0) {
          char in = arduino_usb.readChar();

          if (in == expected) {
            println(">saw: "+String.valueOf(expected));
            return true;
          } else {
            print(in);
          }
        } else {
          delay( 1 ); // don't burn cpu too much
        }
      }
    }

    return true; // for non-port situation
  }

  boolean printImageRotated() {
    // Rotates 90 clockwise, so Y is the printerwidth, x can be any length.
    // Returns true if printing worked.
    // Converts each pixel to b/w
    // We send:
    // WnnnHnnnn\n
    // row\n
    // row\n
    // X\n
    // where nnn is hex for the width & height. nb: height is 4 hex chars
    // where row is hex bytes

    loadPixels(); // into pixels[]

    if (arduino_usb != null) {
      if (!wait_for_arduino( ReadyForImage )) {
        println(">Failed waiting-for-arduino "+ReadyForImage);
        return false;
      }
    }

    int width_to_write = min(PrinterWidth, height); // print width = image height

    if (arduino_usb != null) {
      arduino_usb.write( String.format("W%03XH%04X\r", width_to_write, width) );
    } else {
      print( String.format(">W%03XH%04X\n", width_to_write, width) );
    }
    // flush

    int aByte = 0; // we'll accumulate one byte at a time

    // send a column... (so rotated)
    // x is left-to-right
    for ( int pixel_x = 0; pixel_x < width; pixel_x += 1) {
      // up the image, so y direction first (from bottom!), i.e. rotated

      if (arduino_usb != null) {
        // we could get a flush-image here, which is many rows!
        if (!wait_for_arduino(ReadyForRow, ReadyForRowTimeout )) {
          println(">Failed at row_i "+String.valueOf(pixel_x));
          return false;
        }
      }

      print(">[");
      print( String.format("%2d", pixel_x));
      print("]");

      int bit_count = 0;

      for ( int pixel_y = width_to_write - 1; pixel_y >= 0; pixel_y -= 1) {
        bit_count += 1;
        int pixel_value = get(pixel_x, pixel_y) & 0xFFFFFF; // pixels[ pixel_y * width + pixel_x ] & 0xFFFFFF; // just rgb

        aByte <<= 1; // make room
        aByte |= pixel_value > BlackWhiteAt ? 0 : 1; // change to b/w
        /*print("(");
         print(pixel_y);print(":");
         //print( String.format("%03X",pixel_value));print(" ");
         print(pixel_value > BlackWhiteAt);print(" ");
         print(String.format("%08x",aByte));print(")");
         */

        if (bit_count % 8 == 0 || pixel_y == 0) {
          // last bit for this byte (or last pixel for row)
          if (pixel_y == 0 && bit_count % 8 != 0) { 
            // final partial byte
            aByte <<= 8 - (bit_count % 8) ; // shift over to msb, and we already did 1
          }

          //print( String.format( "%02X", aByte ) );
          if (arduino_usb != null) { 
            arduino_usb.write( String.format( "%02X", aByte) );
            print(String.format( "%02X", aByte));
          } else {
            print( String.format("%8s", Integer.toBinaryString(aByte)).replaceAll(" ", "0") );
          }
          aByte = 0;
          bit_count = 0;
        }
      }

      // end of row
      if (arduino_usb != null) {
        arduino_usb.write("\r");
        print("\n"); // for progress output
      } else {
        print("\n");
      }
    }

    // End of image
    if (arduino_usb != null) {
      print( ">end of image...\n" );

      return wait_for_arduino( ImageAccepted );
    } else {
      print( ">end of image\n" );
      return true; // non-port always works
    }
  }
}
