/*
 * Ultra Arduino is a program to control functions of a modified Nerf Stryfe blaster.
 * Mods and original source are from:
 * https://www.thingiverse.com/thing:2460171/files
 * https://www.thingiverse.com/thing:3029346
 *
 * Created By Michael Dixon (Ultrasonic2) Auckland New Zealand
 * Modified by vogt31337
 * 
 * Needed external libs:
 * 
 */
 
// ---- SETTINGS YOU CAN CHANGE ----
#define POT_PIN 7
#define BAT_PIN 6
#define FIRE_PIN 5
#define OPTO_PIN 8

// -- Trigger Pin --
#define TRG_PIN 4
#define TRG_LED 13

// -- Fire Selector --
#define FIRE_SELECT_PIN1 2
#define FIRE_SELECT_PIN2 1

// -- BLDC Rev System --
#define REV_PIN 82
#define REV_LED 12
#define REV_SERVO_PIN 9

// -- Delay Section --
#define DEBOUNCE_DELAY 10
#define FIRE_ON_DURATION 300
#define FIRE_OFF_DURATION 500

//#define INVERT_POT_DIRECTION 1  // uncomment to inverse your potentiometer
float MaxServo = 180;             // Reduce this if you want to electronically Limit your RPM / FPS .. 0 being 0% 180 being 100%
float MinServo = 0;

// ---- Don't change the below setting ----

#ifdef REV_PIN
#include <Servo.h>
Servo rev_servo;
bool RevTrigger = false;
#endif

unsigned long lastDebounceTime[4] = {0, 0, 0, 0};
int lastButtonState[4] = {LOW, LOW, LOW, LOW};

bool Trigger = false;
int fire_case = 0;

int whichADCtoRead = 0; // 0 = Pot, 1 = Battery
int max_pot = MaxServo;
float battery_level = 0.0f;
int fire_count = 0;
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
  pinMode(FIRE_SELECT_PIN1, INPUT_PULLUP);
  pinMode(FIRE_SELECT_PIN2, INPUT_PULLUP);

  #ifdef OPTO_PIN
  pinMode(OPTO_PIN, INPUT);
  #endif

  ADCSRA =  bit(ADEN);
  ADCSRA |= bit(ADPS0) | bit(ADPS1) | bit(ADPS2);
  ADMUX =  bit(REFS0) | bit(REFS1) | (POT_PIN & 0x07);
  bitSet(ADCSRA, ADSC);

  #ifdef REV_PIN
  pinMode(REV_PIN, INPUT_PULLUP);
  pinMode(REV_LED, OUTPUT);
  rev_servo.attach(REV_SERVO_PIN);
  rev_servo.write(MinServo);
  #endif
}

/**
 * Function to read button states of
 * Rev and Trigger
 */
void Buttonsstate(unsigned long time_millis)
{
  #ifdef REV_PIN
  int rev_state = digitalRead(REV_PIN);
  
  if (rev_state != lastButtonState[0]) {
    lastDebounceTime[0] = time_millis;
  }
  
  if ((time_millis - lastDebounceTime[0]) > DEBOUNCE_DELAY) {
    RevTrigger = (bool) rev_state;
    digitalWrite(REV_LED, rev_state);
  }
  #endif
  
  int trg_state = digitalRead(TRG_PIN);

  if (trg_state != lastButtonState[1]) {
    lastDebounceTime[1] = time_millis;
  }

  int fr1_state = digitalRead(FIRE_SELECT_PIN1);

  if (fr1_state != lastButtonState[2]) {
    lastDebounceTime[2] = time_millis;
  }

  int fr2_state = digitalRead(FIRE_SELECT_PIN2);

  if (fr2_state != lastButtonState[3]) {
    lastDebounceTime[3] = time_millis;
  }

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
    digitalWrite(TRG_LED, trg_state);
  }

  if ((time_millis - lastDebounceTime[2]) > DEBOUNCE_DELAY) {
    if (fr1_state) {
      fire_case |= (1 << 1);
    } else {
      fire_case &= ~(1 << 1);
    }
  }

  if ((time_millis - lastDebounceTime[3]) > DEBOUNCE_DELAY) {
    if (fr2_state) {
      fire_case |= (1 << 2);
    } else {
      fire_case &= ~(1 << 2);
    }
  }
} // buttonState

/*
 * Get potentiometer value, as arduino is 10bit 
 * shift right by 2 bits (divide by 4) which reduces noise greatly
 */
int pot(int current_pot) {
  #ifdef INVERT_POT_DIRECTION
    return map(current_pot, 1023, 0, MinServo, MaxServo);
  #else
    return map(current_pot, 0, 1023, MinServo, MaxServo);
  #endif
}

void loop() {
  unsigned long time_millis = millis();
  Buttonsstate(time_millis);

  // ADC Section
  if (bit_is_clear(ADCSRA, ADSC)) {
    int value = ADC;
    switch (whichADCtoRead) {
      case 0:
        max_pot = pot(value);
        ADMUX = bit(REFS0) | bit(REFS1) | (BAT_PIN & 0x07);
        whichADCtoRead = 1;
        break;
      case 1:
        battery_level = 1.1 / value * 1024.0;
        ADMUX = bit(REFS0) | bit(REFS1) | (POT_PIN & 0x07);
        whichADCtoRead = 0;
        break;
    }
    bitSet(ADCSRA, ADSC);
  }

  #ifdef REV_PIN
  // REV Trigger Section
  if (RevTrigger) {
    rev_servo.write(max_pot);
  } else {
    rev_servo.write(MinServo);
  }
  #endif

  // Trigger Section
  if (Trigger) {
    if (time_millis > fire_event && fire_count > 0) {
      if (fire_count % 2 == 0) {
        digitalWrite(FIRE_PIN, HIGH);
        fire_event = time_millis + FIRE_ON_DURATION;
      } else {
        digitalWrite(FIRE_PIN, LOW);
        fire_event = time_millis + FIRE_OFF_DURATION;
      }
      fire_count--;
    }
  } else {
    digitalWrite(FIRE_PIN, LOW);
    switch(fire_case) {
      case 0: // Single fire
        fire_count = 2;
        break;
      case 1: // Burst fire
        fire_count = 6;
        break;
      case 2: // Sustained fire
        fire_count = 32768;
        break;
      case 3: // undefined, config mode?
        break;
    }
  }
}
