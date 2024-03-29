 /*
 * VK2IDL's CW Morse Decoder program for Arduino [March 2020]
 * (c) 2020, Ian Lindquist - VK2IDL
 * 
 * This software is free of charge and may be used, modified or freely distributed.
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED.
 * 
 * This program reads keyboard text characters from the Arduino IDE monitor and generates 
 * Morse Code for sending to a transceiver. All transmitted text is displayed in the Arduino 
 * IDE's monitor. The program also has 6 preset buffers pre-loaded with commonly used statements
 * including your CQ call, Name, QTH, and Antenna details.  Additional code has been included 
 * to allow your text to be displayed on a separate 20 x 4 LCD connected to the Arduino.
 * 
 * ==== Please refer to "Operation.txt" for general instructions. ====
 * 
 * Serveral software methods used in this program have been borrowed from Budd Churchward's 
 * "WB7FHC's Simple Morse Code Decoder v. 2.4" program. The VK2IDL Morse Encoder program was 
 * designed to compliment the "WB7FHC Simple Morse Code Decoder" to form a complete morse 
 * code Send/Receive station.
 * 
 * This software uses a method borrowed from "WB7FHC's Simple Morse Code Decoder which 
 * derives morse code from an array. The morse code for each character is stored in a 
 * 63 byte array called mySet[]. The binary value of the array position for each character 
 * represents the morse code for that character in 1's and 0's with 1 being a DIT and 0 
 * being a Dah. Preceeding each morse sequence is an additional 1 which is used as a flag 
 * to identify the start of the morse code.    
 * 
 * I have also borrowed from WB7FHC's 'getPunctuation()' function as it also works 
 * equally well for this application.
 * 
 * ADDED MORSE DECODING TO ALLOW DISPLAY OF PADDLE-KEYED MORSE CODE ON THE LCD AND MONITOR
 * Version 1.4 adds two line scrolling to the LCD display.
 * Version 2.2 fixes a bug that caused LF and CR characters from the keyboard to be included 
 * in the morse buffer causing an extra morse 1 to be sent after text.
 * 
 * The latest modification was to add a potentimeter to control the morse speed. Ive now 
 * removed the code that previously allowed the morse speed to be controlled from
 * the keyboard. Adjusting the morse speed potentiometer provides a live update of
 * the morse speed on the monitor and the LCD.
 * Change the values of 'wpm_Min' and 'wpm_Max' to alter the end points for the WPM range.
*/

String verNum = "2.2";        // ========== Version number ==============

#define LCD_Yes 1             // Remove '//' to enable a connected 20x4 LCD

#ifdef LCD_Yes   // ================= if using if using a 20x4 LCD ==================
  // Include libraries
  #include <LiquidCrystal_I2C.h>  // Include LCD library
  #include <Wire.h>               // Include the Wire library for Serial access and I2C control

  // Define LCD2004A Configuration pinout and address
  const int  en = 2, rw = 1, rs = 0, d4 = 4, d5 = 5, d6 = 6, d7 = 7, bl = 3;  // Define LCD's variables
  const int i2c_addr = 0x27;                                                  // Define LCD's I2C Address

  // Initialize the LCD variables with the numbers of the interface pins
  LiquidCrystal_I2C lcd(i2c_addr, en, rw, rs, d4, d5, d6, d7, bl, POSITIVE);  // Define LCD object from library and 
#endif

// Morse Speed variables
int morseSpeed;               // Holds the current Morse speed in WPM
int morseSpeed_Old;           // Holds the previous morse speed in WPM
int speedPin = A0;            // Center pin of the speed potentiometer connects here
int wpm_Min = 7;              // Determines the highest morse speed value
int wpm_Max = 36;             // Determines the lowest morse speed value

// Text and buffer variables
const unsigned int bufferMax = 55;   // Maximum size of the input buffer 
const unsigned int bufferSize = 53;  // Useable size of the input buffer
            
char bufferIn[bufferMax];           // Input buffer to hold characters typed by the user
int bufferIn_Count = 0;             // Keeps track of the # of characters in the buffer
char inputChar;                     // Holds keyboard input character
char bufferChar;                    // Holds the character taken from the keyboard input buffer
char morseChar;                     // Holds character extracted from the mySet[] Array
int morseNum;                       // Holds the location of the morse character in mySet[] array
int txLCD_Count = 0;                // Start location for the TX output text print on the LCD
int tx_LCD_Start = 0;               // Starting column on LCD for TX text 
int LCD_Line = 3;                   // Line # on LCD for printing TX text
char txBuffer[20];                  // Holds LCD TX display buffer for scrolling

// Manual morse timing variables
long countTime = 0;                 // Counts when Key is pressed
long startTime = 0;                 // Counts when Key is pressed
long endTime = 0;                   // Counts when Key is released

long InTime = 0;                    // Time when entering character decoder
long OutTime = 0;                   // Time when exiting character decoder
long toneON;
long toneOFF;
long toneStartTime;                 // Time Tone was switched on
long toneEndTime;                   // Time tone was switched off
long old_toneEndTime;               // Stores old toneEndTime
long DitL;                          // Holds the length of a DIT - character decode
long DahL;                          // Holds length of a Dah - character decoder
long elementL = 120;                // Holds a default element length used for first decode value

boolean dahOnFlag = false;          // True when a Dah is in progress
boolean dahOffFlag = false;         // True when Dah is off
boolean dahToneFlag = false;        // Dah tone is On/Off
boolean ditOnFlag = false;          // True when a Dit is in progress
boolean ditOffFlag = false;         // True when Dit is off
boolean ditToneFlag = false;        // Dah tone is On/Off
boolean space = false;              // Flag defining if its Ok to print a space
boolean character = false;          // Flag defining the end of a character

