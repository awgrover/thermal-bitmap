import processing.serial.*;

Serial connectUSBSerial(int baud) {
  // return a Serial object that we think is it, or null
  String[] flipDotPorts = Serial.list();
  //println(flipDotPorts);

  String arduinoPortName = null;
  for (int i = 0; i < flipDotPorts.length; i++) {
    if (flipDotPorts[i].contains("ACM") ) { 
      arduinoPortName = flipDotPorts[i];
    }
  }

  if (arduinoPortName != null) {
    print("Connected to ");
    println(arduinoPortName);
    return new Serial( this, arduinoPortName, baud);
  } else {
    println("Failed to find an usb serial");
    return null;
  }
}
