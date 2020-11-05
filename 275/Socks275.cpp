
#include "Arduino.h"
#include "Socks275.h"

/* Initialization */

A301::A301(uint8_t pin) {
   _pin = pin;
   _moving_sum = 0;
   _next_slot = 0;
}

void A301::update() {
   // subtract oldest data point from moving sum
   _moving_sum -= _history[_next_slot];
   // replace oldest data point with newest data point
   _history[_next_slot] = analogRead(_pin);
   // add newest data point to moving sum
   _moving_sum += _history[_next_slot];
   // move pointer to oldest data point ready for next update cycle
   _next_slot = (_next_slot + 1) % MOVING_SUM_SIZE;
}

// return moving average voltage
float A301::readVoltage() {
   return (3.3 * _moving_sum) / (4096.0 * MOVING_SUM_SIZE);
}

// return estimate of moving average pressure
// based on voltage-pressure investigations here
float A301::readPressure() {
   return (this->readVoltage() + 0.0484174) / 0.0288813;
}