// Morse Output pins
int morsePin_LED = 6;               // Defines the Output pin to drive the Morse Code flasher LED
int morsePin_CODE = 7;              // Defines the Output pin to drive the Morse code keyer signal
int morsePin_RXin = 8;              // Defines the output pin to drive the morse decoder input
                                    // NOTE: This pin is an inverted signal to the LED and CODE outputs
// Morse-Key Input Pins
int keyPin = 9;                     // Input for traditional morse key
int paddlePin_LEFT = 10;            // Input for morse paddle (Left side)
int paddlePin_RIGHT = 11;           // Input for morse paddle (Right side)

int morseKeyState = 1;              // Stores the state of the Morse Key
int LEFTpaddleState = 1;            // Stores the state of the Left Paddle Key
int RIGHTpaddleState = 1;           // Stores the state of the Right Paddle Key

// Morse Code timing parameters
float ditLength;        // Length of a dit based on the defined Morse Speed
float dahLength;        // Length of a Dah referenced to the Dit length
float characterSpace;   // Space between characters referenced to the Dit length
float elementSpace;     // Space between elements referenced to the Dit length
float wordSpace;        // Space between words referenced to the Dit length

boolean manual_ON = false;

char currentLine[] = "                    ";  // Hold the current line of text
float lastSpace;                              // Holds LCD cursor position of the last space

// ============== Preset Morse Messages Here ===============
char CQ_Mess[] = " CQ CQ CQ DE VK2IDL VK2IDL K ";
char NAME_Mess[] = " NAME IS IAN IAN. ";
char QTH_Mess[] = " QTH IS PORT MACQUARIE PORT MACQUARIE. ";
char SIGNAL_Mess[]=" YR RST ";
char ANT_Mess[] = " ANT IS MULTIBAND TRAP DIPOLE. ";
char CQTest_Mess[] = " CQTEST CQTEST CQTEST DE VK2IDL. ";

// Define morse characters in an array
char mySet[] ="##TEMNAIOGKDWRUS##QZYCXBJP#L#FVH09#8###7#####/=61#######2###3#45"; 

void setup() // === INITIALISATION CODE ====
{
  Serial.begin(9600);               // Setup serial communication for Monitor

  pinMode(morsePin_LED, OUTPUT);    // Define the Morse LED pin
  pinMode(morsePin_CODE, OUTPUT);   // Define the Morse Code-Sending Pin
  pinMode(morsePin_RXin, OUTPUT);   // Define the Morse-to-RXinput pin
  digitalWrite(morsePin_RXin,HIGH); // Set Morse-to-RXinput pin HIGH to match 
                                    
  // Define Morse Key Input Pins
  pinMode(keyPin, INPUT);          // Define the Manual Morse Key Input pin
  digitalWrite(keyPin,1);          // Enable the internal pull up resistor

  pinMode(paddlePin_LEFT, INPUT);  // Define the Morse Paddle-Left pin
  digitalWrite(paddlePin_LEFT,1);  // Enable the internal pull up resistor
  pinMode(paddlePin_RIGHT, INPUT); // Define the Morse Paddle-Right pin
  digitalWrite(paddlePin_RIGHT,1); // Enable the internal pull up resistor

  // Print Header on the monitor
  Serial.print("=== VK2IDL Morse Code Generator Version "); // Print initialisation header
  Serial.print(verNum);            // Print Version #
  Serial.println(" ===");
  Serial.println();                // Print a linefeed

  // Calculate the current setting of the morse speed potentiometer
  morseSpeed = 100000 / (map(analogRead(speedPin),0 ,1023, wpm_Max, wpm_Min) * 440);
  morseSpeed_Old  = morseSpeed;         // Shows a change in the morse speed value
  
  // Initialised the Morse Code timing parameters using the 'morseSpeed' value calculated above
  morseTiming();      // Includes displaying the morse speed on the monitor

#ifdef LCD_Yes        // ================= if using an LCD ==================
  lcd.begin(20, 4);                   // Define the LCD as a 20x4 display
  lcd.clear();                        // Clear the display on startup

  // Print the header on the top line of LCD
  lcd.setCursor(0,0);                 // Position the cursor
  lcd.print("VK2IDL Morse Encoder");  // Print initialisation header
  lcd.setCursor(13,1);
  lcd.print("Ver ");
  lcd.print(verNum);

  // Print default morse speed on the second line of the LCD
  lcd.setCursor(0,1);                 // Position the cursor
  lcd.print(morseSpeed);              // Print morse speed
  lcd.print(" WPM"); 
#endif

  // Print On-Screen Instructions
  Serial.println("To select 'Recall' buffers, use the following keys.");
  Serial.println("[ = Buffer #1       ] = Buffer #2       \\ = Buffer #3");
  Serial.println("{ = Buffer #4       } = Buffer #5       | = Buffer #6");
  Serial.println();
    
  // Initialise the Input buffer with zeros
  memset(bufferIn, 0, bufferMax);   
}



