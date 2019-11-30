/*
  Pass images from processing to thermal printer.

  LED_BUILTIN:
    off at reset, on when setup() is done
    flashing (bright/dim) when idle()
    on when printing();
    FIXME: fast-flash on error

*/

#include "tired_of_serial.h"
#include "Adafruit_Thermal.h"
#include "SoftwareSerial.h"
#define DEBUGLEVEL 1
#include "SerialToThermalStream.h"
#include "Blinker.h"

const int ThermalBaud = 9600;
#define TX_PIN 12 // Arduino transmit  YELLOW WIRE  labeled RX on printer
#define RX_PIN 11 // Arduino receive   GREEN WIRE   labeled TX on printer

SoftwareSerial mySerial(RX_PIN, TX_PIN); // Declare SoftwareSerial obj first
Adafruit_Thermal printer(&mySerial);     // Pass addr to printer constructor
SerialToThermalStream bitmap_transport(printer);

Blinker blinker(LED_BUILTIN, 300);

//FIXME: width/height are in bits, not bytes!

const long int SerialBaud = 57600; // seems to work at DEBUGLEVEL 1 (fails for long rows at 2)

void setup() {
  blinker.begin();

  Serial.begin(SerialBaud);
  Serial.println("start"); // all non-protocol messages should be all lower case

  // NOTE: SOME PRINTERS NEED 9600 BAUD instead of 19200, check test page.
  mySerial.begin(ThermalBaud);  // Initialize SoftwareSerial (or 9600)

  if (true) {
    printer.begin();
    // Init printer
    printer.setDefault(); // redundant?
    printer.println(F("Printer Ready"));
    printer.println();
    printer.println();
    println("printer ready");
    }
  else {
    println("no printer");
    }

  bitmap_transport.begin();

  print( SerialToThermalStream::Ready ); // signal start of protocol
  blinker.state(HIGH);
  }

void loop() {
  boolean led_state = blinker.blink();

  digitalWrite(LED_BUILTIN, HIGH); // leave on when printing, flash when idle
  bitmap_transport.handle();
  digitalWrite(LED_BUILTIN, led_state); // back to blinker state

}
