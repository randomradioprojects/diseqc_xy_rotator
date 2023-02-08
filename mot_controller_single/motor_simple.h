#pragma once
#include "motor_basic.h"
#include <Arduino.h>

class motor_simple {
public:
    struct config {
        int pin_positive;        // pin to be pulled high to move into positive direction, pin_negative will be low
        int pin_negative;        // pin to be pulled high to move into negative direction, pin_positive will be low
        int pin_ref;             // reference switch pin
        float ref_pos;           // degrees
        float ppd;               // pulses per degree
        bool ref_pullup;         // use INPUT_PULLUP for ref switch input
        bool ref_positive_state; // HIGH, LOW - which state the ref pin has to be in for the motor to move into the positive direction
        bool ref_auto;           // automatic reference, will reference when hitting ref switch
        float min_pos_diff_deg;  // position difference in degrees needed for motor to start moving
    };

    void init(struct config config);

    void motorInterrupt() {
        motor.motorInterrupt();
    }

    void reference() {
        motor.reference();
    }

    void moveToPos(float posDegrees) {
        float diff = fabs(getCurrentPos() - posDegrees);
        if (diff >= _config.min_pos_diff_deg) {
            motor.moveToPulsePos(degToPulses(posDegrees));
        }
    }

    float getCurrentPos() {
        return pulsesToDeg(motor.currentPulsePos);
    }

    void print_debug_state() {
        motor.print_debug_state();
        Serial.print("    currentPulsePos(°): ");
        Serial.println(pulsesToDeg(motor.currentPulsePos));
    }

    void stop() {
        motor.stop();
    }

    void loop() {
        motor.loop();
    }

private:
    struct config _config;

    int isr_increment = 0;

    int degToPulses(float deg) {
        return deg * _config.ppd;
    }

    float pulsesToDeg(int pulses) {
        return (float)pulses / _config.ppd;
    }

    motor_basic motor;
};