void loop() // ==== MAIN PROGRAM LOOP ====
{
/*
 * The program first checks to see if the morse Paddle or Manual morse Key are active. If the 
 * Paddle is active, it processes DITs or DAHs depending on whether the left or right paddle 
 * is pressed. If the manual morse key is active, it activates the morse tone for as long as 
 * the Manual key is held (morseKeyDN) and releases the tone it once the Manual key is released 
 * (morseKeyUP). 
 * If neither the paddle nor the morse key are active, the program processes morse from the 
 * keyboard and preset buffers.
 */
   // Check to see if the Morse speed has changed
   morseSpeed = 100000 / (map(analogRead(speedPin),0 ,1023, wpm_Max, wpm_Min) * 440);
   if (morseSpeed != morseSpeed_Old)         // Only if the morse speed has changed
   {
       morseSpeed_Old = morseSpeed;          // Update the variable
       morseTiming();                        // Reset timing variables using the new value
                                             // and update the display of the new morse speed
   }
   
   // Check the state of the Morse Paddle and Manual Key pins
   LEFTpaddleState  = digitalRead(paddlePin_LEFT);   // What is the Left Morse paddle doing?
   RIGHTpaddleState = digitalRead(paddlePin_RIGHT);  // What is the Right Morse paddle doing?
   morseKeyState    = digitalRead(keyPin);           // What is the state of the standard Morse key
   
   // If the Morse Paddle's LEFT or RIGHT keys are active
   if ((!LEFTpaddleState) |  (!RIGHTpaddleState))   // If either paddle pin is LOW (0), the paddle is active
   {
      // If its the left paddle and no previous DIT or DAH is in progress
      if ((!LEFTpaddleState) & (!dahOffFlag) & (!ditOffFlag)) 
      {
        countTime = millis();                // Get the Arduino clock time
        startTime = countTime;               // Save as the start time
        morse_On();                          // Start the DAH Tone
        dahOnFlag = true;                    // True when Dah has started
        dahToneFlag = true;                  // True when Tone is on
        dahOffFlag = true;                   // True while Dah cycle is in progress
      }
      // If its the Right paddle and no previous DIT or DAH is in progress
      else if ((!RIGHTpaddleState) & (!ditOffFlag) & (!dahOffFlag))  
      {
        countTime = millis();                // Get the Arduino clock time
        startTime = countTime;               // Save as the start time
        morse_On();                          // Start the DIT Tone
        ditOnFlag = true;                    // True when Dit has started
        ditToneFlag = true;                  // True when Tone is on
        ditOffFlag = true;                   // True when Dit cycle is in progress
      }
   }

   // Otherwise if the Manual morse key is active
   else if (!morseKeyState & !manual_ON)     // If Manual Key pin is DOWN and the manual_ON flag is false
   {
     morse_On();                   // Activate the Morse Tone
     manual_ON = true;             // Set the manual tone flag to true
                                   // This ensures the Morse tone is only switched on ONCE
   }  
   else if (morseKeyState & manual_ON)  // Otherwise if the key is UP
   { 
    morse_Off();                    // Release the tone
    manual_ON = false;              // Reset the flag
   }


   // If none of the Morse keys have been pressed, process the keyboard characters
   else
   {
     getTX_Characters();       // Get characters from user keyboard and put them into a buffer
     processTX_Buffer();       // Process the characters in the buffer into Morse Code
   } 
  
  paddleTimer();               // Process the Paddle-key morse inputs
  printDitDah();               // Display the Paddle or manual key morse on the LCD
}



// ============================== FUNCTIONS GO HERE ===============================

/* ========= Gets input from the keyboard via the Arduino IDE monitor ========= 
 * Special command characters i.e. those used to change the morse speed or recall 
 * preset buffers are extracted and processed separately. The remaining characters are 
 * converted to uppercase where necessary then inserted into an input buffer array. 
 */
void getTX_Characters()
{
  // ==== Get input characters from the serial keyboard ====
  while (Serial.available()!=0)                 // If a character is being read, do this
  { 
    inputChar = Serial.read();                  // Read user input into variable 'inputChar'
        
    // ==== Isolate special command characters from the raw keyboard input ====
    if ((int(inputChar) >= 91 & int(inputChar) <= 93)
      | (int(inputChar) == 96)
      | (int(inputChar) >= 123 & int(inputChar) <= 126))
      {
        processCommands();                     // Process special command sequences
        return;                                // Exit from here - command characters arent Morse Code
      }
//    else if((int(inputChar) == 10) | (int(inputChar) == 13))            // ignore CR or LF
//    {
//       return;                                 // and exit from here
//    }
      
    else  
    // ==== Put remaining valid characters into the Input Buffer ====
      {
        // Convert lowercase characters to uppercase
        if (int(inputChar) >= 97 &  int(inputChar) <= 122)  // Check for valid lowercase character
          {
            inputChar = int(inputChar) - 32;          // Convert character to uppercase
          }
    
        // Load the characters into the Input Buffer then increment the counter
        if (bufferIn_Count < bufferSize)              // If there is room in the buffer?
          {
            bufferIn[bufferIn_Count] = inputChar;     // Load the character into the buffer
            bufferIn_Count++;                         // Increment the buffer counter for the next character
          }
      }
  }
}


/* ====== Takes characters stored in the input buffer and sends them as morse code ======
 * Characters are taken from the first position in the buffer (bufferIn[0]). All characters
 * are then rotated left putting the next available character in bufferIn[0]. The process
 * is continued until characters have been processed and bufferIn[0] has a value of 0 
 * indicating the buffer is empty. The buffer is then deliberately cleared by filling it 
 * with 0's. 
 */
