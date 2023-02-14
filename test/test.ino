#define M_HALL 2
#define M_P    6
#define M_N    5
#define M_REF  69

// #define USE_PWM
#define PWM_POWER 0.5

#define REFZERO

volatile uint32_t pulses = 0;

void motorint() {
    pulses++;
}

int ref_state = 0;

void setup() {
    Serial.begin(9600);
    attachInterrupt(digitalPinToInterrupt(M_HALL), motorint, RISING);
    pinMode(M_P, OUTPUT);
    pinMode(M_N, OUTPUT);
    pinMode(M_REF, INPUT);

    digitalWrite(M_N, LOW);
    digitalWrite(M_P, LOW);

    ref_state = digitalRead(M_REF);
}

static unsigned long lastTime;
static String buffer;

void loop() {
#ifdef REFZERO
    int current_ref_state = digitalRead(M_REF);
    if (current_ref_state != ref_state) {
        pulses = 0;
        ref_state = current_ref_state;
    }
#endif

    if ((millis() - lastTime) > 1000) {
        lastTime = millis();
        Serial.print("count -> ");
        Serial.println(pulses);
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
        }
        buffer = "";
    }
}
