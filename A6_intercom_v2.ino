/*
   Author Wim Hoogervorst, May 2018
*/

#include <SoftwareSerial.h>   // SoftwareSerial for debugging only
#define A6Serial Serial       // A6 module is connected tot hardware serial

// define pins
#define BLUELEDPIN 5        // blue led connected to pin 5
#define GREENLEDPIN 4       // green led connected to pin 4

#define SERIALDEBUG 0       // define Serial debug on or off
SoftwareSerial DebugSerial(8, 9); // RX, TX

//variables for interrupt switch
volatile int state = LOW;
uint8_t interruptPin = 2;

// variables for the incoming phone numbers
const char number1[] = {"XXXXXXXXXXX"};
const char number2[] = {"XXXXXXXXXXX"};
const char number3[] = {"XXXXXXXXXXX"};

char phone_number[9];
char received[13];
// outgoing phone number
char phone_no[] = "XXXXXXXXXX"; // tel nr Fruitspecialist

int8_t answer;

int32_t maxoutgoing = 120000;   // 120000 ms max outgoing call duration is 2 min
int32_t maxincoming = 600000;   // 600000 ms max incoming call duration is 10 min
int32_t calltime = 0;
int32_t starttime = 0;

void setup()
{
  delay(500);
  pinMode(BLUELEDPIN, OUTPUT);
  pinMode(GREENLEDPIN, OUTPUT);
  pinMode(interruptPin, INPUT_PULLUP);
  // Open Serial communications
  if (SERIALDEBUG)
  {
    DebugSerial.begin(57600);
  }
  A6Serial.begin(115200);
  digitalWrite(GREENLEDPIN, HIGH);
  delay(500);
  digitalWrite(GREENLEDPIN, LOW);
  DebugSerial.println("");
  DebugSerial.println("start setup");
  smartDelay(17000);
  A6start();
  DebugSerial.println("end of setup");
  attachInterrupt(digitalPinToInterrupt(interruptPin), IS_routine, LOW); //pin is pulled up in normal state
  state = LOW;
  digitalWrite(GREENLEDPIN, HIGH);
}

void loop()
{
  DebugSerial.println("loop");
  //  first handle the interrupt request
  if (state == HIGH)
  {
    digitalWrite(BLUELEDPIN, HIGH);
    digitalWrite(GREENLEDPIN, LOW);
    A6Serial.print("ATD");
    A6Serial.println(phone_no); // call number
    smartDelay(2000);
    state = LOW;
    DebugSerial.print("value of state before while loop: ");
    DebugSerial.println(state);
    starttime = millis();
    int8_t answer2 = sendATcommand("", "ERROR", 1000);    //checks whether the caller hang up
    while (answer2 == 0 && calltime < maxoutgoing && state == LOW)        // loop for the call, ended by timer or a hang up from both sides
    {
      DebugSerial.print("value of state in while loop: ");
      DebugSerial.println(state);
      answer2 = sendATcommand("", "ERROR", 1000);
      calltime = millis() - starttime;
    }
    DebugSerial.print("value of state after while loop: ");
    DebugSerial.println(state);
    sendATcommand("ATH", "OK", 1000);                     // hang up the A6 module
    smartDelay(1000);
    state = LOW;
    digitalWrite(BLUELEDPIN, LOW);
    digitalWrite(GREENLEDPIN, HIGH);
  }

  while (answer = sendATcommand("", "+CLIP", 1000))
  {
    //answer is 1 if sendATcommand detects +CLIP
    if (answer == 1)
    {
      DebugSerial.println("Incoming call");
      DebugSerial.print("Received data: ");

      for (int i = 0; i < 15; i++) {
        //read the incoming byte:
        while (A6Serial.available() == 0)
        {
          delay (50);
        }
        //stores phone number
        received[i] = A6Serial.read();
        DebugSerial.print(received[i]);
      }
      DebugSerial.println("");
      A6Serial.flush();

      byte j = 0;
      //phone number comes after quotes (") so discard all bytes until find'em
      while (received[j] != '"') j++;
      j++;
      for (byte i = 0; i < 11; i++) {

        phone_number[i] = received[i + j];

      }
    }
    DebugSerial.print("Phone number: ");
    for (int i = 0; i < 11; i++) {
      // Print phone number:
      DebugSerial.print(phone_number[i]);
    }
    DebugSerial.println("");
    switch (compareNumber(phone_number)) {

      case 0:
        {
          DebugSerial.print("Unknown number, do not answer");
          break;
        }
      case 1:
        {
          DebugSerial.print("known number, answer call");
          //answercall(60000);
          digitalWrite(BLUELEDPIN, HIGH);
          digitalWrite(GREENLEDPIN, LOW);
          smartDelay(1000);
          sendATcommand("ATA", "OK", 1000);
          state = LOW;
          starttime = millis();
          int8_t answer2 = sendATcommand("", "ERROR", 1000);  //checks whether the caller hang up or the intercom hang up
          while (answer2 == 0 && calltime < maxincoming && state == LOW)
          {
            answer2 = sendATcommand("", "ERROR", 1000);
            calltime = millis() - starttime;
          }
          sendATcommand("ATH", "OK", 1000);                   // hang up the A6 module
          smartDelay(1000);
          digitalWrite(BLUELEDPIN, LOW);
          digitalWrite(GREENLEDPIN, HIGH);
          state = LOW;
          break;
        }
    }
  }
}

