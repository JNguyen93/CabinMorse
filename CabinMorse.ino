
#include <SoftwareSerial.h>
#include <Bounce2.h>

#define DOT_DURATION   250               // Duration of a Morse Code "dot" (in milliseconds)
#define DASH_DURATION  DOT_DURATION * 3  // Duration of a Morse Code "dash" (in milliseconds)
#define SIGNAL_GAP     400      // Gap between dots/dashes of a single letter (in ms)
#define LETTER_GAP     DOT_DURATION * 3  // Gap between two letters (in milliseconds)
#define WORD_GAP       DOT_DURATION * 7  // Gap between two words (in milliseconds)

#define LED            13                // The digital connector port to LED anode
#define BUTTON          7                // The diginal connector port to the button
#define MAGLOCK         8

#define DOT             1                // DOT identifier
#define DASH            2                // DASH identifier
#define NONE            0                // Neither DOT nor DASH

//Pin connected to ST_CP of 74HC595
int latchPin = 8;
//Pin connected to SH_CP of 74HC595
int clockPin = 12;
////Pin connected to DS of 74HC595
int dataPin = 11;
const int backspace = 6;
SoftwareSerial LCD(9, 10); // Arduino SS_RX = pin 9 (unused), Arduino SS_TX = pin 10 

int dotFive = 2;
int dashFive = 3;
boolean buttonWasPressed = false;        // Indicator of whether button was pressed in the last cycle
long lastTimestamp = 0;                  // Last recorded timestamp  (used for mesuring duration)
long lastPress = 0;
long lastOutput = 0;
long threshold = 100;
byte inputSignal[5];                     // Input signal buffer
char combination[4];
char sos[3];
char sosAnswer[3];
char answer[4];
byte feedback;
int combinationIndex = 0;
int inputSignalIndex = 0;                // Index into the input signal buffer
int sosIndex = 0;
boolean help;
boolean correct;
boolean confirmed = false;
boolean coordinates = false;

Bounce morse0 = Bounce();
Bounce backButton = Bounce();

void resetInputSignal() {                // Reset the input signal buffer and index
  inputSignal[0] = NONE;
  inputSignal[1] = NONE; 
  inputSignal[2] = NONE;
  inputSignal[3] = NONE;
  inputSignal[4] = NONE;
  inputSignalIndex = 0;
}

void LCDClear(){
  LCD.write(0xFE);
  LCD.write(0x01);
}

void setup() {
  pinMode(LED, OUTPUT);                  // Set the LED output
  pinMode(BUTTON, INPUT_PULLUP);                // Set the button input
  pinMode(MAGLOCK, OUTPUT);
  pinMode(latchPin, OUTPUT);
  pinMode(dotFive, OUTPUT);
  pinMode(dashFive, OUTPUT);
  pinMode(backspace, INPUT_PULLUP);
  digitalWrite(LED, LOW);                // Turn off the LED
  digitalWrite(MAGLOCK, HIGH);
  digitalWrite(dotFive, LOW);
  digitalWrite(dashFive, LOW);
  Serial.begin(9600);                    // Init the serial port
  LCD.begin(9600);
  resetInputSignal();                    // Reset input signal buffer
  morse0.attach(BUTTON);
  morse0.interval(50);
  backButton.attach(backspace);
  backButton.interval(50);
  answer[0] = '2';
  answer[1] = '2';
  answer[2] = '1';
  answer[3] = '7';
  sosAnswer[0] = 'S';
  sosAnswer[1] = 'O';
  sosAnswer[2] = 'S';
  LCDClear();
  LCD.write(0xFE);
  LCD.write(0x0D);
}

// return true if s0-s4 match input signal
boolean matchInputSignal(byte s0, byte s1, byte s2, byte s3, byte s4) {
  return ((inputSignal[0] == s0) && 
          (inputSignal[1] == s1) && 
          (inputSignal[2] == s2) && 
          (inputSignal[3] == s3) &&  
          (inputSignal[4] == s4));
}

