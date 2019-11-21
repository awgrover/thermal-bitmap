/*
 BitMap sender to matching arduion sketch
 For https://www.adafruit.com/product/2751
 
 Using code from https://github.com/adafruit/Adafruit-Thermal-Printer-Library
 
 Will skip printing if arduino times-out. You can reset arduino to try again.
 
 */

import processing.serial.*;

Serial arduino;
final int ArduinoBaud = 115200;

final int PixelationSize = 4; // treat a nxn as one printer pixel, sort of an inverse of scale()
FIXME
  final int PrinterWidth=384; // per adafruit. height is effectively unlimited
final int BlackWhiteAt = 0xFFFFFF/2; // above is white, below is black
final String ArduinoReady = "Thermal\r"; // for next image
final int ArduinoAckTimeout = 10; // time enough to process a row, finish printing
final char ReadyForRow = 'R'; // each row
final char ReadyForImage = 'I';
final char ImageAccepted = 'X'; // at end of image

boolean once = false;

void setup () {
  size(20, 30);
  background(255);

  arduino = connectUSBSerial(ArduinoBaud);
  wait_for_arduino_ready( arduino, true );
}

void draw() {
  while (once) {
    delay(500);
  }

  stroke(0);
  fill(0);
  square(10, 0, 2); // top edge, showing fill bits
  square(2, 2, 2);
  square(8, 8, 9);
  square(width-3, 19, 3); // end edge

  // on print:
  // busy-spinner...
  if (! printImageRotated() ) {
    // show something, decide what to do...
  }

  once = true;
}

void wait_for_arduino_ready( Serial port, boolean echo_other) {
  // wait for the magic ready string
  // with timeout
  println("Waiting for arduino...");

  if (port != null) {
    int at = 0;

    while (true) {
      if (port.available() > 0) {
        char in = arduino.readChar();

        if (in == ArduinoReady.charAt(at)) {
          at += 1;

          if (at == ArduinoReady.length()) {
            println("\nArduino Ready: "+ArduinoReady);
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
    println("No port");
  }
}

boolean wait_for_arduino( Serial port, char expected) {
  // wait for the char
  // with timeout
  println("Waiting for " + String.valueOf(expected)+"...");

  int start = millis();

  if (port != null) {


    while (true) {
      if (millis() - start > ArduinoAckTimeout) {
        println("Error: arduino timed out waiting for "+String.valueOf(expected));
        return false;
      }

      if (port.available() > 0) {
        char in = arduino.readChar();

        if (in == expected) {
          println("saw: "+String.valueOf(expected));
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

    if (arduino != null) {
      if (!wait_for_arduino( arduino, ReadyForImage )) {
        println("Failed at row "+String.valueOf(pixel_x));
        return false;
      }
    }

  int width_to_write = min(PrinterWidth, height); // print width = image height

  if (arduino != null) {
    arduino.write( String.format("W%03XH%04X\n", width_to_write, width) );
  } else {
    print( String.format("W%03XH%04X\n", width_to_write, width) );
  }
  // flush

  int aByte = 0; // we'll accumulate one byte at a time

  // send a column... (so rotated)
  // x is left-to-right
  for ( int pixel_x = 0; pixel_x < width; pixel_x += 1) {
    // up the image, so y direction first (from bottom!), i.e. rotated

    if (arduino != null) {
      if (!wait_for_arduino( arduino, ReadyForRow )) {
        println("Failed at row "+String.valueOf(pixel_x));
        return false;
      }
    }

    print("[");
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
        if (arduino != null) { 
          arduino.write( String.format( "%02X", aByte) );
          // fixme: may have to wait each 3-8 bytes...
        } else {
          print( String.format("%8s", Integer.toBinaryString(aByte)).replaceAll(" ", "0") );
        }
        aByte = 0;
        bit_count = 0;
      }
    }

    // end of row
    if (arduino != null) {
      arduino.write("\n");
      print("\n"); // for progress output
    } else {
      print("\n");
    }
  }

  // End of image
  if (arduino != null) {
    arduino.write("X\n");
    print( "X\n" );

    return wait_for_arduino( arduino, ImageAccepted );
  } else {
    print( "X\n" );
    return true; // non-port always works
  }
}
