# Morse-Code-Encoder-v2.2

UPDATE version 2.2 | March 7, 2023
Fine tuned the code to better format text on the LCD and PC screen.
1. Print a space on the LCD to seperate text everytime the <Enter> key is pressed on the keyboard.
2. Removed the code that automatically (and unnecessarily) printed a CR/LF on the PC screen whenever a period (.) was printed.

UPDATE Version 2.1 | March 6, 2023
After receiving a report of the version 2.0 software sending an additional morse "1" character after each keyboard text, I corrected a bug that caused Line Feed Characters from the keyboard to be processed through the morse buffer. 


==================================================================================================

This is my latest v2.2 Morse Encoder which is a significant improvement on my original v1 Morse Encoder. For full project details go to http://vk2idl.com . I have left my v1 Morse Encoder on github for reference but I recommend using this version instead.

My original Morse Encoder project brief was to create a device that allowed Morse Code to be sent by typing text on a keyboard rather than using a Morse key. It was initially targeting those who were new to Morse Code or who were no longer as proficient as they once were. Hardware inputs were also provided to allow a manual key and a Morse Paddle to be connected, but they were added as secondary option to using the keyboard.

While the keyboard option work quite well, it locked the device to the Arduino IDE which reduced its versatility. To make the device more portable I needed to add an LCD to display the Morse being sent. This also required a software Morse decoder to decode and display live Morse from the paddle or straight key directly onto the LCD. To complete the portability requirements I also included a rotary control for adjusting the Morse Speed. The unit is now completely portable for users who wish to send Morse by hand, but can still be connected to the Arduino IDE (if the user prefers) to type their Morse code via a keyboard.

For a future project, I’m considering adding a direct keyboard connection but that will require a significant hardware change, perhaps to an ESP-32 module with Bluetooth. In the mean time Ive preferred to keep it simple.

The new software changes highlighted a limitation within my original code. I had kept the software simply by using the ‘delay()’ function to control the timing of the Morse elements. These fixed delays were causing delays with other portions of the program including the smooth scrolling of characters on the LCD. To correct this I completely rewrote the Morse generation code to use ‘millis()’ timing counts from the Arduino clock to determine the passing of time. This allowed me to do other functions concurrent with the generation of the Morse code. The LCD display scrolling system was also rewritten to make it more efficient and includes a 2-line scroll with word completion to ensure that whole words are not truncated during the end-of-line scrolling process. Finally a Morse Decoder function was added to read the live code from the Paddle or the Straight Key and seamlessly display it on the LCD. The result is a faster and much more streamline code set.

Currently the encoder can send Morse code at speeds of 6 to 30 wpm (set by the rotary control) using either a Morse Paddle or a straight Morse key, or via a keyboard using the Arduino IDE. When using the Paddle, the dit/dah timing is automatic based on the setting of the rotary Morse Speed control. The Morse speed limits can be adjusted in the software.

The completed unit is powered through the USB socket on the Arduino Nano.



