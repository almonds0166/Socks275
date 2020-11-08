
/* Reach out to almonds@mit.edu for help */

#ifndef _Socks275_
#define _Socks275_

#include "Arduino.h"

/* Sensor options */

#define MOVING_SUM_SIZE 10

/* Class A301 */

class A301 {

   public:

      A301(uint8_t pin);

      void update();
      float readVoltage();  // V
      float readPressure(); // mmHg

   protected:

      uint8_t _pin;
      uint32_t _moving_sum;
      uint16_t _history[MOVING_SUM_SIZE];
      uint16_t _next_slot;

};

#endif
