//programator je AVRISP mkII pro Arduino NANO
#include <EEPROM.h>

//CircularBuffer<float, 10> posfifo;

#define DEBUG

char Version[]="mot_controller_y.ino, 2020-06-06/ MBSat "; // Version


//I/O pins definitions
const int BUTTON_WEST   =  6;     
const int BUTTON_EAST   =  5; 
const int LED_RED       =  4; //WEST
const int LED_GREEN     =  7; //EAST
const int HALL_SENS     =  2;
const int RELE_POS      =  8;
const int RELE_NEG      =  9;
const int REF_SW        =  3;

const int ppd =  36; //pulses per degree, resolution of hall motor rev counter

byte RefSwVal = 0;

float MaxRange; //max angle for the axis, number fom 1 to 90 degree

volatile float curr_pos_float = 0.0;  
volatile float target_pos_float = 0.0;

volatile int curr_pos_puls = 0;
volatile int target_pos_puls = 0;

float ref_offset_degrees= 0;             //Offset from reference switch, degrees, used by axis referencing 
int ref_offset_pulses = 0;  //Offset from reference switch, in pulses, used by axis referencing 

bool referenced = false;

volatile bool move_direction = true;
volatile bool moving = false;

char tempbuf[80];  // keeps the command temporary until CRLF
String buffer;


// the setup routine runs once when you press reset:

void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  
  pinMode(BUTTON_EAST, INPUT_PULLUP);
  pinMode(BUTTON_WEST, INPUT_PULLUP);
  pinMode(REF_SW, INPUT_PULLUP);
  
  pinMode(RELE_POS, OUTPUT);
  pinMode(RELE_NEG, OUTPUT);
 
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT); 

  digitalWrite(RELE_POS, LOW);
  digitalWrite(RELE_NEG, LOW);

 // EEPROM.write(0, ref_offset_pulses);
  //EEPROM.write(2, MaxRange);
  
  ref_offset_pulses = EEPROMReadInt(0);
  ref_offset_degrees=pulses2angle(ref_offset_pulses);
  
  //EEPROM.get(2, MaxRange);
  MaxRange = 90.0;
  
  attachInterrupt(digitalPinToInterrupt(HALL_SENS), hall_sens_isr, RISING);
  
  delay(250);

  Serial.println(Version);
  Serial.print("Y offset=");
  Serial.print(ref_offset_degrees);
  Serial.print("degrees, ");
  Serial.print(ref_offset_pulses);
  Serial.println("hall pulses ");
  
}




void loop(){
/*
  int BtnWestVal = digitalRead(BUTTON_WEST);
  int BtnEastVal = digitalRead(BUTTON_EAST); 
  

  if(BtnWestVal== LOW) {
     digitalWrite(RELE_NEG, LOW);
     digitalWrite(RELE_POS, HIGH);
     digitalWrite(LED_RED, HIGH);
     
  }else
  {
   digitalWrite(RELE_POS, LOW);
   digitalWrite(LED_RED, LOW);
   if(BtnEastVal== LOW) 
    {
      digitalWrite(RELE_POS, LOW);
      digitalWrite(RELE_NEG, HIGH);
      digitalWrite(LED_GREEN, HIGH);
    }else
    {
      
      digitalWrite(RELE_NEG, LOW);
      digitalWrite(LED_GREEN, LOW);
    }
  }
*/

  while (Serial.available() > 0)
  {
    int tmp;
    char st[20];
    char rx = Serial.read();  // read a single charecter
    buffer += rx;  //add character to the string buffer
    //Serial.print(rx);
    if ((rx == '\n') || (rx == '\r'))
    {
    buffer.toCharArray(tempbuf, 40);
       if (buffer.startsWith("setlim"))
            {
                sscanf(tempbuf,"setlim%s",&st); // extract attenuation as floating point number
                MaxRange = strtod(st,NULL);
                Serial.print("swLimY was set to ");
                Serial.println(MaxRange);
            } else
       if (buffer.startsWith("pos"))
            {
                sscanf(tempbuf,"pos%s",&st); // extract attenuation as floating point number
                float pos= strtod(st,NULL);
                Serial.println("pos_y");
                goto_posf(pos);
            } else
      if (buffer.startsWith("dgetpos"))
            {
                Serial.print("dpos_y=");
                Serial.println(pulses2angle(curr_pos_puls));
            } else      
      if (buffer.startsWith("stop")) // 
            {
               Serial.println("StopY");
               stop_moving();
            } else
      if (buffer.startsWith("pgetpos")) // 
            {
               Serial.print("ppos_y=");
               Serial.println(curr_pos_puls);
            } else  
       if (buffer.startsWith("setoff"))
            {
                sscanf(tempbuf,"setoff%s",&st); // extract attenuation as floating point number
                ref_offset_degrees = strtod(st,NULL);
                ref_offset_pulses = angle2pulses(ref_offset_degrees);
                EEPROMWriteInt(0, ref_offset_pulses);
                
                Serial.print("Y offset set =");
                Serial.print(ref_offset_degrees);
                Serial.print("degrees, ");
                Serial.print(ref_offset_pulses);
                Serial.println("pulses ");
            } else 
            if (buffer.startsWith("getoff")) // send back status of reference switch
            {   delay(100);
                ref_offset_degrees = pulses2angle(ref_offset_pulses);
                Serial.print("Y offset=");
                Serial.print(ref_offset_degrees);
                Serial.print("degrees, ");
                Serial.print(ref_offset_pulses);
                Serial.println ("hall pulses ");  
            } else                       
      if (buffer.startsWith("gotoref")) // 
            {
               
               go_ref();
               Serial.println("refyDone");
            } else
      if (buffer.startsWith("getrsw")) // send back status of reference switch
            {
               RefSwVal   = digitalRead(REF_SW);
               Serial.print("RSW status=");
               Serial.println(RefSwVal);  
            } else            
      if (buffer.startsWith("version")) // Check version of Arduino software
            {
              Serial.println(Version);
            } 
            buffer = "";  //erase buffer for next command                     
    }
  
  }//end while (Serial.available() > 0)
  

  
}//end LOOP


//ISR Routines
void hall_sens_isr()
{
  #ifdef DEBUG
  curr_pos_puls = target_pos_puls;
  #endif
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




//OTHER Routines and Functions
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
         Serial.println("mes::Unable to move, do referencing first!");
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
