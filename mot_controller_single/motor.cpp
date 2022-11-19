#include "motor.h"

void motor::init(struct config config) {
    _config = config;

    _ref_pos_pulses = degToPulses(config.ref_pos);
    _pwm_smoothing_pulses = degToPulses(config.pwm_smoothing_deg);

    if (_config.ref_pullup) {
        pinMode(_config.pin_ref, INPUT_PULLUP);
    } else {
        pinMode(_config.pin_ref, INPUT);
    }

    currentPulsePos = 0;
    interruptEnabled = true;
    can_ref_auto = false;
    currentDirection = motorDirection::NONE;
    can_slow_start = false;
}

void motor::motorInterrupt() {
    if (!interruptEnabled) {
        return;
    }

    currentPulsePos += isr_increment;

    if (_config.ref_auto && can_ref_auto) {
        int state = digitalRead(_config.pin_ref);
        if (ref_auto_last_state != state) {
            currentPulsePos = _ref_pos_pulses;
#ifdef DEBUG
            Serial.println("ref_auto");
#endif
        }
        ref_auto_last_state = state;
    }

    switch (currentDirection) {
    case motorDirection::NONE:
        motorSpeen(motorDirection::NONE);
        break;
    case motorDirection::POSITIVE:
        if (currentPulsePos >= targetPulsePos) {
            motorSpeen(motorDirection::NONE);
        }
        break;
    case motorDirection::NEGATIVE:
        if (currentPulsePos <= targetPulsePos) {
            motorSpeen(motorDirection::NONE);
        }
        break;
    }

    if (_config.pwm_smoothing) {
        int pulsediffc = abs(currentPulsePos - targetPulsePos);
        int pulsediffs = abs(currentPulsePos - startPulsePos);
        if (pulsediffc <= _pwm_smoothing_pulses) {
            motorSpeen(currentDirection, (float)pulsediffc / _pwm_smoothing_pulses);
        } else if ((pulsediffs <= _pwm_smoothing_pulses) && can_slow_start) {
            motorSpeen(currentDirection, (float)pulsediffs / _pwm_smoothing_pulses);
        } else {
            can_slow_start = false;
        }
    }

#ifdef DEBUG_WTF
    Serial.print("pulsepos: ");
    Serial.println(currentPulsePos);
#endif
}

void motor::reference() {
    interruptEnabled = false;

#ifdef DEBUG
    Serial.println("referencing");
#endif

    if (digitalRead(_config.pin_ref) == _config.ref_positive_state) {
        motorSpeen(motorDirection::POSITIVE);
        while (digitalRead(_config.pin_ref) == _config.ref_positive_state) {}
        motorSpeen(motorDirection::NONE);
    } else {
        motorSpeen(motorDirection::NEGATIVE);
        while (digitalRead(_config.pin_ref) == (!_config.ref_positive_state)) {}
        motorSpeen(motorDirection::NONE);
    }

    delay(250);

    currentPulsePos = _ref_pos_pulses;
    interruptEnabled = true;

    ref_auto_last_state = digitalRead(_config.pin_ref);
    can_ref_auto = true;

#ifdef DEBUG
    Serial.println("referenced");
#endif
}

void motor::print_debug_state() {
    Serial.print("motor direction: ");
    switch (currentDirection) {
    case motorDirection::POSITIVE:
        Serial.println("+");
        break;
    case motorDirection::NEGATIVE:
        Serial.println("-");
        break;
    case motorDirection::NONE:
        Serial.println("/");
        break;
    }

    Serial.print("currentPulsePos: ");
    Serial.println(currentPulsePos);
    Serial.print("currentPos(°): ");
    Serial.println(getCurrentPos());

    Serial.print("targetPulsePos: ");
    Serial.println(targetPulsePos);

    Serial.print("startPulsePos: ");
    Serial.println(startPulsePos);
}

void motor::stop() {
    targetPulsePos = currentPulsePos;
    motorSpeen(motorDirection::NONE);
    pinMode(_config.pin_positive, OUTPUT);
    digitalWrite(_config.pin_positive, LOW);
    pinMode(_config.pin_negative, OUTPUT);
    digitalWrite(_config.pin_negative, LOW);
}

void motor::motorSpeen(motorDirection direction, float pwm = 1) {
    if (pwm < _config.pwm_min_fraction) {
        pwm = _config.pwm_min_fraction;
    }
    if (pwm > 1) {
        pwm = 1;
    }
#ifdef DEBUG_PWM
    Serial.println(pwm);
#endif

    if ((pwm == 1) && (_config.pwm_smoothing)) {
        // make _sure_ pwm is off
        pinMode(_config.pin_positive, OUTPUT);
        digitalWrite(_config.pin_positive, LOW);
        pinMode(_config.pin_negative, OUTPUT);
        digitalWrite(_config.pin_negative, LOW);
    }

    switch (direction) {
    case motorDirection::NONE:
        currentDirection = motorDirection::NONE;
        digitalWrite(_config.pin_positive, LOW);
        digitalWrite(_config.pin_negative, LOW);
#ifdef DEBUG
        Serial.println("motor direction: none");
#endif
        break;
    case motorDirection::POSITIVE:
        currentDirection = motorDirection::POSITIVE;
        isr_increment = 1;
        if (pwm != 1) {
            analogWrite(_config.pin_positive, pwm * 255);
#ifdef DEBUG_INSANE
            Serial.println("motor PWM");
#endif
        } else {
            digitalWrite(_config.pin_positive, HIGH);
        }
        digitalWrite(_config.pin_negative, LOW);
#ifdef DEBUG_INSANE
        Serial.println("motor direction: +");
#endif
        break;
    case motorDirection::NEGATIVE:
        currentDirection = motorDirection::NEGATIVE;
        isr_increment = -1;
        if (pwm != 1) {
            analogWrite(_config.pin_negative, pwm * 255);
#ifdef DEBUG_INSANE
            Serial.println("motor PWM");
#endif
        } else {
            digitalWrite(_config.pin_negative, HIGH);
        }
        digitalWrite(_config.pin_positive, LOW);
#ifdef DEBUG_INSANE
        Serial.println("motor direction: -");
#endif
        break;
    }
}

void motor::moveToPulsePos(int pos) {
    targetPulsePos = pos;
#ifdef DEBUG_WTF
    Serial.print("targetPulsePos: ");
    Serial.println(targetPulsePos);
#endif
    if (pos == currentPulsePos) {
        motorSpeen(motorDirection::NONE);
        return;
    }

    if (currentDirection == motorDirection::NONE) {
        startPulsePos = currentPulsePos;
        can_slow_start = true;
    }

    motorDirection wantedDirection;
    if (pos > currentPulsePos) {
        wantedDirection = motorDirection::POSITIVE;
    } else {
        wantedDirection = motorDirection::NEGATIVE;
    }

    if (currentDirection != wantedDirection) {
        motorSpeen(wantedDirection);
    }
}
