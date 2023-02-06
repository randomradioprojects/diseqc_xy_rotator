#include "motor_basic.h"

void motor_basic::init(struct config config) {
    _config = config;

    if (_config.ref_pullup) {
        pinMode(_config.pin_ref, INPUT_PULLUP);
    } else {
        pinMode(_config.pin_ref, INPUT);
    }

    currentPulsePos = 0;
    interruptEnabled = true;
    currentDirection = motorDirection::NONE;
    ref_auto_last_state = digitalRead(_config.pin_ref);
}

void motor_basic::motorInterrupt() {
    if (!interruptEnabled) {
        return;
    }

    currentPulsePos += isr_increment;

    if (_config.ref_auto) {
        int state = digitalRead(_config.pin_ref);
        if (ref_auto_last_state != state) {
            currentPulsePos = _config.ref_pos;
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

#ifdef DEBUG_WTF
    Serial.print("pulsepos: ");
    Serial.println(currentPulsePos);
#endif
}

void motor_basic::reference() {
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

    currentPulsePos = _config.ref_pos;
    interruptEnabled = true;

    ref_auto_last_state = digitalRead(_config.pin_ref);

#ifdef DEBUG
    Serial.println("referenced");
#endif
}

void motor_basic::print_debug_state() {
    Serial.println("motor_basic {");
    Serial.print("    motor direction: ");
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

    Serial.print("    currentPulsePos: ");
    Serial.println(currentPulsePos);

    Serial.print("    targetPulsePos: ");
    Serial.println(targetPulsePos);
    Serial.println("}");
}

void motor_basic::stop() {
    targetPulsePos = currentPulsePos;
    motorSpeen(motorDirection::NONE);
    pinMode(_config.pin_positive, OUTPUT);
    digitalWrite(_config.pin_positive, LOW);
    pinMode(_config.pin_negative, OUTPUT);
    digitalWrite(_config.pin_negative, LOW);
}

void motor_basic::motorSpeen(motorDirection direction) {
    switch (direction) {
    case motorDirection::NONE:
        currentDirection = motorDirection::NONE;
        move_stop();
#ifdef DEBUG
        Serial.println("motor direction: none");
#endif
        break;
    case motorDirection::POSITIVE:
        currentDirection = motorDirection::POSITIVE;
        isr_increment = 1;
        move_positive();
#ifdef DEBUG_INSANE
        Serial.println("motor direction: +");
#endif
        break;
    case motorDirection::NEGATIVE:
        currentDirection = motorDirection::NEGATIVE;
        isr_increment = -1;
        move_negative();
#ifdef DEBUG_INSANE
        Serial.println("motor direction: -");
#endif
        break;
    }
}

void motor_basic::moveToPulsePos(int pos) {
    targetPulsePos = pos;
    if (pos == currentPulsePos) {
        motorSpeen(motorDirection::NONE);
        return;
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

void motor_basic::move_positive() {
    digitalWrite(_config.pin_positive, HIGH);
    digitalWrite(_config.pin_negative, LOW);
}

void motor_basic::move_negative() {
    digitalWrite(_config.pin_negative, HIGH);
    digitalWrite(_config.pin_positive, LOW);
}

void motor_basic::move_stop() {
    digitalWrite(_config.pin_positive, LOW);
    digitalWrite(_config.pin_negative, LOW);
}
