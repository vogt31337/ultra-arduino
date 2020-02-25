/*
   Ultra Arduino is a program to control functions of a modified Nerf Stryfe blaster.
   Mods and original source are from:
   https://www.thingiverse.com/thing:2460171/files
   https://www.thingiverse.com/thing:3029346

   Created By Michael Dixon (Ultrasonic2) Auckland New Zealand
   Modified by vogt31337

   Needed external libs:

*/

#define SERIAL_DEBUG true

// ---- SETTINGS YOU CAN CHANGE ----
#define POT_PIN 1
#define BAT_PIN 0
#define FIRE_PIN 5
#define OPTO_PIN 8

// -- Trigger Pin --
#define TRG_PIN 4
#define TRG_LED 13

// -- Fire Selector --
#define FIRE_SELECT_PIN1 3
#define FIRE_SELECT_PIN2 2

// -- BLDC Rev System --
//#define REV_PIN 6
#define REV_LED 12
#define REV_SERVO_PIN 9

// -- Delay Section --
#define DEBOUNCE_DELAY 50l
#define FIRE_ON_DURATION 30
#define FIRE_OFF_DURATION 50

// -- Mag Sesnor Section --
#define MAG_SENSE 1
#define MAG_SENSE_PIN 11
#define MAG_LED_PIN 10

//#define INVERT_POT_DIRECTION true  // uncomment to inverse your potentiometer
float MaxServo = 180;             // Reduce this if you want to electronically Limit your RPM / FPS .. 0 being 0% 180 being 100%
float MinServo = 0;

// ---- Don't change the below setting ----

#ifdef MAG_SENSE
#include <Wire.h>
#include "TCS34725.h"
TCS34725 tcs;
unsigned long mag_case = 0;
#endif

#ifdef REV_PIN
#include <Servo.h>
Servo rev_servo;
bool RevTrigger = false;
#endif

unsigned long lastDebounceTime[5] = {0, 0, 0, 0, 0};
int lastButtonState[5] = {LOW, LOW, LOW, LOW, LOW};

bool Trigger = false;
int fire_case = 0;

int whichADCtoRead = 0; // 0 = Pot, 1 = Battery
int max_pot = MaxServo;
float battery_level = 0.0f;
unsigned int fire_count = 0;
unsigned long fire_event = 0;

#ifdef OPTO_PIN
unsigned long opto_timing = 0;
float opto_fps = 0.0f;
#endif

void setup()
{
  Serial.begin(115200);
  pinMode(TRG_PIN, INPUT_PULLUP);
  pinMode(POT_PIN, INPUT);
  pinMode(FIRE_PIN, OUTPUT);
  pinMode(TRG_LED, OUTPUT);
  pinMode(FIRE_SELECT_PIN1, INPUT_PULLUP);
  pinMode(FIRE_SELECT_PIN2, INPUT_PULLUP);
#ifdef MAG_SENSE
  pinMode(MAG_SENSE_PIN, INPUT_PULLUP);
  pinMode(MAG_LED_PIN, OUTPUT);
  digitalWrite(MAG_LED_PIN, HIGH);
#endif

#ifdef OPTO_PIN
  pinMode(OPTO_PIN, INPUT);
  Serial.println(F("Opto Option Enabled!"));
#endif

  ADCSRA  = bit(ADEN);
  ADCSRA |= bit(ADPS0) | bit(ADPS1) | bit(ADPS2);
  ADMUX   = bit(REFS0) | /*bit(REFS1) |*/ (POT_PIN & 0x07);
  delay(2);

#ifdef REV_PIN
  pinMode(REV_PIN, INPUT_PULLUP);
  pinMode(REV_LED, OUTPUT);
  rev_servo.attach(REV_SERVO_PIN);
  rev_servo.write(MinServo);
  Serial.println(F("BLDC Option Enabled!"));
#endif

#ifdef MAG_SENSE
  Wire.begin();
  if (!tcs.attach(Wire))
    Serial.println("Error TCS34725 not found.");

  tcs.power(true);
  tcs.integrationTime(33); // ms
  tcs.gain(TCS34725::Gain::X01);
#endif
}

