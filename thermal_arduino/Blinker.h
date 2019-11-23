/*
  Simple led blinker, non-blocking.
  Starts as off
  Consumes several bytes
*/

class Blinker {
  private:
    int pin;
    unsigned long timer;
    boolean _state;

  public:
    // you can change this at will
    int rate;

    Blinker(int pin, int rate) : pin(pin), rate(rate), timer( millis() ), _state(LOW) {} 

    void begin() {
      pinMode(pin, OUTPUT);
      digitalWrite(pin, _state);
      }

    // you can force the state
    boolean state() { return _state; }
    boolean state(boolean state) { 
      timer = millis(); // less drift
      _state=state; 
      digitalWrite(pin, _state);
      return state; 
      }

    boolean blink() {
      if (millis() - timer > rate) {
        state( ! _state );
      }
      return _state;
    }
};
