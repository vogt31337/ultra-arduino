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

//Battery + "Bucked" down > 5ish volts in at +5
//Battery - > GND
//Trigger > D2
// Servo Signal (PULSE) > D9
// Servo Signal (PULSE) > D10
// trimpot > A7

#define POT_PIN 7
#define BAT_PIN 6

// -- Trigger Pin --
#define TRG_PIN D3
#define TRG_LED 13

// -- Fire Selector --
#define FIRE_SELECT_PIN1 D3
#define FIRE_SELECT_PIN2 D4

// -- BLDC Rev System --
#define REV_PIN D2
#define REV_LED 12
#define REV_SERVO_PIN 9

// Quite complex to calculate, depends on your resistor values
#define BAT_SCALE_FACTOR 0.4f

#define DEBOUNCE_DELAY 50l

// uncomment to inverse your potentiometer
//#define INVERT_POT_DIRECTION 1

//----SETTINGS YOU CAN CHANGE
float MaxServo = 180;       // Reduce this if you want to electronically Limit your RPM / FPS .. 1000 being 0% 2000 being 100%
float MinServo = 0;

// Don't change the below setting

#include <Servo.h>
Servo rev_servo;

unsigned long[4] lastDebounceTime = {0, 0, 0, 0};
int[4] lastButtonState = {LOW, LOW, LOW, LOW};

bool RevTrigger = false;
bool Trigger = false;
int fire_case = 0;

int whichADCtoRead = 0; // 0 = Pot, 1 = Battery
int max_pot = MaxServo;
float battery_level = 0.0f;

void setup()
{
  Serial.begin(115200);
  pinMode(TRG_PIN, INPUT_PULLUP);
  pinMode(POT_PIN, INPUT);
  pinMode(FIRE_SELECT_PIN1, INPUT_PULLUP);
  pinMode(FIRE_SELECT_PIN2, INPUT_PULLUP);

  ADCSRA =  bit(ADEN);
  ADCSRA |= bit(ADPS0) | bit(ADPS1) | bit(ADPS2);
  ADMUX =  bit(REFS0) | bit(REFS1) | (POT_PIN & 0x07);
  bitSet(ADCSRA, ADSC);

  pinMode(REV_PIN, INPUT_PULLUP);
  rev_servo.attach(REV_SERVO_PIN);
  rev_servo.write(MinServo);
}

/**
 * Function to read button states of
 * Rev and Trigger
 */
void Buttonsstate(unsigned long time_millis)
{
  int rev_state = digitalRead(REV_PIN);
  int trg_state = digitalRead(TRG_PIN);

  if (rev_state != lastButtonState[0]) {
    lastDebounceTime[0] = time_millis;
  } else if (trg_state != lastButtonState[1]) {
    lastDebounceTime[1] = time_millis;
  }
  
  if ((time_millis - lastDebounceTime[0]) > DEBOUNCE_DELAY) {
    RevTrigger = true;
    digitalWrite(REV_LED, HIGH);
  } else {
    RevTrigger = false;
    digitalWrite(REV_LED, LOW);
  }

  if ((time_millis - lastDebounceTime[1]) > DEBOUNCE_DELAY) {
    Trigger = true;
    digitalWrite(TRG_LED, HIGH);
  } else {
    Trigger = false;
    digitalWrite(TRG_LED, LOW);
  }

  if ((time_millis - lastDebounceTime[2]) > DEBOUNCE_DELAY) {
    fire_case |= (1 << 1);
  } else {
    fire_case &= ~(1 << 1);
  }

  if ((time_millis - lastDebounceTime[3]) > DEBOUNCE_DELAY) {
    fire_case |= (1 << 2);
  } else {
    fire_case &= ~(1 << 2);
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
        battery_level = value * BAT_SCALE_FACTOR;
        ADMUX = bit(REFS0) | bit(REFS1) | (POT_PIN & 0x07);
        whichADCtoRead = 0;
        break;
    }
    bitSet(ADCSRA, ADSC);
  }

  // REV Trigger Section
  if (RevTrigger) {
    rev_servo.write(max_pot)
  } else {
    rev_servo.write(MinServo);
  }

  // Trigger Section
  if (Trigger) {
    switch(fire_case) {
      case 0: // Single fire
      
        break;
      case 1: // Burst fire
      
        break;
      case 2: // Sustained fire
      
        break;
      case 3: // undefined, config mode?
        break;
    }
  }
}
