//programator je AVRISP mkII pro Arduino NANO
#include <EEPROM.h>
#include <math.h>


#include <NeoSWSerial.h> //Software serial is used to communicate with axis Y


const double RTD = 180.0 / M_PI;  //constant for conversion radians to degrees
const double DTR = M_PI / 180.0;  //constant for conversion degrees to radians

const char Version[]="mot_controller_x.ino, 2020-06-06/ MBSat "; // axis X controller



//I/O pins definitions
const int LED_RED       =  4; //WEST
const int LED_GREEN     =  7; //EAST
const int HALL_SENS     =  2;
const int RELE_POS      =  8;
const int RELE_NEG      =  9;
const int REF_SW        =  3;

const int swser_rx      =  A3;
const int swser_tx      =  A2;

const int ppd =  36; //pulses per degree, resolution of hall motor rev counter

byte RefSwVal = 0;

float MaxRange; //max angle for the axis, number fom 1 to 90 degree

float oldX, oldY;
float stepd = 1.0;

volatile float curr_pos_float = 0.0;  
volatile float target_pos_float = 0.0;

volatile int curr_pos_puls = 0;
volatile int target_pos_puls = 0;

float ref_offset_degrees= 0;             //Offset from reference switch, degrees, used by axis referencing 
int ref_offset_pulses = 0;  //Offset from reference switch, in pulses, used by axis referencing 

bool referenced = false;

volatile bool move_direction = true;
volatile bool moving = false;

char tempbuf[100];  // keeps the command temporary until CRLF
String buffer;
char swtempbuf[80];  // keeps the command temporary until CRLF
String swser_buffer;

NeoSWSerial swSerial(swser_rx, swser_tx); // RX, TX



// the setup routine runs once when you press reset:

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  swSerial.begin(9600);
  swSerial.listen();
  
  pinMode(REF_SW, INPUT_PULLUP);
  
  pinMode(RELE_POS, OUTPUT);
  pinMode(RELE_NEG, OUTPUT);
 
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT); 

  digitalWrite(RELE_POS, LOW);
  digitalWrite(RELE_NEG, LOW);

  
  ref_offset_pulses = EEPROMReadInt(0);
  ref_offset_degrees=pulses2angle(ref_offset_pulses);
  
  MaxRange = 90.0;
  
  attachInterrupt(digitalPinToInterrupt(HALL_SENS), hall_sens_isr, RISING);

  Serial.println(Version);  
  
  Serial.print("X offset=");
  Serial.print(ref_offset_degrees);
  Serial.print("degrees, ");
  Serial.print(ref_offset_pulses);
  Serial.println("hall pulses ");
}