// convert input signal to letter or ? if not found
char currentInputSignalToLetter() {  
  if (matchInputSignal(DOT, DASH, NONE, NONE, NONE))  { return 'A'; }
  if (matchInputSignal(DASH, DOT, DOT, DOT, NONE))    { return 'B'; }
  if (matchInputSignal(DASH, DOT, DASH, DOT, NONE))   { return 'C'; }
  if (matchInputSignal(DASH, DOT, DOT, NONE, NONE))   { return 'D'; }
  if (matchInputSignal(DOT, NONE, NONE, NONE, NONE))  { return 'E'; }
  if (matchInputSignal(DOT, DOT, DASH, DOT, NONE))    { return 'F'; }
  if (matchInputSignal(DASH, DASH, DOT, NONE, NONE))  { return 'G'; }
  if (matchInputSignal(DOT, DOT, DOT, DOT, NONE))     { return 'H'; }
  if (matchInputSignal(DOT, DOT, NONE, NONE, NONE))   { return 'I'; }
  if (matchInputSignal(DOT, DASH, DASH, DASH, NONE))  { return 'I'; }
  if (matchInputSignal(DASH, DOT, DASH, NONE, NONE))  { return 'K'; }
  if (matchInputSignal(DOT, DASH, DOT, DOT, NONE))    { return 'L'; }
  if (matchInputSignal(DASH, DASH, NONE, NONE, NONE)) { return 'M'; }
  if (matchInputSignal(DASH, DOT, NONE, NONE, NONE))  { return 'N'; }
  if (matchInputSignal(DASH, DASH, DASH, NONE, NONE)) { return 'O'; }
  if (matchInputSignal(DOT, DASH, DASH, DOT, NONE))   { return 'P'; }
  if (matchInputSignal(DASH, DASH, DOT, DASH, NONE))  { return 'Q'; }
  if (matchInputSignal(DOT, DASH, DOT, NONE, NONE))   { return 'R'; }
  if (matchInputSignal(DOT, DOT, DOT, NONE, NONE))    { return 'S'; }
  if (matchInputSignal(DASH, NONE, NONE, NONE, NONE)) { return 'T'; }
  if (matchInputSignal(DOT, DOT, DASH, NONE, NONE))   { return 'U'; }
  if (matchInputSignal(DOT, DOT, DOT, DASH, NONE))    { return 'V'; }
  if (matchInputSignal(DOT, DASH, DASH, NONE, NONE))  { return 'W'; }
  if (matchInputSignal(DASH, DOT, DOT, DASH, NONE))   { return 'X'; }
  if (matchInputSignal(DASH, DOT, DASH, DASH, NONE))  { return 'Y'; }
  if (matchInputSignal(DASH, DASH, DOT, DOT, NONE))   { return 'Z'; }
  if (matchInputSignal(DOT, DASH, DASH, DASH, DASH))  { return '1'; }
  if (matchInputSignal(DOT, DOT, DASH, DASH, DASH))   { return '2'; }
  if (matchInputSignal(DOT, DOT, DOT, DASH, DASH))    { return '3'; }
  if (matchInputSignal(DOT, DOT, DOT, DOT, DASH))     { return '4'; }
  if (matchInputSignal(DOT, DOT, DOT, DOT, DOT))      { return '5'; }
  if (matchInputSignal(DASH, DOT, DOT, DOT, DOT))     { return '6'; }
  if (matchInputSignal(DASH, DASH, DOT, DOT, DOT))    { return '7'; }
  if (matchInputSignal(DASH, DASH, DASH, DOT, DOT))   { return '8'; }
  if (matchInputSignal(DASH, DASH, DASH, DASH, DOT))  { return '9'; }
  if (matchInputSignal(DASH, DASH, DASH, DASH, DASH)) { return '0'; }
  if (matchInputSignal(DOT, DASH, DOT, DASH, DASH))   { return '1'; }
  if (matchInputSignal(DOT, DASH, DASH, DOT, DASH))   { return '1'; }
  if (matchInputSignal(DOT, DASH, DASH, DASH, DOT))   { return '1'; }
  if (matchInputSignal(DOT, DASH, DOT, DOT, DASH))    { return '1'; }
  if (matchInputSignal(DOT, DASH, DASH, DOT, DOT))    { return '1'; }
  if (matchInputSignal(DOT, DASH, DOT, DOT, DOT))     { return '1'; }
  return '?';
}

