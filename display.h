#include "Adafruit_LEDBackpack.h"
#include "Adafruit_GFX.h"

class Mirror_AlphaNum4 : public Adafruit_AlphaNum4 {
  public: 
    void writeDigitMirror(uint8_t n, uint8_t a, bool d);
};
