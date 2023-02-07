#include "motor_simple.h"

struct config {
        int pin_positive;        // pin to be pulled high to move into positive direction, pin_negative will be low
        int pin_negative;        // pin to be pulled high to move into negative direction, pin_positive will be low
        int pin_ref;             // reference switch pin
        bool ref_pullup;         // use INPUT_PULLUP for ref switch input
        bool ref_positive_state; // HIGH, LOW - which state the ref pin has to be in for the motor to move into the positive direction
        bool ref_auto;           // automatic reference, will reference when hitting ref switch
        int  ref_pos;            // in pulses
    };

void motor_simple::init(struct config config) {
    _config = config;

    struct motor_basic::config basic_cfg = {
      .pin_positive       = _config.pin_positive,
      .pin_negative       = _config.pin_negative,
      .pin_ref            = _config.pin_ref,
      .ref_pullup         = _config.ref_pullup,
      .ref_positive_state = _config.ref_positive_state,
      .ref_auto           = _config.ref_auto,
      .ref_pos            = degToPulses(_config.ref_pos),
    };

    motor.init(basic_cfg);
}
