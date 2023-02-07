#include "motor_basic.h"

#define PANIC(str, condition)                   \
    do {                                        \
        if (condition) {                        \
            panic(str " -- " #condition, this); \
        }                                       \
    } while (0)

static void panic(const char *str, motor_basic *mot) {
    mot->print_debug_state();
    Serial.print("panic: ");
    Serial.println(str);
    String buffer;
    while (Serial.available() > 0) {
        char rx = Serial.read();
        buffer += rx;
        if (!((rx == '\n') || (rx == '\r'))) {
            continue;
        }
        if (buffer.startsWith("OK")) {
            break;
        }
        mot->print_debug_state();
        Serial.print("panic: ");
        Serial.println(str);
        buffer = "";
    }
}

void motor_basic::init(struct config config) {
    _config = config;

    if (_config.ref_pullup) {
        pinMode(_config.pin_ref, INPUT_PULLUP);
    } else {
        pinMode(_config.pin_ref, INPUT);
    }

    pinMode(_config.pin_positive, OUTPUT);
    pinMode(_config.pin_negative, OUTPUT);

    currentPulsePos = 0;
    interruptEnabled = true;
    currentDirection = motorDirection::NONE;
    ref_auto_last_state = digitalRead(_config.pin_ref);
    isr_increment = 0;
}

void motor_basic::motorInterrupt() {
    if (!interruptEnabled) {
        return;
    }

    currentPulsePos += isr_increment;

    if (_config.ref_auto) {
        int state = digitalRead(_config.pin_ref);
        if (ref_auto_last_state != state) {
            PANIC("out of range autoref", abs(currentPulsePos) > 100);
            currentPulsePos = _config.ref_pos;
        }
        ref_auto_last_state = state;
    }

    switch (currentDirection) {
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
}

void motor_basic::reference() {
    interruptEnabled = false;

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
    ref_auto_last_state = digitalRead(_config.pin_ref);

    interruptEnabled = true;
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
        Serial.println("X");
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
    digitalWrite(_config.pin_positive, LOW);
    digitalWrite(_config.pin_negative, LOW);
}

void motor_basic::motorSpeen(motorDirection direction) {
    // safety to make sure isr_increment does not get set while the motor is moving in a different direction
    if (currentDirection != direction && currentDirection != motorDirection::NONE) {
        move_stop();
        delay(50);
    }

    currentDirection = direction;
    switch (direction) {
    case motorDirection::NONE:
        move_stop();
        break;
    case motorDirection::POSITIVE:
        isr_increment = 1;
        move_positive();
        break;
    case motorDirection::NEGATIVE:
        isr_increment = -1;
        move_negative();
        break;
    }
}

void motor_basic::moveToPulsePos(int pos) {
    targetPulsePos = pos;
    if (pos == currentPulsePos) {
        motorSpeen(motorDirection::NONE);
        return;
    }

    motorDirection wantedDirection = motorDirection::NEGATIVE;
    if (pos > currentPulsePos) {
        wantedDirection = motorDirection::POSITIVE;
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