void processTX_Buffer()
{
   // Get the character from the start of the buffer then rotate the rest of the character one space left
   if (int(bufferIn[0]) != 0)                   // If there are characters in the buffer
   {
      bufferChar = bufferIn[0];                 // Get the first character from the buffer
     
      // Rotate all the characters in the buffer 1 space to the left
      for (int K = 0; K <= bufferIn_Count; K++) // Loop for the number of characters in the buffer
      {
        bufferIn[K] = bufferIn[K+1];            // Copy buffer characters 1 position to the left
      }
        
      bufferIn_Count--;                         // Decrement the buffer counter for the next input read
      if (bufferIn_Count <= 0)                  // Make sure bufferIn_Count is never less than 0
      {
        bufferIn_Count = 0;                     // Buffer is empty
        memset(bufferIn, 0, bufferMax);         // For safety, clear empty buffer by filling with 0s
      }

      // ==================== Process Characters into Morse Code ============================
      // If the character is a SPACE, apply a 'word-space' delay
      if (bufferChar == 32)                        // If the character is a <SPACE> character
      {
        delay(wordSpace);                          // Insert a 'word-space' delay to the morse
        Serial.print(bufferChar);                  // Print a space on the Screen

#ifdef LCD_Yes        // ================= if using an LCD ==================
       LCDprint_TXChar();                      // Print the space on the LCD
#endif
      }
      else if ((bufferChar == 10) | (bufferChar == 13)) // If the character is a CR or LF
      {
        // Process character on the LCD
        #ifdef LCD_Yes        // ================= if using an LCD ==================
          bufferChar = 32;                       // Change the character to a space
          LCDprint_TXChar();                     // Print the space on the LCD
        #endif

        // Process character on the PC Screen
        Serial.println();   // Print a CR/LF on the screen
      }      
        
      // Otherwise check for illegal punctuation characters
      else if((bufferChar >= 34 && bufferChar <= 38) 
                | (bufferChar >= 42 && bufferChar <= 43)
                | (bufferChar >= 59 && bufferChar <= 60)
                | (bufferChar == 62))
      {
        Serial.print("#");                         // Print '#' if the character is not recognised
      }
        
      // Otherwise if the buffer is empty
      else if (bufferChar == 0 && bufferIn_Count == 0)   
      {
         memset(bufferIn, 0, bufferMax);           // Fill buffer with 0s to clear it
      }

       else
       // Search the 'mySet[]' Array for a matching character using 'morseNum' values 
       // from 0 to 64. The resulting bit value of 'morseNum' holds the Morse Code
      {
        morseNum = 0;                           // start the array counter at 0
        do  
        {
          morseChar = mySet[morseNum];          // Read a character from the array
          morseNum++;                           // Increment the counter

        // Repeat the loop until a character match is found in the array
        // or 'morseNum' exceeds the length of the array (63)
        } while ((morseChar != bufferChar) && (morseNum <= 64)); 
          morseNum--;                 // Restore incremented value of 'morseNum'

        // If the character was found in the array, send and print the character
        if (morseNum <= 63)           
        {
          sendCharacter(morseNum);    // Convert character to morse and send
          delay(characterSpace);      // Add the correct space between characters
        }
        else
        // Otherwise check the character for valid punctuation and send/print
        {
          getPunctuation();         // Look for the character in the  Punctuation table
          sendCharacter(morseNum);    // Convert character to morse and send
          delay(characterSpace);      // Add the correct space between characters
        }

        // This routine has been deemed unnecessary in version 2.2 and has been disabled
/*      if (bufferChar == 46)                   // If the character was a full-stop (.)
        {
          Serial.println();                    // Print Line Feed on the display when a (.) is sent
        }
*/
        Serial.print(bufferChar);               // Display the character being sent

#ifdef LCD_Yes        // ================= if using an LCD ==================
        LCDprint_TXChar();                    // Print the character on the LCD
#endif
      }
  }
}


/*============= Decodes the embedded morse in 'morseNum' and sends it.=============
 * The morse code is embedded in the value of 'morseNum'. The bits in 'morseNum' are 
 * rotated left and bit 7 of 'morseNum' is checked for the first logic 1 bit. 
 * This indicates the start of the morse character so this bit is discarded as the 
 * actual morse character is encoded in the bits that follow. 
 * The bits continue to be rotated left and the subsequent 1's or 0's at bit 7 are 
 * then used to call the morse_Dah or morse_Dit functions that send the morse code.
 */
void sendCharacter(int morseNum)
{
  int bitCount = 0;               // Keeps track of the number of bit rotations (total 8)

// =========== Rotate all bits to the left until the morse-flag bit is found at bit 7 ================
// ================== Subsequent bits contain the morse code  =======================  

  // Find the Morse-flag
  do                                                                          
    {
      morseNum = morseNum << 1;              // Rotate the left-most bit in 'morseNum' 
      bitCount++;                            // Increment the counter to keep track of the number of bits processed
    } while (bitRead(morseNum, 7) != 0x01);  // Loop until the 7th bit value = 1


  // Extract the morse code using the subsequent bits in morseNum
  for (int I= bitCount+1; I <= 7; I++)       // Loop for the remaining bits in morseNum
  {
    morseNum = morseNum << 1;                // Rotate the left-most bit in 'morseNum' 
    if (bitRead(morseNum, 7) != 0x01)        // if the 7th bit is not a 1
    {
      morse_Dah();                           // Send a DAH
    }
    else                                     // Otherwise
    {
      morse_Dit();                           // Send a DIT
    }
  }
}


/* =================== Morse Timing ================================
 * Defines the timing ratios of each element of the morse code.
 * The specific timing used is based on the morse speed in variable 'morseSpeed'. 
 * The morse speed can be selected by the user using command characters ` and ~. 
 * After each morse speed change by the user, this function is called to reset the 
 * morse speed parameters for dits, dahs and character/word spacing.
 * === Ratios are as list below === 
 *   DIT Length Ratio = 1
 *   DAH Length Ratio = 3
 *   Element Space Ratio = 1
 *   Character Space Ratio = 3
 *   Word Space Ratio = 7
 *   DIT Length = 1200/WPM (based on the word 'PARIS')
 */
void morseTiming()
{
  // Morse Code timing parameters
  ditLength = 1200/morseSpeed;      // Length of a dit based on the defined Morse Speed
  dahLength = ditLength * 3;        // Length of a Dah referenced to the Dit length
  characterSpace = ditLength * 3;   // Space between characters referenced to the Dit length
  elementSpace = ditLength;         // Space between elements referenced to the Dit length
  wordSpace = ditLength * 7;        // Space between words referenced to the Dit length
/*
  // Print the new morse speed value
  Serial.print("Speed = ");
    if (morseSpeed < 10)
  {
    Serial.print("0");                   // Print a leading 0
  }
  Serial.print(morseSpeed);             // Print the Morse Speed
  Serial.println(" WPM");  
*/
#ifdef LCD_Yes    // ================= if using an LCD ==================
  // Print default morse speed on the LCD
  lcd.setCursor(0,1);                 // Position the cursor
  if (morseSpeed < 10)
  {
    lcd.print("0");                   // Print a leading 0
  }
  lcd.print(morseSpeed);              // Print morse speed
  lcd.print(" WPM"); 
#endif

}


