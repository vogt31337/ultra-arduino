// Created By Michael Dixon (Ultrasonic2) Auckland New Zealand

//Battery + "Bucked" down > 5ish volts in at +5
//Battery - > GND

//Trigger > D2

// Servo Signal (PULSE) > D9
// Servo Signal (PULSE) > D10

// trimpot > A7






//----SETTINGS YOU CAN CHANGE
int BatteryS = 3 ; //2= 2s, 3= 3s, 4 = 4s 5 = 5s
int MotorKV = 3000; // motors KV. enter your kv here


int EnablePot =1 ; // 1 = yes 0 = no
int InveretPotDirection = 0; // 1 = yes  0 =no

float MaxServo = 2000; // Reduce this if you want to electronically Limit your RPM / FPS .. 1000 being 0% 2000 being 100%



// Don't change the below setting 

#include <Servo.h>
Servo myservo; 
Servo myservo2; 
//accelerate time setting
float RiseToTime = 0; //ms. Start with 200ms for 4s /// not used anymore
int RevPulled; 
int PreviousFlyWheel = 0; //off
float MinServo = 1000;
float PotServo = 1300;
float MinBelhiServo = 1040;
float MaxBelhiServo = 1960;
float CurrentServoTime = MinServo;
float TimefromTriggerPulled;
float TimefromTriggerchanged;
//Brake time setting
float FallToTime = 6000; //ms IF you have a bLHeli_32 OR S set this to 1000
float TimeFromLastRev;
float TimeFromTrigReleaseMS;
float DiffBetweenBLMinMax;
float MStoTrottleGain;
float temp1;
float temp2;
float temp3;
int CurrentFlyWheel;
float PercentofFallToTime;
float TimefromBoot;
float MaxServoOriginal;
int CurrentPot;
float t1;
float t2;
int DebounceTime;
int DebounceNum;
float DebounceArray[8];
float DebounceArrayTime[8];
String  RevTrigger;
float MotorRPM ;
float GetNewFalltoTime;

//

void setup() {
  MaxServoOriginal = MaxServo;
   myservo.attach(9);
   myservo2.attach(10);
   pinMode(2, INPUT_PULLUP);
   digitalWrite(7,LOW);
   Serial.begin(57600); //hum
 //  myservo.write(MinServo);
  myservo.write(1000);
  myservo2.write(1000);
  delay(9000);
  CurrentPot = MaxServo;
  //TimeFromTrigReleaseMS  = FallToTime  ; 
  RevTrigger = "RevNotPulled";
  TimeFromLastRev =1;
  MotorRPM = BatteryS * MotorKV * 3.7;
  temp1 = (MotorRPM / 33300) * 100;
  FallToTime =  (FallToTime / 100) * temp1 ;
}

void Buttonsstate()// de bounce 
{
 DebounceTime = 30;
  DebounceNum = 0;

  DebounceArray[DebounceNum] = digitalRead(2);// -- D2 Rev

  if (DebounceArray[DebounceNum] != DebounceArray[(DebounceNum + 1)] ) 
  {
    if (DebounceArrayTime[DebounceNum] <= TimefromBoot) 
    {
      DebounceArrayTime[DebounceNum] = TimefromBoot + DebounceTime; 
      DebounceArray[(DebounceNum + 1)] = DebounceArray[DebounceNum]; 
    }
  }

  if (DebounceArray[DebounceNum + 1] == LOW )
  {
    RevTrigger = "RevPulled";
    //digitalWrite(13, HIGH);
   
  }
  else
  {
    RevTrigger = "RevNotPulled";
    //digitalWrite(13, LOW);
  }

  
}// buttonState




void RevDown()// RevDown 
{



    

  TimeFromTrigReleaseMS = TimefromBoot - TimeFromLastRev;

  if (FallToTime > 0) {

  temp1 =  MaxBelhiServo - MinBelhiServo;
  temp2 = MaxServo - 1000;
  temp3 = temp2 / temp1 * 100;

 GetNewFalltoTime = FallToTime / 100 * temp3;

 if (GetNewFalltoTime < 2000)
 {
  GetNewFalltoTime = 2000;
  }
    
    if (TimeFromLastRev > 0)
    {
      PercentofFallToTime = ( TimeFromTrigReleaseMS / GetNewFalltoTime) * 100;


      temp1 =   MaxServo - MinBelhiServo;
      temp1 = (temp1 / 100) * PercentofFallToTime;
      temp2 =   MaxServo - temp1;

      if (temp2 <= (MinBelhiServo+1))
      {
        temp2 = MinServo;
      }
      CurrentServoTime = temp2;
    }
    else
    {
      //Serial.println("Stop");
      CurrentServoTime = 1000 ;
    }
  }
  else
  {
      CurrentServoTime = 1000;
  }


}// RevDown


void RevUp()
{

   TimeFromTrigReleaseMS = TimefromBoot - TimefromTriggerchanged;

if (RiseToTime != 0 ){
  if (TimeFromTrigReleaseMS > 0){
    
         PercentofFallToTime = (TimeFromTrigReleaseMS/RiseToTime) *100;
  
         temp1 =  ((1000/100*PercentofFallToTime));
         temp2 = 1000 + temp1 + temp3;
         
          if (temp2 > MaxServo) {
            temp2 = MaxServo;
            }
         
         CurrentServoTime = temp2; 
  }
  else
  {
     temp3=(CurrentServoTime-1000) ;
  }
  
  }
else{
  
CurrentServoTime = MaxServo;
}

}/// REVUP

void pot(){
  CurrentPot = analogRead(7);
   


  if (InveretPotDirection == 0){
    if (CurrentPot > 1010 ){
      CurrentPot = 2000;
    }
    else{
      CurrentPot = 1000+ CurrentPot ; // yes this is pointless :)
    }
  }
  else{
     if (CurrentPot < 13 ){
      CurrentPot = 2000;
    }
    else{
      CurrentPot = (1023-CurrentPot)+1000 ; // yes this is pointless :)
    }
    
  }
  
 CurrentPot =  mapf(CurrentPot,1000,2000,PotServo,MaxServoOriginal);
}


float mapf(int x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}


void loop() {
  CurrentServoTime = 1000;
  
  TimefromBoot = millis();
  
   Buttonsstate();
  
  if (EnablePot ==1){
    pot();
  }

  MaxServo = CurrentPot;
   
  if  (RevTrigger == "RevPulled" )// pulled, REV UP
  {
    RevUp();
    digitalWrite(13, HIGH);
    TimeFromLastRev = millis();
  }
  else ////// REV DOWN
  {
      RevDown();
      digitalWrite(13, LOW);
      //Stop Motor
    
  }

 //Serial.println(CurrentPot);
 Serial.println(CurrentServoTime);
 myservo.write(CurrentServoTime);
 myservo2.write(CurrentServoTime);

//delay (100); // TEMPPPPPPPPPPPPPPPPPPPP
}

