class motor {
public:
  void init(int pinPositive, int pinNegative, int referenceSwitch, float referencePosition, int ppd) {
    _pinPositive = pinPositive;
    _pinNegative = pinNegative;
    _referenceSwitch = referenceSwitch;
    _referencePosition = referencePosition;
    _ppd = ppd;

    pinMode(_pinPositive, OUTPUT);
    pinMode(_pinNegative, OUTPUT);
    pinMode(_referenceSwitch, INPUT);
    
    currentPulsePos = 0;
    interruptEnabled = true;
  }

  /*void setOffset(int offsetPulses) {
    _offsetPulses = 0;
  }*/
  
  void motorInterrupt() {
    if(!interruptEnabled) {
      return;
    }
    switch(currentDirection) {
      case motorDirection::NONE:
        Serial.println("what the fuck is wrong with your setup, the motors aint supposed to be moving rn breh");
        motorSpeen(motorDirection::NONE);
        return;
        break;
      case motorDirection::POSITIVE:
        currentPulsePos++;
        if(currentPulsePos >= targetPulsePos) {
          motorSpeen(motorDirection::NONE);
        }
        break;
      case motorDirection::NEGATIVE:
        currentPulsePos--;
        if(currentPulsePos <= targetPulsePos) {
          motorSpeen(motorDirection::NONE);
        }
        break;
    }
  }

  void reference() {
    interruptEnabled = false;
    
    /* mnux motor */
    if(digitalRead(_referenceSwitch)) {
      Serial.println("already ref");
    }
    motorSpeen(motorDirection::NEGATIVE);
    while(!digitalRead(_referenceSwitch)) { }
    motorSpeen(motorDirection::NONE);
    
    /* felix motor */
    /*if(digitalRead(_referenceSwitch) == 0) {
      motorSpeen(motorDirection::POSITIVE);
      while(!digitalRead(_referenceSwitch)) {}
      motorSpeen(motorDirection::NONE);
    } else {
      motorSpeen(motorDirection::NEGATIVE);
      while(!digitalRead(_referenceSwitch)) {}
      motorSpeen(motorDirection::NONE);
    }*/
    
    delay(250);
    
    currentPulsePos = degToPulses(_referencePosition);
    interruptEnabled = true;
  }

  void moveToPos(float posDegrees) {
    moveToPulsePos(degToPulses(posDegrees));
  }

  float getCurrentPos() {
    return pulsesToDeg(currentPulsePos);
  }

  void moveWait() {
    while((currentDirection == motorDirection::POSITIVE) || (currentDirection == motorDirection::NEGATIVE)) {}
  }

  
private:
  /* stuff */
  enum class motorDirection { NONE, POSITIVE, NEGATIVE };

  /* constants */
  float _referencePosition;
  int _ppd;
  /* pins */
  int _pinPositive;
  int _pinNegative;
  int _referenceSwitch;

  /* runtime idfk stuff */
  int currentPulsePos;
  int targetPulsePos;
  bool interruptEnabled;
  motorDirection currentDirection = motorDirection::NONE;
  //int _offsetPulses = 0; // do we really need this???
  
  void motorSpeen(motorDirection direction) {
    switch(direction) {
      case motorDirection::NONE:
        currentDirection = motorDirection::NONE;
        digitalWrite(_pinPositive, LOW);
        digitalWrite(_pinNegative, LOW);
        Serial.println("movingn't");
        break;
      case motorDirection::POSITIVE:
        currentDirection = motorDirection::POSITIVE;
        digitalWrite(_pinPositive, HIGH);
        digitalWrite(_pinNegative, LOW);
        Serial.println("moving positive");
        break;
      case motorDirection::NEGATIVE:
        currentDirection = motorDirection::NEGATIVE;
        digitalWrite(_pinPositive, LOW);
        digitalWrite(_pinNegative, HIGH);
        Serial.println("moving negative");
        break;
    }
  }

  int degToPulses(float deg) {
    return deg * _ppd;
  }

  float pulsesToDeg(int pulses) {
    return pulses/((float)_ppd);
  }
  
  void moveToPulsePos(int pos) {
    targetPulsePos = pos;
    if(pos == currentPulsePos) {
      motorSpeen(motorDirection::NONE);
      Serial.print("no need to move, already at");
      Serial.print(currentPulsePos);
      Serial.print("want ");
      Serial.println(pos);
      return;
    }
    interruptEnabled = true;
    if(pos > currentPulsePos) {
      Serial.print("moving positive, at ");
      Serial.print(currentPulsePos);
      Serial.print("want ");
      Serial.println(pos);
      motorSpeen(motorDirection::POSITIVE);
    } else {
      Serial.print("moving negative, at ");
      Serial.print(currentPulsePos);
      Serial.print("want ");
      Serial.println(pos);
      motorSpeen(motorDirection::NEGATIVE);
    }
  }
  
};

motor motorX;
motor motorY;

void motorXint() {
  motorX.motorInterrupt();
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  motorX.init(5, 4, 3, -90, 38);
  Serial.println("1");
  attachInterrupt(digitalPinToInterrupt(2), motorXint, RISING);
  Serial.println("2");
  motorX.reference();
  Serial.println("referenced!");
  Serial.print("current position: ");
  Serial.println(motorX.getCurrentPos());
  motorX.moveToPos(1);
  motorX.moveWait();
  Serial.print("current position: ");
  Serial.println(motorX.getCurrentPos());
  motorX.reference();
  Serial.println("referenced!");
  Serial.print("current position: ");
  Serial.println(motorX.getCurrentPos());
}

void loop() {
  // put your main code here, to run repeatedly:

}