/*  ====================== Send DITs and DAHs ==========================
 *  Switch arduino pins HIGH or LOW to send morse code.
 *  Three pins are used for various morse code outputs.
 *  
 *  The =LED= pin drives an LED to provide a visual indication that your 
 *  morse code is being sent. 
 *  
 *  The =CODE= pin drives the radio's morse-key input via a 2N2222 transistor
 *  to send the morse code over the radio. 
 *  
 *  The =RXin= pin connects directly to the digital input pin of a separate 
 *  WB7FHC Morse Decoder unit. This provides an identical signal to the output 
 *  of the WB7FHC morse decoder's LM567 tone decoder IC, allowing your 
 *  transmitted morse to be decoded and displayed on the decoder's LCD. 
 *  You should not attempt to receive and send morse code at the same time as 
 *  the two signals will confuse the decoder.
 *  
 *  NOTE: The signal logic on the =RXin= pin is inverted compared to the 
 *  =LCD= and =CODE= pins which is why it requires a separate pin.
 */

// ========= For Morse Paddle and Manual Key ========================
// Send morse Code Manually
void morse_On()                           // Switch the Morse Tone ON
{
  toneStartTime = millis();               // Read the clock when the tone starts
  digitalWrite(morsePin_LED, HIGH);       // Take LED Pin HIGH
  digitalWrite(morsePin_CODE, HIGH);      // Take CODE Pin HIGH
  digitalWrite(morsePin_RXin,LOW);        // Take RX Input LOW
}

void morse_Off()                          // Switch the Morse Tone OFF
{
  toneEndTime = millis();                 // Read the clock when the tone stops
  digitalWrite(morsePin_LED, LOW);        // Take LED Pin LOW
  digitalWrite(morsePin_CODE, LOW);       // Take CODE Pin LOW
  digitalWrite(morsePin_RXin,HIGH);       // Take RX Input HIGH
}


// ==== Auto timing Morse Code for KEYBOARD and Buffered Morse ====
// Send an Auto DIT 
void morse_Dit()                          // Send a DIT
{
  digitalWrite(morsePin_LED, HIGH);       // Take LED Pin HIGH
  digitalWrite(morsePin_CODE, HIGH);      // Take CODE Pin HIGH
  digitalWrite(morsePin_RXin,LOW);        // Take RX Input LOW
  
  delay(ditLength);                       // Hold the Pins High for the length of a DIT
  
  digitalWrite(morsePin_LED, LOW);        // Take LED Pin LOW
  digitalWrite(morsePin_CODE, LOW);       // Take CODE Pin LOW
  digitalWrite(morsePin_RXin,HIGH);       // Take RX Input HIGH

  delay(elementSpace);                    // Add an ELEMENT space at the end of the DIT
}

// Send an Auto DAH 
void morse_Dah()                          // Send a DAH
{
  digitalWrite(morsePin_LED, HIGH);       // Take LED Pin HIGH
  digitalWrite(morsePin_CODE, HIGH);      // Take CODE Pin HIGH
  digitalWrite(morsePin_RXin,LOW);        // Take RX Input LOW

  delay(dahLength);                       // Hold the Pins High for the length of a DAH
  
  digitalWrite(morsePin_LED, LOW);        // Take LED Pin LOW
  digitalWrite(morsePin_CODE, LOW);       // Take CODE Pin LOW
  digitalWrite(morsePin_RXin,HIGH);       // Take RX Input HIGH

  delay(elementSpace);                    // Add an ELEMENT space and the end of the DAH
}


#ifdef LCD_Yes  // ================== if using an LCD ==================
/* ============ This function can be include if using an LCD =============
 * This is a debugging function which can be used to display the number
 * of characters currently in the input buffer on an attached LCD. This is 
 * sometimes useful as the buffer has a limited size and will be overloaded 
 * if too many characters are entered.Unfortunately printing this value on 
 * the serial monitor isnt very useful as it cannot be displayed separately 
 * from the rest of the text. This function is really only useful when using 
 * an LCD as the value can then be displayed in a specific location on the 
 * LCD away from other content.
 */
 
void LCD_Buffer_Count()
{
  lcd.setCursor(0,0);                     // Position the cursor

  if (bufferIn_Count < 10)                // If the buffer counter is less than 10
  {
   lcd.print('0');                        // Print a preceeding 0
  }
     
  lcd.print(bufferIn_Count);              // Then print the number
}


/*  ============ Include this function include if using an attached LCD =============
 *  This function displays your transmitted characters on a connected LCD. 
 *  If you prefer to connect an LCD, please enable this code.
 *  Versions of this program that display the transmitted text using the 
 *  arduino IDE monitor are not required to enable this function
 */
 
