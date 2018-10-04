#include <U8g2lib.h>
#include <U8x8lib.h>
#include <RotaryEncoder.h>

/****************************************************************************/
/*                           SPOT WELDER FIRMWARE                           */
/****************************************************************************/
/*________________________________04 Oct 2018_______________________________*/
/* Designed to allow for control of a Microwave Oven Transformer based      */
/* spot-welder. Values in CAPS are user-changeable.                         */
/* The welder is designed to operate on two pulses: one to soften the metal,*/
/* the other to weld it together. The weld pulse is user-settable.          */
/*    Uses:                                                                 */
/*         Arduino Uno                                                      */
/*         128x32 I2C OLED                                                  */
/*         5V-240V 10A relay                                                */
/*         Rotary Encoder                                                   */
/*         Momentary push button                                            */
/*         LED                                                              */
/*                                                                          */
/* OLED display gives important data about settings.                        */
/* Relay switches the power on the MAINS (240V) side.                       */
/* Rotary encoder navigates the menu.                                       */
/* Momentary push button fires the welder                                   */
/* LED indicates welding. For troubleshooting and visual cue                */
/****************************************************************************/

/*                             PIN DECLARATIONS                             */
int TRIGBTN = 4;     // Triggers the pulse through the electrodes
int RELAY = 5;       // Does the actual switching of the power
int LED = 13;        // Flashes in time with welding



/*                               SCREEN SETUP                               */
// From HelloWorld example
int ENC_CLOCK = A5;
int ENC_DATA = A4;

U8G2_SSD1306_128X32_UNIVISION_1_SW_I2C u8g2
(U8G2_R0, /* clock=*/ ENC_CLOCK, /* data=*/ ENC_DATA, /* reset=*/ U8X8_PIN_NONE);

/*                                ENCODER SETUP                             */
//Wired according to https://circuit-diagramz.com/arduino-rotary-encoder-wiring/
//EncBtn = 0; //UNDEFINED, For future use
int InitPulse = 50;        // Pulse to soften metal before weld pulse
int PulseLength = 100;     // Default pulse length

int EncA = 6;              // One side of encoder
int EncB = 7;              // Other side of encoder
// To read changes in encoder position
int AState;
int AStatePrev;
int BState;
int BStatePrev;

int ADJUST_STEP = 5;       // How many millisecs each encoder step equals 

// Swap these to switch encoder direction
int CW = 1;
int CCW = -1;

/*                               TRIGGER SETUP                              */
int OldBtnState = 0;
int BtnState = 0;

/*                              FUNCTION SETUP                              */
int ReadEncoder(){
// Reads encoder position and decides direction it has been turned in
    AState = digitalRead(EncA); // Reads the current state of outputA
    BState = digitalRead(EncB); // Reads the current state of outputB
    int Dir = 0;
    // 4 possible positions: 11, 10, 00, 01
    // Find which it WAS on
    if(AStatePrev != AState || BStatePrev != BState){ // If something changed
        if(AStatePrev == HIGH && BStatePrev == HIGH){    // Was 11
           if(AState == HIGH){                           // Is 10
               Dir = CW;
           }
           else{Dir = CCW;}                              // Is 01
       }
        else if(AStatePrev = HIGH && BStatePrev == LOW){ // Was 10
           if(AState == LOW){                            // Is 00
               Dir = CW;
           }
           else{Dir = CCW;}                              // Is 11
        } 
        else if(AStatePrev == LOW && BStatePrev == LOW){ // Was 00
            if(BState == HIGH){                          // Is 01
                Dir = CW;
            }
            else{Dir = CCW;}                             // Is 10
        }
        else{/* AStatePrev == LOW && BStatePrev == HIGH*/// Was 01
             if(AState == HIGH){                         // Is 11
                 Dir = CW;
             }
             else{Dir = CCW;}                            // Is 00
        } 
    }
    else{Dir = 0;}           // Indicate lack of change
    // Update previous states 
    AStatePrev = AState; 
    BStatePrev = BState;
    return Dir;
}

void ScreenWrite(int pulse){
// Basic printout to screen to indicate pulse set. Upgrade to menu?
    u8g2.setFont(u8g2_font_7x14_mr);
    u8g2.firstPage();
    do{
        u8g2.setCursor(0,14);
        u8g2.print("Welder Operational");
        u8g2.setCursor(0,31);
        u8g2.print("Pulse: ");
        u8g2.print(pulse);
        u8g2.print("ms");  
    }
    while (u8g2.nextPage());
}

bool ReadBtn(){
// Read firing trigger, return HIGH if pressed, contains debounce
    int State = LOW;
    BtnState = digitalRead(TRIGBTN);
    if (BtnState != OldBtnState && BtnState == LOW){
        State = HIGH;
    }
    else{
        State = LOW;
    }
    OldBtnState = BtnState;
    return State;
}

int GetPulse() {
// Poll the encoder to find pulse length and write to screen
    PulseLength = PulseLength + (ReadEncoder()*ADJUST_STEP);
    return PulseLength;
    }
    


/*                                 MAIN CODE                                */
void setup() {
    Serial.begin(9600); //DEBUG

    pinMode(TRIGBTN, INPUT_PULLUP);
    pinMode(RELAY, OUTPUT);
    pinMode(LED, OUTPUT);

    // ENCODER
    pinMode (EncA,INPUT_PULLUP);
    pinMode (EncB,INPUT_PULLUP);
    // Obtain starting states of encoder switches
    AStatePrev = digitalRead(EncA); 
    BStatePrev = digitalRead(EncB);
    

    // SCREEN
    pinMode (ENC_CLOCK, OUTPUT);
    pinMode (ENC_DATA, OUTPUT);
    digitalWrite (ENC_CLOCK, 0);  // Ensure no erronious data
    digitalWrite (ENC_DATA, 0);   // Ensure no erronious data
    u8g2.begin();
  
    // Quick start-up screen
    u8g2.firstPage();
    do
      {
       u8g2.setFont(u8g2_font_helvB14_tr);
       u8g2.setCursor(0,15);
       u8g2.print("Spot Welder");
       u8g2.setFont(u8g2_font_chroma48medium8_8r);
       u8g2.setCursor(0,31);
       u8g2.print("S.Maybury, 10/18");
    }
    while (u8g2.nextPage());
    delay(2000);
}

void loop() {
    // Check set pulse length and print to screen
    GetPulse();
    ScreenWrite(PulseLength);
    
    if(ReadBtn() == HIGH){
    // If trigger pressed, switch relay in pulses
        digitalWrite(RELAY, HIGH);
        Serial.print("HIGH\n");     // DEBUG
        digitalWrite(LED, HIGH);
        delay(InitPulse);
        digitalWrite(RELAY, LOW);
        digitalWrite(LED, LOW);
        Serial.print("LOW\n");     // DEBUG
        delay(InitPulse);          // EXPERIMENTAL LENGTH
        digitalWrite(RELAY, HIGH);
        digitalWrite(LED, HIGH);
        Serial.print("HIGH\n");    // DEBUG
        delay(PulseLength);
        digitalWrite(RELAY, LOW);
        digitalWrite(LED, LOW);
        Serial.print("LOW\n");     // DEBUG
        delay(1000);               // 1 second safety delay
    }


    
    
    
}
