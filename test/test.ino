#define M_HALL 2
#define M_P    6
#define M_N    5

#define OTHER_HALL 3

// #define USE_PWM

#define PWM_POWER 0.5

volatile uint32_t pulses = 0;
volatile uint32_t other_pulses = 0;

void other_motorint() {
    other_pulses++;
}

void motorint() {
    pulses++;
}

void setup() {
    Serial.begin(9600);
    attachInterrupt(digitalPinToInterrupt(M_HALL), motorint, RISING);
    attachInterrupt(digitalPinToInterrupt(OTHER_HALL), other_motorint, RISING);
    pinMode(M_P, OUTPUT);
    pinMode(M_N, OUTPUT);

    digitalWrite(M_N, LOW);
    digitalWrite(M_P, LOW);
}

static unsigned long lastTime;
static String buffer;

void loop() {
    if ((millis() - lastTime) > 1000) {
        lastTime = millis();
        Serial.print("main,other -> ");
        Serial.print(pulses);
        Serial.print(",");
        Serial.println(other_pulses);
    }
    while (Serial.available() > 0) {
        char rx = Serial.read();
        buffer += rx;
        if (!((rx == '\n') || (rx == '\r'))) {
            continue;
        }
        if (buffer.startsWith("POS")) {
#ifdef USE_PWM
            analogWrite(M_P, PWM_POWER * 255);
            analogWrite(M_N, 0 * 255);
#else
            digitalWrite(M_P, HIGH);
            digitalWrite(M_N, LOW);
#endif
        } else if (buffer.startsWith("NEG")) {
#ifdef USE_PWM
            analogWrite(M_P, 0 * 255);
            analogWrite(M_N, PWM_POWER * 255);
#else
            digitalWrite(M_P, LOW);
            digitalWrite(M_N, HIGH);
#endif
        } else if (buffer.startsWith("KILL")) {
#ifdef USE_PWM
            analogWrite(M_P, 0 * 255);
            analogWrite(M_N, 0 * 255);
#else
            digitalWrite(M_P, LOW);
            digitalWrite(M_N, LOW);
#endif
        } else if (buffer.startsWith("RESET")) {
#ifdef USE_PWM
            analogWrite(M_P, 0 * 255);
            analogWrite(M_N, 0 * 255);
#else
            digitalWrite(M_P, LOW);
            digitalWrite(M_N, LOW);
#endif
            pulses = 0;
            other_pulses = 0;
        }
        buffer = "";
    }
}