void LCDprint_TXChar()
{
  int I;                                         // Temporary loop-counter variable
    
  if (txLCD_Count < 19)                          // While the cursor position is less that 19
  {
    lcd.setCursor(txLCD_Count,LCD_Line);         // Set the cursor position
    lcd.print(bufferChar);                       // Print the latest character
    txBuffer[txLCD_Count] = bufferChar;          // Load the latest character into the buffer
    currentLine[txLCD_Count] = bufferChar;       // Load the latest character into the line array
    if (bufferChar == 32)                        // If the character is a <space> 
    {
      lastSpace = txLCD_Count;                   // Store the cursor position
    }
    txLCD_Count++;                               // Increment the cursor position
  }
  else                                            // otherwise once the cursor position has reached 19
  {
    // otherwise once the cursor position has reached 19
    lcd.setCursor(txLCD_Count,LCD_Line);          // Set the cursor position
    lcd.print(bufferChar);                        // Print the latest character
    txBuffer[txLCD_Count] = bufferChar;           // Load the latest character into the buffer
    currentLine[txLCD_Count] = bufferChar;         // Load the latest character into the line array
    if (bufferChar == 32)                        // If the character is a <space> 
    {
      lastSpace = txLCD_Count;                   // Store the cursor position
    }
    
    // === If the end of line 3 has been reached,scroll the text to line 2 ===
    
    // Copy the text from the current line into line 2 
    for (I = 0; I <= 19; I++)        // Copy only the text up to the last space
    {
      lcd.setCursor(I,2);                        // Set the cursor position to line 2
      lcd.print(currentLine[I]);                 // Move the current line (up to the last space)
      lcd.setCursor(I,3);                        // Set the cursor position to line 3
      lcd.print(" ");                            // Clear Line 3
    }
    
    // == Process characters after the last space ===
    
    // Overwrite any text copied onto line 2 that was after the last space
     for (I = lastSpace; I <= 19; I++)           // From the last space to the end
    {
      lcd.setCursor(I,2);                        // Set the cursor position to line 2
      lcd.print(" ");                            // Clear the text
    }

    // Print the remaining characters from line 2 on line 3
    txLCD_Count = 0;                           // Restart the LCD position counter
    for (I = lastSpace+1; I <= 19; I++)        // // Copy the missing characters to Line 3
    {
      lcd.setCursor(txLCD_Count,3);            // Set the cursor position to line 3
      lcd.print(currentLine[I]);               // Print the missing characters on line 3
      txLCD_Count++;                           // Increment the counter for the next character
    }
  }

}

void LCDprint_Char()
{
  int I;                                       // Temporary loop-counter variable
  
  if (txLCD_Count < 19)                        // While there is still room on the line
  {
    lcd.setCursor(txLCD_Count,LCD_Line);       // Set the cursor position
    lcd.print(morseChar);                      // Print the latest character
    txBuffer[txLCD_Count] = morseChar;         // Load the latest character into the TX buffer
    currentLine[txLCD_Count] = morseChar;      // Load the latest character into the line array
    
    // Is the character a <space>?
    if (morseChar == 32)                       // If the character is a <space> 
    {
      lastSpace = txLCD_Count;                 // Store the current cursor position
    }
    
    txLCD_Count++;                             // Increment the cursor position
  }
  else 
  {
    // otherwise once the cursor position has reached 19
  
    lcd.setCursor(txLCD_Count,LCD_Line);       // Set the cursor position
    lcd.print(morseChar);                      // Print the latest character
    txBuffer[txLCD_Count] = morseChar;         // Load the latest character into the TX buffer
    currentLine[txLCD_Count] = morseChar;      // Load the latest character into the line array
    
    // Is the character a <space>?
    if (morseChar == 32)                       // If the character is a <space> 
    {
      lastSpace = txLCD_Count;                 // Store the cursor position
    }

    // === If the end of line 3 has been reached,scroll the text to line 2 ===
    
    // Copy the text from the current line into line 2 
    for (I = 0; I <= 19; I++)                  // Copy the current line of text into line 2
    {
      lcd.setCursor(I, 2);                     // Set the cursor position to line 2
      lcd.print(currentLine[I]);               // Move the current-line character to line 2
      lcd.setCursor(I, 3);                     // Change the cursor position to line 3
      lcd.print(" ");                          // Clear the character position in Line 3
    }
    
    // == Process characters after the last space ===
    
    // Overwrite any text copied onto line 2 that was after the last space
    for (I = lastSpace; I <= 19; I++)          // From the last space to the end
    {
      lcd.setCursor(I,2);                      // Set the cursor position to line 2
      lcd.print(" ");                          // Clear the text
    }

    // Print the remaining characters from line 2 on line 3
    txLCD_Count = 0;                           // Restart the LCD position counter
    for (I = lastSpace + 1; I <= 19; I++)      // Copy the missing characters to Line 3
    {                                          
      lcd.setCursor(txLCD_Count, 3);           // Set the cursor position on line 3
      lcd.print(currentLine[I]);               // Print the remaining characters on line 3
      txLCD_Count++;                           // Increment the counter for the next character
    }
  }
}

#endif


/* ====== Keyboard Command Character Functions for recalling Preset Buffers ==================
 * These functions that are called by the processCommands() function to send messages 
 * stored in the preset buffers. There are 6 preset buffers driven by keyboard 
 * commands using special characters as listed below.
 * [ = buffer #1
 * ] = buffer #2
 * \ = buffer #3
 * { = buffer #4
 * } = buffer #5
 * | = buffer #6
 * 
 */
// Get characters from the CQ_Mess buffer and put them in the Input buffer 
void sendCQ_Mess()
{
  for (int PP=0; PP <= sizeof(CQ_Mess); PP++)     // For the size of the buffer
  {
    inputChar = CQ_Mess[PP];                      // Read character into variable 'inputChar'

    if (inputChar !=0)                            // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)            // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;      // Load the character into the buffer
       bufferIn_Count++;                          // Increment the buffer counter for the next character
      }
    }
    else                                          // Otherwise its the end of the buffer
    {
      return;                                     // Return. We're done here
    }
  } 
}


// ========== Get characters from the ANT_Mess buffer and put them in the Input buffer ==========
void sendANT_Mess()
{
  for (int PP=0; PP <= sizeof(ANT_Mess); PP++)    // For the size of the buffer
  {
    inputChar = ANT_Mess[PP];                     // Read character into variable 'inputChar'

    if (inputChar !=0)                            // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)            // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;      // Load the character into the buffer
       bufferIn_Count++;                          // Increment the buffer counter for the next character
      }
    }
    else                                          // Otherwise its the end of the buffer
    {
      return;                                     // Return. We're done here
    }
  } 
}



