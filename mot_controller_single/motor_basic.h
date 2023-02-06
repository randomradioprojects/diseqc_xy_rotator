#pragma once
#include <Arduino.h>

class motor_basic {
public:
    struct config {
        int pin_positive;        // pin to be pulled high to move into positive direction, pin_negative will be low
        int pin_negative;        // pin to be pulled high to move into negative direction, pin_positive will be low
        int pin_ref;             // reference switch pin
        bool ref_pullup;         // use INPUT_PULLUP for ref switch input
        bool ref_positive_state; // HIGH, LOW - which state the ref pin has to be in for the motor to move into the positive direction
        bool ref_auto;           // automatic reference, will reference when hitting ref switch
        int ref_pos;             // in pulses
    };

    void init(struct config config);

    void motorInterrupt();

    void reference();

    void print_debug_state();

    void stop();

    void moveToPulsePos(int pos);

    volatile int currentPulsePos;
    volatile int targetPulsePos;

private:
    struct config _config;

    /* runtime idfk stuff */
    volatile bool interruptEnabled;
    volatile enum class motorDirection { NONE, POSITIVE, NEGATIVE } currentDirection;
    volatile int ref_auto_last_state;
    int isr_increment = 0;

    void motorSpeen(motorDirection direction);

    void move_positive();
    void move_negative();
    void move_stop();
};