void A6start()
{
  DebugSerial.println("A6 begin...");
  //smartDelay(2000);

  answer = 0;

  // checks if the module is started
  answer = sendATcommand("AT", "OK", 2000);
  if (answer == 0)
  {
    // waits for an answer from the module
    while (answer == 0)
    { // send AT every two seconds and wait for the answer
      answer = sendATcommand("AT", "OK", 2000);
      }
  }
  int8_t answer1 = 0;
  smartDelay(1000);
  answer1 = sendATcommand("AT+CREG?", "+CREG: 1,1", 2000);
  if (answer1 == 0)
  {
    // waits for an answer from the module
    while (answer1 == 0)
    { // checks whether module is connected to network
      answer1 = sendATcommand("AT+CREG?", "+CREG: 1,1", 2000);
      }
  }
  int8_t answer2 = 0;
  smartDelay(1000);
  answer2 = sendATcommand("AT+CLIP=1", "OK", 2000);
  if (answer2 == 0)
  {
    // waits for an answer from the module
    while (answer2 == 0)
    { // checks whether module is connected to network
      answer2 = sendATcommand("AT+CLIP=1", "OK", 2000);
    }
  }
  sendATcommand("AT+CMGD=1,4", "OK", 5000);
}

int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout)
{
  uint8_t x = 0,  answer = 0;
  char response[100];
  unsigned long previous;

  memset(response, '\0', 100);    // Initialice the string
  delay(100);
  while ( DebugSerial.available() > 0) DebugSerial.read();   // Clean the input buffer
  A6Serial.println(ATcommand);    // Send the AT command
  //Serial.println(ATcommand);    // Print the AT command on DebugSerial
  x = 0;
  previous = millis();

  // this loop waits for the answer
  do {
    // if there are data in the UART input buffer, reads it and checks for the asnwer
    if (A6Serial.available() != 0) {
      response[x] = A6Serial.read();
      x++;
      // check if the desired answer is in the response of the module
      if (strstr(response, expected_answer) != NULL)
      {
        answer = 1;
      }
    }
    // Waits for the aswser with time out
  } while ((answer == 0) && ((millis() - previous) < timeout));
  DebugSerial.println(response);
  return answer;
}

//compares the incoming call phone number with known phones
byte compareNumber(char* number)
{
  for (byte i = 0; i < 11; i++) {
    if (number[i] == number1[i]) {
      if (i == 10) return 1;        // known number
    } else i = 11;
  }
  for (byte i = 0; i < 11; i++) {
    if (number[i] == number2[i]) {
      if (i == 10) return 1;        // known number
    } else i = 11;
  }
  for (byte i = 0; i < 11; i++) {
    if (number[i] == number3[i]) {
      if (i == 10) return 1;        // known number
    } else i = 11;
  }
  return 0;
}

static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do
  {
    if ( A6Serial.available())
    {
      DebugSerial.write( A6Serial.read());
    }
  } while (millis() - start < ms);
}

void IS_routine() {
  state = HIGH;
}