// ========== Get characters from the CQTest_Mess buffer and put them in the Input buffer ==========
void sendCQTest_Mess()
{
  for (int PP=0; PP <= sizeof(CQTest_Mess); PP++)    // For the size of the buffer
  {
    inputChar = CQTest_Mess[PP];                     // Read character into variable 'inputChar'
        
    if (inputChar !=0)                               // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)               // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;         // Load the character into the buffer
       bufferIn_Count++;                             // Increment the buffer counter for the next character
      }
    }
    else                                             // Otherwise its the end of the buffer
    {
      return;                                        // Return. We're done here
    }
  } 
}



// ========== Get characters from the QTH_Mess buffer and put them in the Input buffer ==========
void sendQTH_Mess()
{
  for (int PP=0; PP <= sizeof(QTH_Mess); PP++)     // For the size of the buffer
  {
    inputChar = QTH_Mess[PP];                      // Read character into variable 'inputChar'
        
    if (inputChar !=0)                             // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)             // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;       // Load the character into the buffer
       bufferIn_Count++;                           // Increment the buffer counter for the next character
      }
    }
    else                                           // Otherwise its the end of the buffer
    {
      return;                                      // Return. We're done here
    }
  } 
}


// ========== Get characters from the NAME_Mess buffer and put them in the Input buffer ==========
void sendNAME_Mess()
{
  for (int PP=0; PP <= sizeof(NAME_Mess); PP++)     // For the size of the buffer
  {
    inputChar = NAME_Mess[PP];                      // Read character into variable 'inputChar'
        
    if (inputChar !=0)                              // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)              // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;        // Load the character into the buffer
       bufferIn_Count++;                            // Increment the buffer counter for the next character
      }
    }
    else                                            // Otherwise its the end of the buffer
    {
      return;                                       // Return. We're done here
    }
  } 
}

// ========== Get characters from the SIGNAL_Mess buffer and put them in the Input buffer ==========
void sendSIGNAL_Mess()
{
  for (int PP=0; PP <= sizeof(SIGNAL_Mess); PP++)    // For the size of the buffer
  {
    inputChar = SIGNAL_Mess[PP];                     // Read character into variable 'inputChar'
        
    if (inputChar !=0)                               // As long as the next character isnt a 0
    {
      if (bufferIn_Count < bufferSize)               // If there is room in the buffer?
      {
       bufferIn[bufferIn_Count] = inputChar;         // Load the character into the buffer
       bufferIn_Count++;                             // Increment the buffer counter for the next character
      }
    }
    else                                             // Otherwise its the end of the buffer
    {
      return;                                        // Return. We're done here
    }
  } 
}  

// =================== Identify punctuation characters ====================
void getPunctuation() 
{
  // Punctuation marks are made up of more dits and dahs than
  // letters and numbers. Rather than extend the character array
  // out to reach these higher numbers we will simply check for
  // them here. This funtion only gets called when morseNum is greater than 63
  
  // Thanks to Jack Purdum for the changes in this function
  // The original uses if then statements and only had 3 punctuation
  // marks. Then as I was copying code off of web sites I added
  // characters we don't normally see on the air and the list got
  // a little long. Using 'switch' to handle them is much better.

  switch (bufferChar) 
  {
    case ':':                 // ':'  [- - - . . .]
      morseNum = 71;          // Set Morsecode number
      break;

    case ',':                 // ','  [- - . . - -] 
      morseNum = 76;          // Set Morsecode number
      break;

    case '!':                 // '!'  [- . - . - -]
      morseNum = 84;          // Set Morsecode number
      break;

    case '-':                 // '-'  [- . . . . -]
      morseNum = 94;          // Set Morsecode number
      break;

    case 39:                  // Apostrophe  [. - - - - .]
      morseNum = 97;          // Set Morsecode number
      break;

    case '@':                 // 101, '@' [. - - . - .]
      morseNum = 101;         // Set Morsecode number
      break;

    case '.':                 // 106, '.' [. - . - . -]
      morseNum = 106;         // Set Morsecode number
      break;

    case '?':                 // 115, '?' [. . - - . .] 
      morseNum = 115;
      break;

    case '$':                 // 246, '$' [. . . - . . -]
      morseNum = 246;         // Set Morsecode number
      break;

    default:
      morseNum = '0';         // Should not get here
      break;
  }
}


void sendPunctuation() 
{
  // Punctuation marks are made up of more dits and dahs than
  // letters and numbers. Rather than extend the character array
  // out to reach these higher numbers we will simply check for
  // them here. This function only gets called when myNum is greater than 63
  
  // Thanks to Jack Purdum for the changes in this function
  // The original uses if then statements and only had 3 punctuation
  // marks. Then as I was copying code off of web sites I added
  // characters we don't normally see on the air and the list got
  // a little long. Using 'switch' to handle them is much better.


  switch (morseNum) 
  {
    case 71:
      morseChar = ':';
      break;
    case 76:
      morseChar = ',';
      break;
    case 84:
      morseChar = '!';
      break;
    case 94:
      morseChar = '-';
      break;
    case 97:
      morseChar = 39;    // Apostrophe
      break;
    case 101:
      morseChar = '@';
      break;
    case 106:
      morseChar = '.';
      break;
    case 115:
      morseChar = '?';
      break;
    case 246:
      morseChar = '$';
      break;
    case 122:
      morseChar = 'k';
      break;
    default:
      morseChar = '#';             // Should not get here
      break;
  }
    LCDprint_Char();               // Print the character on the LCD
    
    // If the character is a PERIOD, add a Line Feed to the monitor display
    if (morseNum == 106)
    {
      Serial.print(morseChar);     // Print the character on the Monitor
      Serial.print(char(10));      // Print a Line Feed
    }
}


