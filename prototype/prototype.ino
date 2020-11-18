
#include <Six302.h>

/*
 * Compiler constants
 */

// Clock cycle definitions of the system. How long it will take
// the device to respond to changes. All units in microseconds.
#define STEP_PERIOD   10000
#define REPORT_PERIOD 50000
#define BLINK_PERIOD  1000000

// determines when to switch states
// units of time cycles
#define TOL_LOW_INR 10 // for TOO_LOW to IN_RANGE transition
#define TOL_INR_LOW 10 // for IN_RANGE to TOO_LOW
#define TOL_INR_HIGH 10 // for IN_RANGE to TOO_HIGH
#define TOL_HIGH_INR 10 // for TOO_HIGH to IN_RANGE

// units of mmHg
#define DEV_LOW_INR 5.0 // transition from LOW to IN_RANGE state
#define DEV_INR_LOW -80 // really low so that valve only turns on once
#define DEV_INR_HIGH 5.0
#define DEV_HIGH_INR -5.0

// pins for FlexiForce sensors
#define SENSOR_UPPER  A7
#define SENSOR_MIDDLE A8
#define SENSOR_LOWER  A9

// pin for valve switch output
#define VALVE_PIN A14

// audio buzzer
#define SPEAKER_PIN 6 // pwm
#define SPEAKER_VOLUME 100.0 // between 0.0 and 100.0 -- not logarithmic

// system states
#define STATE_INITIAL  0
#define STATE_TOO_LOW  1
#define STATE_TOO_HIGH 2
#define STATE_IN_RANGE 3

// Pressure space we're working in. All are floats in units of
// mmHg.
#define P_MIN 15.0
#define P_MAX 50.0
#define P_INCREMENT 5.0

// 6302view data reporter
// https://github.com/almonds0166/6302view
CommManager cm(STEP_PERIOD, REPORT_PERIOD);

#include <Socks275.h> // for the FlexiForce sensors

// sensor objects
A301 sensor_upper(SENSOR_UPPER);
A301 sensor_middle(SENSOR_MIDDLE);
A301 sensor_lower(SENSOR_LOWER);
float P_lower;
float P_middle;
float P_upper;

#include "gui.h" // buttons and OLED

// button Bounce2 objects
Bounce btn_up   = Bounce();
Bounce btn_down = Bounce();
Bounce btn_set  = Bounce();
Bounce btn_E    = Bounce();

// OLED display object
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI
   oled(OLED_ROTATION, OLED_CS, OLED_DC, OLED_RESET);

/*
 * System variables
 */

uint8_t state = STATE_INITIAL; // system state
bool valve_is_on = false;
bool device_is_screeching = false;
bool blink_off;

// high-level variables
float P_desired; // 0.0 corresponds to no desired pressure set yet
float P_reading; // pressure according to the moving average
float P_temp = P_MIN; // pressure before the user confirms P_desired

// Length of a continuous chunk of clock cycles in which the
// pressure is out of desired range. To be used with TOLERANCE.
int16_t outside_range_count;

// timing
elapsedMicros main_timer;
elapsedMicros screen_timer;

#include <stdio.h>
#define STRING_LEN 32

char buf[STRING_LEN]; // generic char buffer

/*
 * setup() and loop()
 */

void setup() {
   // read/write resolution -> 12 bits
   analogReadResolution(12);
   analogWriteResolution(12);

   // output pins
   pinMode(VALVE_PIN, OUTPUT);
   pinMode(SPEAKER_PIN, OUTPUT);

   // input pins
   pinMode(SENSOR_UPPER, INPUT);
   pinMode(SENSOR_MIDDLE, INPUT);
   pinMode(SENSOR_LOWER, INPUT);

   // best frequency to drive speaker (depends
   // on the speaker you use)
   //analogWriteFrequency(); // default: ~448 Hz
   
   // setup oled
   SPI.setSCK(SPI_CLK); // move SPI SCK pin
   oled.begin();
   oled.setFont(OLED_FONT);
   oled.setBitmapMode(1);
   oled.setClipWindow(0, 9, 127, 37); // depends on font, etc.

   // setup buttons
   pinMode(BTN_UP, INPUT_PULLUP);
   pinMode(BTN_DOWN, INPUT_PULLUP);
   pinMode(BTN_SET, INPUT_PULLUP);
   pinMode(BTN_E, INPUT_PULLUP);
   btn_up.attach(BTN_UP);     btn_up.interval(BTN_DEBOUNCE);
   btn_down.attach(BTN_DOWN); btn_down.interval(BTN_DEBOUNCE);
   btn_set.attach(BTN_SET);   btn_set.interval(BTN_DEBOUNCE);
   btn_E.attach(BTN_E);       btn_E.interval(BTN_DEBOUNCE);
   update_buttons();

   // setup 6302 modules
   cm.addPlot(&P_reading,  "P_reading", 0, 75);
   cm.addPlot(&P_lower,   "P_lower",    0, 75);
   cm.addPlot(&P_middle,  "P_middle",   0, 75);
   cm.addPlot(&P_upper,   "P_upper",    0, 75);

   turn_off_buzzer();
   turn_off_valve();

   write_current_pressure();
   write_set_pressure();

   // connect over Serial
   cm.connect(&Serial, 115200);
   
   main_timer = 0;
}