void loop(){
  while (Serial.available() > 0)
  {
    int tmp;
    char st[20];
    char rx = Serial.read();  // read a single charecter
    char sbtrAz[10]; // string azimut
    char sbtrEl[10]; // string elevation
    char sbtrX[10];  // string azimut
    char sbtrY[10];  // string elevation
    byte pos1,pos2;  // positions of two " " characters in Orbitron or WXtrack message "AZ126.42 EL4.17 UP0 DN0"
    float Az, El, X, Y;
    Az=0.0;
    El=0.0;
    
    buffer += rx;  // add character to the string buffer
    if ((rx == '\n') || (rx == '\r'))
    {
    buffer.toCharArray(tempbuf, 40);
       if (buffer.startsWith("setlimx"))
            {
                sscanf(tempbuf,"setlimx%s",&st); // extract attenuation as floating point number
                MaxRange = strtod(st,NULL);
                Serial.print("swLimX was set to ");
                Serial.println(MaxRange);
            } else
       if (buffer.startsWith("setlimy"))
            {
                sscanf(tempbuf,"setlimy%s",&st); // extract attenuation as floating point number
                float m = strtod(st,NULL);
                //Serial.println("Limit Y sending to Y ");
                swSerial.print("setlim");
                swSerial.println(m);
                //swSerial.listen();
            } else            
       if (buffer.startsWith("posx"))
            {
                sscanf(tempbuf,"posx%s",&st); // extract attenuation as floating point number
                float pos= strtod(st,NULL);
                Serial.print("pos_x ");
                goto_posf(pos);
            } else
       if (buffer.startsWith("posy"))
            {
                sscanf(tempbuf,"posy%s",&st); // extract attenuation as floating point number
                float pos= strtod(st,NULL);
                swSerial.print("pos ");
                swSerial.println(pos);
                //swSerial.listen();
            } else
      if (buffer.startsWith("dgetposx"))
            {
                Serial.print("dpos_x=");
                Serial.println(pulses2angle(curr_pos_puls));
            } else
      if (buffer.startsWith("dgetposy"))
            {
                swSerial.println("dgetpos");
                //swSerial.listen();
            } else                    
      if (buffer.startsWith("stop")) // 
            {
               Serial.println("StopX");
               stop_moving();
               swSerial.println("stop");
            } else
      if (buffer.startsWith("pgetposx")) // 
            {
               Serial.print("ppos_x=");
               Serial.println(curr_pos_puls);
            } else
      if (buffer.startsWith("pgetposy")) // 
            {
               swSerial.println("pgetpos");
               //swSerial.listen();
            } else               
       if (buffer.startsWith("sox"))
            {
                sscanf(tempbuf,"sox%s",&st); // extract attenuation as floating point number
                ref_offset_degrees = strtod(st,NULL);
                ref_offset_pulses = angle2pulses(ref_offset_degrees);
                //EEPROM.write(0, ref_offset_pulses);
                EEPROMWriteInt(0, ref_offset_pulses);
                
                Serial.print("X offset=");
                Serial.print(ref_offset_degrees);
                Serial.print("degrees, ");
                Serial.print(ref_offset_pulses);
                Serial.println("hall pulses ");
            } else 
       if (buffer.startsWith("soy"))
            {
                sscanf(tempbuf,"soy%s",&st); // extract attenuation as floating point number
                float ro = strtod(st,NULL);
                swSerial.print("setoff");
                swSerial.println(ro);
                //swSerial.listen();
            } else 
            if (buffer.startsWith("getoffx")) // send back status of reference switch
            {
                //ref_offset_degrees = pulses2angle(ref_offset_pulses);
                Serial.print("X offset=");
                Serial.print(ref_offset_degrees);
                Serial.print("degrees, ");
                Serial.print(ref_offset_pulses);
                Serial.println ("hall pulses ");  
            } else                       
            if (buffer.startsWith("getoffy")) // send back status of reference switch
            {
              swSerial.println("getoff");
              //swSerial.listen();  
            } else  
      if (buffer.startsWith("gotoref")) // 
            {
               swSerial.println("gotoref");
               go_ref();
               Serial.println("refxDone");
               
            } else
      if (buffer.startsWith("getrswx")) // send back status of reference switch
            {
               RefSwVal   = digitalRead(REF_SW);
               Serial.print("RSW status=");
               Serial.println(RefSwVal);  
            } else  
      if (buffer.startsWith("getrswy")) // send back status of reference switch
            {
               swSerial.print("getrsw");
               //swSerial.listen(); 
            } else       
       if (buffer.startsWith("AZ EL"))//Read command from Orbitron or WXtrack (Easy Comm I protocol: AZ172.08 EL3.34 UP0 DN0)
            {
                //Serial.print("buffer=");
                //Serial.println(buffer);
                pos1 = buffer.indexOf(" ");
                pos2 = buffer.indexOf(" ", pos1+1 );
                
                buffer.toCharArray(tempbuf, 40);
                //substring(string, sub, position, length);
                substring(tempbuf, sbtrAz,  3, pos1-2);
                substring(tempbuf, sbtrEl,  pos1+4, pos2-pos1-3);
                //Serial.println(sbtrAz);
                //Serial.println(sbtrEl); 
                Az =  strtod(sbtrAz,NULL);
                El =  strtod(sbtrEl,NULL);
                
                float Az2, El2;
                MBSat_XYtoAzEl(X, Y, &Az2, &El2);
                
              Serial.print("AZ");
              Serial.print(Az2);
              Serial.print(" EL");
              Serial.println(El2);
                
                //MBSat_AzEltoXY(Az, El, &X, &Y);
                //Serial.print("X=" );
                //Serial.print(X);
                //Serial.print(" Y=" );
                //Serial.print(Y);

                if(abs(X-oldX)>stepd)
                {
                  goto_posf(X);
                  oldX = X;
                }
                if(abs(Y-oldY)>stepd)
                {
                    swSerial.print("pos"); 
                    swSerial.println(Y);
                    oldY = Y;
                }  
                



            }else                            
      if (buffer.startsWith("version")) // Check version of Arduino software
            {
               Serial.println(Version);
               swSerial.println("version");
            } else
      if (buffer.startsWith("help"))
            {
               
                Serial.println("Commands for XY antenna rotor controller");
                Serial.println(" * gotoref                  --move both axes to reference"); 
                Serial.println(" * setlimx90.0, setlimy90.0 --set software limit for given axis");
                Serial.println(" * posx90.0, posy90.0       --move given axis to position");
                Serial.println(" * dgetposx, dgetposy       --return axes positions in degrees");
                Serial.println(" * pgetposx, pgetposy       --return axes positions in pulses");
                Serial.println(" * stop                     --stop both axes moving");
                Serial.println(" * sox-3.0, soy-2.5         --set axes offsets");
                Serial.println(" * getoffx, getoffy         --return actual offsets");
                Serial.println(" * AZ360.0 EL90.0 az el     --position command from WXTrack, angles are Az El");

                
                
            } 
            buffer = "";  //erase buffer for next command                     
    }
  
  }//end while (Serial.available() > 0)
  
  while (swSerial.available() )
  {
    char rx = swSerial.read();
    Serial.print(rx);
  }
  
}//end LOOP