// ============ Process special COMMAN characters to access preset buffers ==============
// ======================= and change Morse Speed ================================
void processCommands()
{
   switch (inputChar)       // Check the value of the raw keyboard input
  {
    case 91:                        // '[' pressed - Recall Buffer #1 =====
      sendCQ_Mess();                // Process Send CQ message 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;
      
    case 92:                        // '\' pressed - Recall Buffer #3 =====
      sendQTH_Mess();               // Process QTH buffer 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;

    case 93:                        // ']' pressed - Recall Buffer #2 =====
      sendNAME_Mess();              // Process NAME buffer 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;
       
    case 123:                       // '{' pressed - Recall Buffer #4 =====
      sendSIGNAL_Mess();            // Process the SIGNAL Strength buffer 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;
      
    case 124:                       // '|' pressed - Recall Buffer #6 =====
      sendCQTest_Mess();            // Process buffer #1 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed
      break;

    case 125:                       // '}' pressed - Recall Buffer #5 =====
      sendANT_Mess();               // Process ANTENNA buffer 
      processTX_Buffer;             // Send as morse
      Serial.println();             // Print a linefeed 
      break;
     
    default:                        // Capture anything else here
    break;
  }
}

// ==== Prints the manual Morse from the key or paddle to the LCD and monitor ====
void printDitDah()
{
  
  // If the required time has passed and there is no further incoming tone
  // then process the character
  if ((character) & (toneStartTime == 0))
  {
    InTime = millis();             // Grab the clock
    if ((InTime - OutTime) >= elementL * 2)  // If elapsed time matches required character delay?
    {
      if (morseNum > 63)           // If it's a punctuation character
      {                                 // Then get character from punctuation table
        sendPunctuation();              // Print the punctuation character
        character = false ;             // Character is done - clear the flag
        space = true;                   // Enable space flag
        morseNum = 1;                   // Reset morseNum for new character
      }
      else                         // Otherwise it's a normal character
      {
        morseChar = mySet[morseNum];    // Get character from table
        LCDprint_Char();                // Print the character on the LCD
        Serial.print(morseChar);        // Print the character on the monitor
        character = false ;             // Character is done - clear the flag
        space = true;                   // Enable space flag
        morseNum = 1;                   // Reset morseNum
      }
    }

  }    
  // If the required time has passed and there is no further incoming tone
  // then process a word space
  else if ((space) & (toneStartTime == 0))
  {
    InTime = millis();                        // Grab the clock
    if ((InTime - OutTime) >= elementL * 7)   // Is it the end of a word?
    {
      morseChar = 32;               // Select the space character
      LCDprint_Char();              // Print the space character on the LCD
      Serial.print(" ");            // Print a space on the Monitor
      space = false;                // A space is done - clear the flag
    }
  }

  // Looking for the first tone pass or a pass with no change to the  End Tone timing
  if ((toneEndTime == 0 ) || (toneEndTime == old_toneEndTime) || (toneEndTime == toneStartTime)) // If First pass or if incomplete tone
  {
    old_toneEndTime = toneEndTime;  // Update the old_ToneEndTime flag
  }

  // Compare the incoming tone Length to the average Element Space
  else if ((toneEndTime - toneStartTime) < elementL * 2)  // Is the tone length less than our default
  {                                                    // If so - its a DIT
    DitL = toneEndTime - toneStartTime;                // Calculate actual length of the DIT
    DahL = DitL * 3;                                   // Calculate length of a Dah
    elementL = DitL;                                   // Calculate the length of an element space

    // Add a '1' bit to character variable
    morseNum = morseNum << 1;       // Rotate 1 bit to the left
    morseNum++;                     // Increment value by 1
    character = true;               // OK for character?
    OutTime = toneEndTime;          // Mark the end of the tone
    toneStartTime = 0;              // Clear toneStartTime variable
  }
  else if ((toneEndTime - toneStartTime) > elementL * 2)  // Else if the tone length is longer
  {                                                    // Otherwise it's a DAH
    DahL = toneEndTime - toneStartTime;                // Calculate actual length of the DAH
    DitL = DahL / 3;                                   // Calculate the length of a DIT
    elementL = DitL;                                   // Calculate the length of an element space

    // Add a '0' bit to character variable
    morseNum = morseNum << 1;       // Rotate 1 bit to the left
    character = true;               // OK for character
    OutTime = toneEndTime;          // Mark the end of the tone
    toneStartTime = 0;              // Clear toneStartTime variable
  }
  
  old_toneEndTime = toneEndTime;    // Update the old_toneEndTime variable
}


// Processes the DIT / DAH Timing for the Morse paddle
void paddleTimer()
{
  // Timing is done live using milli timers so as to not require using delays
  // for the morse on/off/spacing timing

  // Check to see if Tone-On time has been completed for DIT or DAH
  countTime = millis();               // Check the current Arduino clock time
  if ((dahToneFlag) & (countTime - startTime >= dahLength)) // Check Dah timer
  {
    toneON = countTime - startTime;   // Store the tone ON time
    endTime = countTime;              // Get current clock time
    morse_Off();                      // Switch OFF the tone
    dahToneFlag = false;              // Tone is Off (ensures only 1 pass here)
  }
  if ((ditToneFlag) & (countTime - startTime >= ditLength)) // Check Dit timer
  {
    toneON = countTime - startTime;   // Store the tone ON time
    endTime = countTime;              // Get current clock time
    morse_Off();                      // Switch OFF the tone
    ditToneFlag = false;              // Tone is Off (ensures only 1 pass here)
  }

  // Check to see if the element space has expired
  // Dah element spacing
  countTime = millis();               // Check the current Arduino clock time
  if ((dahOffFlag) & (!dahToneFlag) & (countTime - endTime >= elementSpace))
  {
    toneOFF = countTime - endTime;    // Store the tone OFF time
    dahOffFlag = false;               // Dah completed
  }
  // Dit element spacing
  if ((ditOffFlag) & (!ditToneFlag) & (countTime - endTime >= elementSpace))
  {
    toneOFF = countTime - endTime;    // Store the tone OFF time
    ditOffFlag = false;               // Dit completed
  }
}