/**
   Function to read button states of
   Rev and Trigger
*/
void Buttonsstate(unsigned long time_millis)
{
#ifdef SERIAL_DEBUG
  Serial.print(F(" TIME: "));
  Serial.print(time_millis);
#endif

#ifdef REV_PIN
  int rev_state = digitalRead(REV_PIN);

  if (rev_state != lastButtonState[0]) {
    lastDebounceTime[0] = time_millis;
    lastButtonState[0] = rev_state;
  }

  if ((time_millis - lastDebounceTime[0]) > DEBOUNCE_DELAY) {
    RevTrigger = (bool) rev_state;
    digitalWrite(REV_LED, rev_state);
  }
#endif

  int trg_state = digitalRead(TRG_PIN);

  if (trg_state != lastButtonState[1]) {
    lastDebounceTime[1] = time_millis;
    lastButtonState[1] = trg_state;
  }

  int fr1_state = digitalRead(FIRE_SELECT_PIN1);

  if (fr1_state != lastButtonState[2]) {
    lastDebounceTime[2] = time_millis;
    lastButtonState[2] = fr1_state;
  }

  int fr2_state = digitalRead(FIRE_SELECT_PIN2);

  if (fr2_state != lastButtonState[3]) {
    lastDebounceTime[3] = time_millis;
    lastButtonState[3] = fr2_state;
  }

#ifdef MAG_SENSE_PIN
  int mag_state = digitalRead(MAG_SENSE_PIN);

  if (mag_state != lastButtonState[4]) {
    lastDebounceTime[4] = time_millis;
    lastButtonState[4] = mag_state;
  }
#endif

#ifdef OPTO_PIN
  int opto_state = digitalRead(OPTO_PIN);

  if (opto_state) {
    opto_timing = micros();
  } else {
    opto_timing = micros() - opto_timing;
    // divide size of dart by timing to get fps
    // dart size is 2.84in = 0.236667ft
    // one second has 1.000.000us
    opto_fps = 236667.6f / opto_timing;
  }
#endif

  if ((time_millis - lastDebounceTime[1]) > DEBOUNCE_DELAY) {
    Trigger = (bool) trg_state;
  }

  if ((time_millis - lastDebounceTime[2]) > DEBOUNCE_DELAY) {
    if (fr1_state) {
      fire_case |= (1 << 0);
    } else {
      fire_case &= ~(1 << 0);
    }
  }

  if ((time_millis - lastDebounceTime[3]) > DEBOUNCE_DELAY) {
    if (fr2_state) {
      fire_case |= (1 << 1);
    } else {
      fire_case &= ~(1 << 1);
    }
  }

#ifdef MAG_SENSE
  if ((time_millis - lastDebounceTime[4]) > DEBOUNCE_DELAY) {
    if (mag_state) {
      mag_case = time_millis + 200;
      digitalWrite(MAG_LED_PIN, LOW);
    } else {
      mag_case = 0;
      digitalWrite(MAG_LED_PIN, HIGH);
    }
  }
#endif
} // buttonState

/*
   Get potentiometer value, as arduino is 10bit
   shift right by 2 bits (divide by 4) which reduces noise greatly
*/
int pot(int current_pot) {
#ifdef INVERT_POT_DIRECTION
  return map(current_pot, 1023, 0, MinServo, MaxServo);
#else
  return map(current_pot, 0, 1023, MinServo, MaxServo);
#endif
}

// ------------------------------ main loop ------------------------------

void loop() {
  unsigned long time_millis = millis();
  Buttonsstate(time_millis);

  // ADC Section
  //int tADCSRA = ADCSRA;
  if (bit_is_clear(ADCSRA, ADSC)) {
    uint8_t low = ADCL;
    uint8_t high = ADCH;
    int value = (high << 8) | low;
    switch (whichADCtoRead) {
      case 0:
        max_pot = pot(value);
        ADMUX = bit(REFS0) | /*bit(REFS1) |*/ (BAT_PIN & 0x07);
        whichADCtoRead = 1;
        break;
      case 1:
        battery_level = 5.0 / 1024.0 * value;
        ADMUX = bit(REFS0) | /*bit(REFS1) |*/ (POT_PIN & 0x07);
        whichADCtoRead = 0;
        break;
    }
    bitSet(ADCSRA, ADSC);
#ifdef SERIAL_DEBUG
    Serial.print(F(" POT: "));
    Serial.print(max_pot);
    Serial.print(F(" BAT: "));
    Serial.print(battery_level);
#endif
  }

#ifdef REV_PIN
  // REV Trigger Section
  if (!RevTrigger) {
    rev_servo.write(max_pot);
  } else {
    rev_servo.write(MinServo);
  }
#ifdef SERIAL_DEBUG
  Serial.print(F(" REV: "));
  Serial.print(RevTrigger);
#endif
#endif

  // Trigger Section
  if (!Trigger) {
    if (time_millis > fire_event && fire_count > 0) {
      if (fire_count % 2 == 0) {
        digitalWrite(FIRE_PIN, HIGH);
        digitalWrite(TRG_LED, HIGH);
        fire_event = time_millis + FIRE_ON_DURATION;
      } else {
        digitalWrite(FIRE_PIN, LOW);
        digitalWrite(TRG_LED, LOW);
        fire_event = time_millis + FIRE_OFF_DURATION;
      }
      fire_count--;
    }
  } else {
    digitalWrite(FIRE_PIN, LOW);
    digitalWrite(TRG_LED, LOW);
    switch (fire_case) {
      case 3: // Single fire
        fire_count = 2;
        break;
      case 2: // Burst fire
        fire_count = 6;
        break;
      case 1: // Sustained fire
        fire_count = 32768;
        break;
      case 0: // undefined, config mode?
        break;
    }
  }

#ifdef MAG_SENSE
  if (mag_case != 0 && time_millis > mag_case) 
  {
    while (!tcs.available()) { // if current measurement has done
      delay(2);
    }
    TCS34725::Color color = tcs.color();
#ifdef SERIAL_DEBUG
    Serial.print(F("Color Temp : ")); Serial.println(tcs.colorTemperature());
    Serial.print(F("Lux        : ")); Serial.println(tcs.lux());
    Serial.print(F("R          : ")); Serial.println(color.r);
    Serial.print(F("G          : ")); Serial.println(color.g);
    Serial.print(F("B          : ")); Serial.println(color.b);
#endif
  }
#endif

#ifdef SERIAL_DEBUG
  Serial.print(F(" TRG: "));
  Serial.print(Trigger);
  Serial.print(F(" FRC: "));
  Serial.println(fire_case);
#endif
}
