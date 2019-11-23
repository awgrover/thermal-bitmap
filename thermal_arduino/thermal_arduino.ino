/*
  Pass images from processing to thermal printer.

  LED_BUILTIN:
    off at reset, on when setup() is done
    flashing when idle()
    on when printing();
    FIXME: fast-flash on error

*/

#include "tired_of_serial.h"
#include "Adafruit_Thermal.h"
#include "SoftwareSerial.h"
#include "SerialToThermalStream.h"
#include "Blinker.h"

#define TX_PIN 6 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 5 // Arduino receive   GREEN WIRE   labeled TX on printer

SoftwareSerial mySerial(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor
SerialToThermalStream bitmap_transport(printer);

Blinker blinker(LED_BUILTIN, 300);

const int SerialBaud = 115200;

void setup() {
  blinker.begin();

  Serial.begin(SerialBaud);
  println("start"); // all non-protocol messages should be all lower case

  // NOTE: SOME PRINTERS NEED 9600 BAUD instead of 19200, check test page.
  mySerial.begin(19200);  // Initialize SoftwareSerial (or 9600)

  printer.begin();        // Init printer (same regardless of serial type)
  printer.setDefault(); // redundant?
  printer.println(F("Printer Ready"));

  print( SerialToThermalStream::Ready ); // signal start of protocol

  blinker.state(HIGH);
  }

void loop() {
  boolean led_state = blinker.blink();

  digitalWrite(LED_BUILTIN, HIGH); // leave on when printing, flash when idle
  bitmap_transport.handle(); // blocks till done
  digitalWrite(LED_BUILTIN, led_state); // back to blinker state

}