// turn on the LED for the specified duration in milliseconds
void showLightForDuration(long duration) {
  digitalWrite(LED, HIGH);
  delay(duration);
  digitalWrite(LED, LOW);
}  

// show signal (DOT or DASH) via LED 
boolean showSignal(byte dotDashNone) {
  switch(dotDashNone) {
    case DOT:
      showLightForDuration(DOT_DURATION);
      return true;
    case DASH:
      showLightForDuration(DASH_DURATION);
      return true;
    default:
      return false;
  }
}

// show letter from signals (DOTs and DASHes) via LED
void showLetterForSignals(byte s0, byte s1, byte s2, byte s3, byte s4) {
  if (showSignal(s0)) {
    delay(SIGNAL_GAP);
    if (showSignal(s1)) {
      delay(SIGNAL_GAP);
      if (showSignal(s2)) {
        delay(SIGNAL_GAP);
        if (showSignal(s3)) {
          delay(SIGNAL_GAP);
          showSignal(s4);
        }
      }
    }
  }
  delay(LETTER_GAP);
}  

// show letter from byte via LED
void showLetter(byte letter) {
  if (97 <= letter && letter <= 122) { // if a-z
    letter -= 32; // map to A-Z
  }
  switch(letter) {
    case 'A': showLetterForSignals(DOT, DASH, NONE, NONE, NONE); break;
    case 'B': showLetterForSignals(DASH, DOT, DOT, DOT, NONE); break;
    case 'C': showLetterForSignals(DASH, DOT, DASH, DOT, NONE); break;
    case 'D': showLetterForSignals(DASH, DOT, DOT, NONE, NONE); break;
    case 'E': showLetterForSignals(DOT, NONE, NONE, NONE, NONE); break;
    case 'F': showLetterForSignals(DOT, DOT, DASH, DOT, NONE); break;
    case 'G': showLetterForSignals(DASH, DASH, DOT, NONE, NONE); break;
    case 'H': showLetterForSignals(DOT, DOT, DOT, DOT, NONE); break;
    case 'I': showLetterForSignals(DOT, DOT, NONE, NONE, NONE); break;
    case 'J': showLetterForSignals(DOT, DASH, DASH, DASH, NONE); break;
    case 'K': showLetterForSignals(DASH, DOT, DASH, NONE, NONE); break;
    case 'L': showLetterForSignals(DOT, DASH, DOT, DOT, NONE); break;
    case 'M': showLetterForSignals(DASH, DASH, NONE, NONE, NONE); break;
    case 'N': showLetterForSignals(DASH, DOT, NONE, NONE, NONE); break;
    case 'O': showLetterForSignals(DASH, DASH, DASH, NONE, NONE); break;
    case 'P': showLetterForSignals(DOT, DASH, DASH, DOT, NONE); break;
    case 'Q': showLetterForSignals(DASH, DASH, DOT, DASH, NONE); break;
    case 'R': showLetterForSignals(DOT, DASH, DOT, NONE, NONE); break;
    case 'S': showLetterForSignals(DOT, DOT, DOT, NONE, NONE); break;
    case 'T': showLetterForSignals(DASH, NONE, NONE, NONE, NONE); break;
    case 'U': showLetterForSignals(DOT, DOT, DASH, NONE, NONE); break;
    case 'V': showLetterForSignals(DOT, DOT, DOT, DASH, NONE); break;
    case 'W': showLetterForSignals(DOT, DASH, DASH, NONE, NONE); break;
    case 'X': showLetterForSignals(DASH, DOT, DOT, DASH, NONE); break;
    case 'Y': showLetterForSignals(DASH, DOT, DASH, DASH, NONE); break;
    case 'Z': showLetterForSignals(DASH, DASH, DOT, DOT, NONE); break;
    case '1': showLetterForSignals(DOT, DASH, DASH, DASH, DASH); break;
    case '2': showLetterForSignals(DOT, DOT, DASH, DASH, DASH); break;
    case '3': showLetterForSignals(DOT, DOT, DOT, DASH, DASH); break;
    case '4': showLetterForSignals(DOT, DOT, DOT, DOT, DASH); break;
    case '5': showLetterForSignals(DOT, DOT, DOT, DOT, DOT); break;
    case '6': showLetterForSignals(DASH, DOT, DOT, DOT, DOT); break;
    case '7': showLetterForSignals(DASH, DASH, DOT, DOT, DOT); break;
    case '8': showLetterForSignals(DASH, DASH, DASH, DOT, DOT); break;
    case '9': showLetterForSignals(DASH, DASH, DASH, DASH, DOT); break;
    case '0': showLetterForSignals(DASH, DASH, DASH, DASH, DASH); break;
    case ' ': delay(WORD_GAP); break;
    default: 
      Serial.print("Don't understand [");
      Serial.print((char) letter);
      Serial.print("]");
      showLightForDuration(50);
      delay(50);
      showLightForDuration(50);
      delay(50);
      showLightForDuration(50);
  }
}

