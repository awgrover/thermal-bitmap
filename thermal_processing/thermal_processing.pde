/*
 BitMap sender to matching arduion sketch
 For https://www.adafruit.com/product/2751
 
 Using code from https://github.com/adafruit/Adafruit-Thermal-Printer-Library
 
 Will skip printing if arduino times-out. You can reset arduino to try again.
 
 */

import processing.serial.*;

final int ArduinoBaud = 57600;
final int PixelationSize = 4; // treat a nxn as one printer pixel, sort of an inverse of scale()

PrintImage print_image = new PrintImage();

void setup () {
  size(30, 22); // min 30,22. max nn,348
  background(255);

  print_image.begin( connectUSBSerial(ArduinoBaud), ArduinoBaud);
}

void draw_pattern() {  

  stroke(0);
  fill(0);
  square(10, 0, 2); // top edge, showing fill bits
  square(2, 2, 2);
  square(8, 8, 9);
  square(width-3, 19, 3); // end edge
}

int state=0;
void draw() {

  switch(state) {
  case 0:
    draw_pattern();
    break;
    
  case 1:
    // on print:
    // busy-spinner...
    // this blocks
    if (! print_image.printImageRotated() ) {
      // show something, decide what to do...
      print("protocol returned false! something didn't work.");
    }
    print_image.wait_for_image_start();
    break;
    
  case 2:
    delay(500); // don't burn cpu
    break;
  }

  state += 1;
}