//ISR Routines
void hall_sens_isr()
{
  if(move_direction)
  {
    curr_pos_puls++;
  }
  else
  {
    curr_pos_puls--;
  }
  if(move_direction == true)//jedeme +
  {
    if(curr_pos_puls >= target_pos_puls)
    {
      stop_moving();
    } 
  }else//jedeme - 
  if(curr_pos_puls <= target_pos_puls)
    {
      stop_moving();
    } 

}


//*****Help Routines and Functions*****//

//This function will write a 2 byte integer to the eeprom at the specified address and address + 1
void EEPROMWriteInt(int p_address, int p_value)
     {
     byte lowByte = ((p_value >> 0) & 0xFF);
     byte highByte = ((p_value >> 8) & 0xFF);

     EEPROM.write(p_address, lowByte);
     EEPROM.write(p_address + 1, highByte);
     }

//This function will read a 2 byte integer from the eeprom at the specified address and address + 1
unsigned int EEPROMReadInt(int p_address)
     {
     byte lowByte = EEPROM.read(p_address);
     byte highByte = EEPROM.read(p_address + 1);

     return ((lowByte << 0) & 0xFF) + ((highByte << 8) & 0xFF00);
     }

// C substring function definition
void substring(char s[], char sub[], int p, int l) {
   int c = 0;
   
   while (c < l) {
      sub[c] = s[p+c-1];
      c++;
   }
   sub[c] = '\0';
}

double to_radians(double degrees) {
    return degrees * ( M_PI/180.0);
}

double to_degrees(double radians) {
    return radians * (180.0 / M_PI);
}


void DigiTwist_AzEltoXY(float az, float el, float *x, float *y)
{
  float Xrad, Yrad, Azrad, Elrad;
  //Y = arcsin (sin (AZ) * cos (EL)) 
  Azrad = to_radians(az);
  Elrad = to_radians(el);
  Yrad = asin(sin(Azrad) * cos(Elrad));
  *y=-to_degrees(Yrad);
  //X = arccos (sin (EL) / cos (Y))
  Xrad = acos(sin(Elrad)/ cos(Yrad ));
  if(az>270.0 or  az<90.0){
    Xrad = -Xrad;
  }
  *x = -to_degrees(Xrad);
  
}
//---------------------------------------------------------------------------
void MBSat_AzEltoXY(float az, float el, float *x, float *y)
{
    const float accuracy = 0.01; // rotor accuracy in degrees

    if(el <= accuracy)
        *x = 90.0;
    else if(el >= (90.0 - accuracy))
        *x = 0.0;
    else
        *x = -atan(-cos(az * DTR) / tan(el * DTR)) * RTD;

        *y = -asin(sin(az * DTR) * cos(el * DTR)) * RTD;
}
//---------------------------------------------------------------------------
void MBSat_XYtoAzEl(float X, float Y, float *az, float *el)
{
    float AZIM, ELEV;
    float tanX, tanX2, sinY2, sinY, A, B;

    tanX = tan(X * DTR);
    tanX2 = tanX * tanX;
    sinY  = sin(Y * DTR);
    sinY2 = sinY * sinY;

    // B = cos Az
    B = sqrt(-1.0 * ((tanX2 * sinY2 - tanX2) / (sinY2 + tanX2)));
    // A = cos El
    A  = sinY / sqrt(1.0 - B*B) ;

    // cos Az = B and therefore
    AZIM = acos(B) * RTD;
    //cos El = A and therefore
    ELEV = acos(A) * RTD;

    // make sure we got some return value (even if it is wrong)
    *az = AZIM;
    *el = ELEV;

    if(ELEV > 0) {
        if(ELEV > 90)
            ELEV = 180.0 - ELEV;
        else {
            ELEV = 0;
            *el = ELEV;
        }
    }

    // determine quadrant from X and Y angle, then determine correct azimuth and elevation
    if(X < 0) {
        if(Y < 0) // quadrant = 4;
            *az = 360.0 - AZIM;
        else if(Y > 0) // quadrant = 1;
            *az = AZIM;
        else { // quadrant = 14;
            *az = 0;
            *el = 90.0 - fabs(X);
        }
    }
    else if(X > 0) {
        if(Y < 0) // quadrant = 3;
            *az = 180.0 + AZIM;
        else if(Y > 0) // quadrant = 2;
            *az = 180.0 - AZIM;
        else {
            // quadrant = 23;
            *az = 180;
            *el = 90.0 - fabs(X);
        }
    }
    else { // X = 0, antenna move only in E-W direction and back
        if(Y > 0) // quadrant = 12;
            *az = AZIM;
        else if(Y < 0) // quadrant = 34;
            *az = AZIM;
        else {
            // quadrant= 0;
            *az = 0;
            *el = 90;
        }
    }

}


