# A6-intercom-v2
Improved version of my intercom (https://github.com/Wim3d/A6_intercom)
In this version the communication between Arduino and A6 module is improved via hardware serial.
You can also call this intercom from your phone if the number is programmed in the intercom.

The code is inspired by some people:

https://www.cooking-hacks.com/documentation/tutorials/how-to-make-a-caller-identifier-with-leds-using-gsm-gprs/

https://youtu.be/ers6Pq0ck38

Important: The default baud rate of the A6 module is 115200 and an Arduino cannot handle this speed via software serial, therefore I used hardware serial for the communication between A6 module and Arduino.
I tried to use a ESP8266 which can handle software serial at a speed of 115200 BAUD, however I did not succeed in getting a stable serial connection at startup.