void loop() {
   update_buttons();
   update_pressure_readings();

   // state specific functions
   switch( state ) {
      case STATE_INITIAL: {
         // see non-state specific functions bellow
         break;
      }
      case STATE_TOO_LOW: {
         if( !valve_is_on ) {
            turn_on_valve();
            valve_is_on = true;
         }
         if( P_reading > P_desired + DEV_LOW_INR ) {
            outside_range_count++;
         } else {
            outside_range_count = 0;
         }
         if( outside_range_count >= TOL_LOW_INR ) {
            // we're now considered in range
            outside_range_count = 0;
            state = STATE_IN_RANGE;
         }
         break;
      }
      case STATE_TOO_HIGH: {
         if( !device_is_screeching ) {
            alert_high_pressure();
            device_is_screeching = true;
         }
         if( P_reading < P_desired + DEV_LOW_INR ) {
            outside_range_count--;
         } else {
            outside_range_count = 0;
         }
         if( outside_range_count <= -TOL_HIGH_INR ) {
            // we're now considered in range
            outside_range_count = 0;
            state = STATE_IN_RANGE;
         }
         break;
      }
      case STATE_IN_RANGE: {
         if( valve_is_on ) {
            turn_off_valve();
            valve_is_on = false;
         }
         if( device_is_screeching ) {
            turn_off_buzzer();
            device_is_screeching = false;
         }
         if( P_reading > P_desired + DEV_INR_HIGH ) {
            outside_range_count++;
         } else if( P_reading < P_desired + DEV_INR_LOW ) {
            outside_range_count--;
         } else {
            outside_range_count = 0;
         }
         if( outside_range_count >= TOL_INR_HIGH ) {
            state = STATE_TOO_HIGH;
            outside_range_count = 0;
         } else if( outside_range_count <= -TOL_INR_LOW ) {
            state = STATE_TOO_LOW;
            outside_range_count = 0;
         }
         break;
      }
   }

   // non-state specific functions
   if( btn_E.fell() ) {
      if( valve_is_on ) {
         turn_off_valve();
         valve_is_on = false;
      }
      if( device_is_screeching ) {
         turn_off_buzzer();
         device_is_screeching = false;
      }
      P_desired = 0.0;
      P_temp = P_MIN;
      outside_range_count = 0;
      state = STATE_INITIAL;
      write_set_pressure();
   } else if( btn_set.rose() ) {
      P_desired = P_temp;
      write_set_pressure();
      if( P_reading < P_desired - P_INCREMENT ) {
         state = STATE_TOO_LOW;
      } else if( P_reading > P_desired + P_INCREMENT ) {
         state = STATE_TOO_HIGH;
      } else {
         state = STATE_IN_RANGE;
      }
   } else if( btn_up.rose() ) {
      if( P_temp < P_MAX ) {
         P_temp += P_INCREMENT;
         write_set_pressure();
      }
   } else if( btn_down.rose() ) {
      if( P_temp > P_MIN ) {
         P_temp -= P_INCREMENT;
         write_set_pressure();
      }
   }

   update_display();
   cm.step();
}

/*
 * All other functions
 */

void update_buttons() {
   btn_up.update();
   btn_down.update();
   btn_set.update();
   btn_E.update();
}

void update_pressure_readings() {
   sensor_upper.update();
   sensor_middle.update();
   sensor_lower.update();
   P_lower  = sensor_lower.readPressure();
   P_middle = sensor_middle.readPressure();
   P_upper  = sensor_upper.readPressure();
   P_reading = (
      P_lower +
      P_middle +
      P_upper ) / 3.0;
}

void update_display() {
   // breaks if screen need not be updated
   if( screen_timer >= BLINK_PERIOD ) {
      //Serial.println("Updating P_reading on screen");
      write_current_pressure();
      screen_timer = 0;
   }
}

void write_current_pressure() {
   // draw black box over the previously written pressure
   oled.setDrawColor(0); // transparent
   oled.drawBox(0, 9, 127, 14); // note, this depends on what font we're using
   // write new pressure
   oled.setDrawColor(1); // solid
   sprintf(buf, "Current: %.1f mmHg  ", P_reading);
   oled.drawStr(5, 20, buf);
   oled.updateDisplay();
}

void write_set_pressure() {
   // draw black box over the previously written pressure
   oled.setDrawColor(0); // transparent
   oled.drawBox(0, 23, 127, 12);
   // write new pressure
   oled.setDrawColor(1); // solid
   sprintf(buf, "Desired: %.1f mmHg %c ",
      P_temp,
      (P_temp != P_desired)? '*' : ' '); // print asterisk if unconfirmed
   oled.drawStr(5, 35, buf);
   oled.updateDisplay();
}

void turn_on_valve() {
   analogWrite(VALVE_PIN, 4095);
}

void turn_off_valve() {
   analogWrite(VALVE_PIN, 0);
}

void alert_high_pressure() {
   // references:
   // http://justanotherlanguage.org/content/tutorial_pwm2
   // https://www.pjrc.com/teensy/td_pulse.html
   // note: duty cycle of 0.5 or more is max volume
   uint16_t value = 4096.0 * (SPEAKER_VOLUME / 200.0);
   analogWrite(SPEAKER_PIN, (value < 4095) ? value : 4095);
}

void turn_off_buzzer() {
   analogWrite(SPEAKER_PIN, 0);
}