void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
  // This shifts 8 bits out MSB first, 
  //on the rising edge of the clock,
  //clock idles low

  //internal function setup
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  //for each bit in the byte myDataOut�
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights. 
  for (i=7; i>=0; i--)  {
    digitalWrite(myClockPin, 0);

    //if the value passed to myDataOut and a bitmask result 
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000 
    // and proceeds to set pinState to 1.
    if ( myDataOut & (1 << i) ) {
      pinState= 1;
    }
    else {  
      pinState= 0;
    }

    //Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
    //register shifts bits on upstroke of clock pin  
    digitalWrite(myClockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
  }

  //stop shifting
  digitalWrite(myClockPin, 0);
}

void LCDbackspace(int* index){
  Serial.print("Index: ");
  Serial.println(*index);
  if(*index > 0){
    LCD.print("\b");
    /*LCD.write(0xFE);
    LCD.write(0x10);
    LCD.print(" ");
    LCD.write(0xFE);
    LCD.write(0x10);*/
    *index = *index - 1;
    Serial.print("Index: ");
    Serial.println(*index);
  }
}

void loop() {
  long currentTimestamp  = millis(); // get the current timestamp
  long duration = currentTimestamp - lastTimestamp; // get elapsed time
  backButton.update();
  if(backButton.fell()){
    if(!help){
      LCDbackspace(&sosIndex);
    }
    if(help && !correct){
      LCDbackspace(&combinationIndex);
    }
  }
  morse0.update();
  if (morse0.fell()) { // if the button is pressed
    if (!buttonWasPressed) { //  if the button was previously not pressed
      //Serial.println("Pressed!");
      while(digitalRead(BUTTON) == LOW){}
      buttonWasPressed = true; // remember the button press
      digitalWrite(LED, HIGH); // turn on the LED
      lastTimestamp = currentTimestamp; // record the time of the button press
      if (duration > LETTER_GAP) {
        //Serial.print(' ');
      }
    } // end of if (button was not pressed)
  } else{ // the button is not pressed
    if (buttonWasPressed) {  // the button was just released
      if (duration < DOT_DURATION) { // if the button was pressed for up to DOT cutoff
        inputSignal[inputSignalIndex] = DOT; // remember the current signal as a DOT
      } else { // if the button was pressed for longer than DOT cutoff
        inputSignal[inputSignalIndex] = DASH; // remember the current signal as a DASH
      }
      inputSignalIndex++; // advance the index to the input signal buffer
      digitalWrite(LED, LOW); // turn off the LED
      buttonWasPressed = false; // consume previous button press
      lastTimestamp = currentTimestamp; // record the time the button was released
    } else { // the button was not just released
      if (inputSignalIndex > 0) { // if there is data in the input signal buffer
        if(millis() - lastOutput > SIGNAL_GAP){
            //Serial.print(currentInputSignalToLetter());
            //Serial.write(0xFE);
            //Serial.write(0x10);
            lastOutput = millis();
          }
        if (duration > SIGNAL_GAP || inputSignalIndex >= 5) { // if we have a complete letter
          //Serial.print(currentInputSignalToLetter()); // parse the letter and send it via serial
          LCD.print(currentInputSignalToLetter());
          for(int i = 3; i >= 0; i--){
            if(inputSignal[i] == DASH){
              feedback = feedback | (1 << (i * 2) + 1);
            }
            else if(inputSignal[i] == DOT){
              feedback = feedback | (1 << i * 2);
            }
          }
          if(feedback > 0){
            //Serial.println(feedback, BIN);
            digitalWrite(latchPin, 0);
            shiftOut(dataPin, clockPin, feedback);
            digitalWrite(latchPin, 1);
            if(inputSignal[4] == DOT){
              digitalWrite(dotFive, HIGH);
              digitalWrite(dashFive, LOW);
            }
            else if(inputSignal[4] == DASH){
              digitalWrite(dotFive, LOW);
              digitalWrite(dashFive, HIGH);
            }
            else{
              digitalWrite(dotFive, LOW);
              digitalWrite(dashFive, LOW);
            }
            feedback = 0;
          }
          //Serial.println(help);
          if(!help){
            sos[sosIndex] = currentInputSignalToLetter();
            if(sosIndex < 2 && sosIndex >= 0){
              sosIndex++;
            }
            else if(sosIndex >= 2){
              help = true;
              Serial.println();
              for(int i = 0; i < 3; i++){
                Serial.print(sos[i]);
                if(sos[i] != sosAnswer[i]){
                  help = false;
                  //break;
                }
              }
              Serial.println();
              sosIndex = 0;
              delay(500);
              LCDClear();
            }
            if(help){
                LCD.print("...");
                delay(2000);
                LCDClear();
                LCD.print("We've received your SOS. Please enter your coordinates.");
                delay(5000);
                LCDClear();
                LCD.print("Coordinates: ");
             }
          }

          else{
            Serial.println(combinationIndex);
            combination[combinationIndex] = currentInputSignalToLetter();
            /*if(combinationIndex > 0 && !confirmed){
                LCD.write(0xFE);
                LCD.write(0x01);
                LCD.print("Coordinates: ");
                confirmed = true;
                combinationIndex = 0; 
            }*/
            if(combinationIndex < 3 && combinationIndex >= 0){
              combinationIndex++;
            }
            else if(combinationIndex >= 3){
              correct = true;
              Serial.println();
              for(int i = 0; i < 4; i++){
                //Serial.print(combination[i]);
                //Serial.print(" equals ");
                //Serial.println(answer[i]);
                if(combination[i] != answer[i]){
                  correct = false;
                  //Serial.println(correct);
                  //break;
                }
              }
              Serial.println();
              delay(500);
              LCDClear();
              LCD.print("Coordinates: ");
              combinationIndex = 0;
            }
            if(correct){
              delay(2000);
              LCDClear();
              LCD.print("Rescue teams are on their way.");
              delay(3000);
              LCDClear();
              LCD.print("...");
              delay(3000);
              LCDClear();
              LCD.print("It looks like the exit has been snowed in. 'Breakout' using another exit.");
              digitalWrite(MAGLOCK, LOW);
              while(1){}
            }
          }
          
          resetInputSignal(); // reset the input signal buffer
        }
      }
    } // end of else (button was not previously pressed)
  } // end of else (button is not pressed)
  if (Serial.available() > 0) { // if there is data availalbe on the serial port
    showLetter(Serial.read()); // read the next byte and output it as morse code via LED
  }
} // end of loop