void goto_posf(float target_float)
{
   target_pos_float = target_float;
   
   if (target_float <-MaxRange) 
   {
      target_pos_float=-MaxRange;
      Serial.print("Target limited to ");
      Serial.println(target_pos_float);
   }else
   
   if (target_float > MaxRange) 
   {
    target_pos_float= MaxRange;
    Serial.print("Target limited to ");
    Serial.println(target_pos_float);
   }
   
   target_pos_puls = angle2pulses(target_pos_float); 

   Serial.print("TargetPosPulses=");
   Serial.println(target_pos_puls);
   
   if(moving==false)//stojime
   { 
      if(referenced)
      {
         if(curr_pos_puls < target_pos_puls)
         {
            move_direction = true;
            move_pos();
         }
         else
         {
            move_direction = false;
            move_neg();
         }
       }else
       {
         Serial.println("mes::Unable to move, do referencing first!!!");
         go_ref();
       }
   }
   else //jedeme
   {
     if(move_direction == true)//jedeme +
     {
        if(curr_pos_puls > target_pos_puls)//mame jet -
        {
          stop_moving();
          delay(150);
          //move_direction = false;
          move_neg();//jed -
        }
     }
     else  //jedeme -
     {
        if(curr_pos_puls < target_pos_puls)//mame jet +
        {
          stop_moving();
          delay(150);
          //move_direction = true;
          move_pos();//jed +
        }
     }
   }

   
}



int angle2pulses(float ang)
{
  int angle_pulses = ang * ppd;
  return angle_pulses;
}

float pulses2angle(int pulses)
{
  return pulses/float(ppd);
}



void go_ref()
{
  stop_moving();
  referenced = false;
  detachInterrupt(digitalPinToInterrupt(HALL_SENS));
  RefSwVal=digitalRead(REF_SW);
  if(RefSwVal == 0) //ref sw neni zmacknuty, jedeme -
  {
    move_pos();
    while(RefSwVal ==0)
    {
      RefSwVal=digitalRead(REF_SW);
    }
    stop_moving();
    delay(250);
    
  }
  else //ref sw je zmacknuty, jedeme +
  {
    move_neg();
    while(RefSwVal==1)
    {
      RefSwVal=digitalRead(REF_SW);
    }
    stop_moving();
    delay(250);
    
    
  }
  curr_pos_puls = ref_offset_pulses;
  attachInterrupt(digitalPinToInterrupt(HALL_SENS), hall_sens_isr, RISING); 
  referenced = true;
  //goto_posf(0.0);//after referencing goto 0 degree
  Serial.flush();
  swSerial.flush();
  
}

void move_pos()
{
  move_direction = true;
  digitalWrite(RELE_NEG, LOW);
  digitalWrite(RELE_POS, HIGH);
  digitalWrite(LED_RED, HIGH);
  moving =true;
}

void move_neg()
{ 
  move_direction = false;
  digitalWrite(RELE_POS, LOW);
  digitalWrite(RELE_NEG, HIGH);
  digitalWrite(LED_GREEN, HIGH);
  moving =true;
}

void stop_moving()
{
  digitalWrite(RELE_NEG, LOW);
  digitalWrite(RELE_POS, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_RED, LOW);
  moving =false;
}
