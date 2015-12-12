/**
SIM 908 MODULE ARDUINO FIRMWARE
//make short description

TODO list:
1. Write setup for all features of module
2. Error handling, set critical error reporting
3. Write main loop to get gps and send it

*/

#include <SoftwareSerial.h>

#define SIM_908_PWR_PIN 9
#define SS_RX 10
#define SS_TX 11
#define SIM_908_PWR_RATE 170
#define GSM_LED_PIN 7
#define GPS_LED_PIN 8
#define BUTTON 12
#define PWR_LED_PIN 4
int buttonState = 0;
int timer = 0;
bool DEBUG = true;
uint8_t answer=0;


SoftwareSerial mySerial(SS_RX, SS_TX); // RX, TX


int8_t sendATcommand(const char* ATcommand, const char* expected_answer, unsigned int timeout){

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);    // Initialize empty string

    delay(100);

    while (Serial.available() > 0)
    {
      Serial.read();    // Clean the input buffer
    }

    Serial.print(ATcommand);    // Send the AT command 
    Serial.print("\r\n");

    x = 0;
    previous = millis();

    // this loop waits for the answer
    do {
        if(Serial.available() != 0)
        {
            // if there are data in the UART input buffer, reads it and checks for the asnwer
            response[x] = Serial.read();
            // printf("%c",response[x]); // i am fucked by this shit ALL BECAUSE OF YHIS FUCKING line!!!!!!!!!!
            x++;
            // check if the desired answer  is in the response of the module
            if (strstr(response, expected_answer) != NULL)    
            {
              answer = 1;
            }
        }
    }
    // Waits for the asnwer with time out
    while ((answer == 0) && ((millis() - previous) < timeout));    
    // Serial.println(response);

    return answer;
}

void ledFlash(int del, int pin, int count) {
  for (int i = 0; i < count; i++) {
    digitalWrite(pin, HIGH);
    delay(del);
    digitalWrite(pin, LOW);
    delay(del);
  }
}

void reboot() {
 uint8_t answer=0;

 answer = sendATcommand("AT+CFUN=1,1", "OK", 5000);
 if (answer == 0) {
   while(answer == 0){     // Send AT every two seconds and wait for the answer
       answer = sendATcommand("AT+CFUN=1,1", "OK", 5000);
     }
   }
}

void power_down() {
 uint8_t answer=0;
    
    // checks if the module is started
    answer = sendATcommand("AT+CPOWD=1", "NORMAL POWER DOWN", 2000);
    if (answer == 0)
    {
        while(answer == 0){     // Send AT every two seconds and wait for the answer
            answer = sendATcommand("AT+CPOWD=1", "NORMAL POWER DOWN", 2000);
        }
    }
}

void check_stat() {
 uint8_t answer=0;
    
  // checks if the module is started
  answer = sendATcommand("AT", "OK", 2000);
  if (answer == 0) {
    while(answer == 0){     // Send AT every two seconds and wait for the answer
      ledFlash(200, PWR_LED_PIN, 5);
      answer = sendATcommand("AT", "OK", 2000);
    }
  }
  if (answer == 1) {
    digitalWrite(PWR_LED_PIN, HIGH);
  }
}

void gps_up() {
 uint8_t pwr=0;
 uint8_t rst=0;
 uint8_t stat=0;

  pwr = sendATcommand("AT+CGPSPWR=1", "OK", 2000);
  rst = sendATcommand("AT+CGPSRST=1", "OK", 2000);

  while (pwr == 0 && rst == 0) { 
    pwr = sendATcommand("AT+CGPSPWR=1", "OK", 2000);
    rst = sendATcommand("AT+CGPSRST=1", "OK", 2000);
    ledFlash(1000, GPS_LED_PIN, 5); // TODO: set flashing for 1 second as a signal that smthng went wrong!
  }
  if(pwr == 1 && rst == 1) {
    stat = sendATcommand("AT+CGPSSTATUS", "2D Fix", 2000);
    while(stat == 0) {
      ledFlash(200, GPS_LED_PIN, 5);
      stat = sendATcommand("AT+CGPSSTATUS", "2D Fix", 2000);
      delay(60000);
    }
    if (stat) {
      digitalWrite(GPS_LED_PIN, HIGH); // if got gps satellites - gps led is ON
    }
  } else {
    // critical error here!
  }
}

void gsm_up() {
 uint8_t reg=0;
 uint8_t pin=0;

  pin = sendATcommand("AT+CPIN?", "READY", 2000);
  reg = sendATcommand("AT+CREG?", "1,1", 2000);

  if (!pin) {
    pin = sendATcommand("AT+CPIN=1234", "READY", 2000); //TODO: pin code - ?
  }
  while (!reg) { 
    ledFlash(200, GSM_LED_PIN, 5);
    reg = sendATcommand("AT+CREG?", "1,1", 2000);
  }
  if (pin && reg) {
    digitalWrite(GSM_LED_PIN, HIGH); // if registered to the network - gsm led is ON
  }
}

void gprs_up() {

}

void setup() {
  pinMode(SIM_908_PWR_PIN, OUTPUT);
  // pinMode(BUTTON, INPUT);
  pinMode(GPS_LED_PIN, OUTPUT);
  pinMode(GSM_LED_PIN, OUTPUT);
  pinMode(PWR_LED_PIN, OUTPUT);

  Serial.begin(115200);
  // mySerial.begin(115200);

  check_stat();
  delay(1000);
  gsm_up();
}

void loop() {

  //TODO: use check_stat and gsm_up here, to report if smthng is down during normal work of module

  // buttonState = digitalRead(BUTTON);
  // if (buttonState == HIGH)
  // {
  //   // timer++;
  //   // delay(10);
  //   analogWrite(SIM_908_PWR_PIN, SIM_908_PWR_RATE);
  // } else {
  //   // timer = 0;
  //   // delay(10);
  //   digitalWrite(SIM_908_PWR_PIN, 0);
  // }

  // delay(2000);
  // if (DEBUG == true && mySerial.available()) {
  //    Serial.write(mySerial.read());
  // }
  // if (Serial.available()) {
  //   mySerial.write(Serial.read());
  // }
}
