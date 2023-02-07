// #define DEBUG
// #define DEBUG_PWM
// #define DEBUG_INSANE
// #define DEBUG_WTF

// #define SERIAL_ECHO

#include "mathstuff.h"
#include "motor_simple.h"

/* start configuring here */

#define X_HALL_PIN 2
#define Y_HALL_PIN 3
/*
#define MOTOR_ADVANCED
static struct motor::config config_x = {
    .pin_positive = 6,
    .pin_negative = 5,
    .pin_ref = 4,
    .ref_pos = 0,
    .ppd = 36.9222222222,
    .ref_pullup = true,
    .ref_positive_state = LOW,
    .ref_auto = true,
    .pwm_smoothing = false,
    .pwm_smoothing_deg = 5,
    .pwm_min_fraction = 0.7, // 70% min so motor/driver does not burn
    .min_pos_diff_deg = 0.1,
};

static struct motor::config config_y = {
    .pin_positive = 9,
    .pin_negative = 10,
    .pin_ref = 8,
    .ref_pos = 0,
    .ppd = 36.5777777778,
    .ref_pullup = true,
    .ref_positive_state = LOW,
    .ref_auto = true,
    .pwm_smoothing = false,
    .pwm_smoothing_deg = 5,
    .pwm_min_fraction = 0.7, // 70% min so motor/driver does not burn
    .min_pos_diff_deg = 0.1,
};
*/
static struct motor_simple::config config_x = {
    .pin_positive = 6,
    .pin_negative = 5,
    .pin_ref = 4,
    .ref_pos = 0,
    .ppd = 36,
    .ref_pullup = true,
    .ref_positive_state = LOW,
    .ref_auto = true,
    .min_pos_diff_deg = 0.1,
};

static struct motor_simple::config config_y = {
    .pin_positive = 9,
    .pin_negative = 10,
    .pin_ref = 8,
    .ref_pos = 0,
    .ppd = 36,
    .ref_pullup = true,
    .ref_positive_state = LOW,
    .ref_auto = true,
    .min_pos_diff_deg = 0.1,
};
/* stop here*/

static motor_simple motorX;
static motor_simple motorY;

static void motorXint() {
    motorX.motorInterrupt();
}

static void motorYint() {
    motorY.motorInterrupt();
}

static void moveToAzEl(float az, float el) {
    float x;
    float y;
    /*Serial.print("moving to AZ,EL X,Y");
    Serial.print(az);
    Serial.print(",");
    Serial.print(el);*/
    MBSat_AzEltoXY(az, el, &x, &y);
    /*Serial.print(" ");
    Serial.print(x);
    Serial.print(",");
    Serial.println(y);*/
    motorX.moveToPos(x);
    motorY.moveToPos(y);
}

void setup() {
    Serial.begin(4800);

    motorX.init(config_x);
    motorY.init(config_y);
    attachInterrupt(digitalPinToInterrupt(X_HALL_PIN), motorXint, RISING);
    attachInterrupt(digitalPinToInterrupt(Y_HALL_PIN), motorYint, RISING);

    motorX.reference();
    // motorX.print_debug_state();
    motorY.reference();
    // motorY.print_debug_state();

    moveToAzEl(0, 90); // parking
}

static String buffer;

void loop() {
    while (Serial.available() > 0) {
        char rx = Serial.read();
        buffer += rx;
        if (!((rx == '\n') || (rx == '\r'))) {
            continue;
        }
        if ((buffer.startsWith("AZ")) && (buffer.length() < 26)) {
            char strAz[10];
            char strEl[10];
            if (sscanf(buffer.c_str(), "AZ%s EL%s", strAz, strEl) == 2) {
                float az, el;
                az = strtod(strAz, NULL);
                el = strtod(strEl, NULL);
                moveToAzEl(az, el);
            } else {
                float az, el;
                MBSat_XYtoAzEl(motorX.getCurrentPos(), motorY.getCurrentPos(), &az, &el);
                char str_az[6];
                char str_el[6];
                dtostrf(az, 5, 1, str_az);
                dtostrf(el, 5, 1, str_el);
                Serial.print("AZ");
                Serial.print(str_az);
                Serial.print(" EL");
                Serial.println(str_el);
            }
        } else if (buffer.startsWith("REF")) {
            motorX.reference();
            motorY.reference();
        } else if (buffer.startsWith("DBG")) {
            Serial.println("X:");
            motorX.print_debug_state();
            Serial.println("Y:");
            motorY.print_debug_state();
        } else if (buffer.startsWith("KILL")) {
            motorX.stop();
            motorY.stop();
        }
        buffer = "";
    }
#ifdef MOTOR_ADVANCED
    if (config_x.pwm_smoothing) {
        motorX.pwmadjust();
    }
    if (config_y.pwm_smoothing) {
        motorY.pwmadjust();
    }
#endif
}
