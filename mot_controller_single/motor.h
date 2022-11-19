#include <Arduino.h>

class motor {
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
        bool pwm_smoothing;      // use PWM to smooth start and stop
        float pwm_smoothing_deg; // will stop/start PWMing down at x degrees from start/stop
        float pwm_min_fraction;  // values 0-1
        float min_pos_diff_deg;  // position difference in degrees needed for motor to start moving
    };

    void init(struct config config);

    void motorInterrupt();

    void reference();

    void moveToPos(float posDegrees) {
        float diff = fabs(getCurrentPos() - posDegrees);
        if (diff >= _config.min_pos_diff_deg) {
            moveToPulsePos(degToPulses(posDegrees));
        }
    }

    float getCurrentPos() {
        return pulsesToDeg(currentPulsePos);
    }

    void print_debug_state();

    void stop();

private:
    struct config _config;

    int _ref_pos_pulses;
    int _pwm_smoothing_pulses;

    /* runtime idfk stuff */
    volatile int currentPulsePos;
    volatile int targetPulsePos;
    volatile int startPulsePos;
    volatile bool interruptEnabled;
    volatile enum class motorDirection { NONE, POSITIVE, NEGATIVE } currentDirection;
    volatile bool can_ref_auto;
    volatile int ref_auto_last_state;
    int isr_increment = 0;
    volatile bool can_slow_start;

    void motorSpeen(motorDirection direction, float pwm = 1);

    int degToPulses(float deg) {
        return deg * _config.ppd;
    }

    float pulsesToDeg(int pulses) {
        return (float)pulses / _config.ppd;
    }

    void moveToPulsePos(int pos);
};
