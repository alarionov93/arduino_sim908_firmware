#include <SoftwareSerial.h>

#define SIM_908_PWR_PIN 9
#define SIM_908_PWR_RATE 170
#define BUTTON 12
#define LED 4
int buttonState = 0;
int timer = 0;
bool DEBUG = false;

SoftwareSerial mySerial(10, 11); // RX, TX


int8_t sendATcommand(const char* ATcommand, const char* expected_answer, unsigned int timeout){

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);    // Initialize empty string

    delay(100);

    while (mySerial.available() > 0)
    {
      mySerial.read();    // Clean the input buffer
    }

    mySerial.print(ATcommand);    // Send the AT command 
    mySerial.print("\r\n");

    x = 0;
    previous = millis();

    // this loop waits for the answer
    do {
        if(mySerial.available() != 0)
        {
            // if there are data in the UART input buffer, reads it and checks for the asnwer
            response[x] = mySerial.read();
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
    if (answer == 0)
    {
      while(answer == 0){     // Send AT every two seconds and wait for the answer
          answer = sendATcommand("AT", "OK", 2000);
      }
      digitalWrite(LED, HIGH);
      delay(2000);
      digitalWrite(LED, LOW);
    } else {
      digitalWrite(LED, HIGH);
      delay(200);
      digitalWrite(LED, LOW);
    }
}

void setup() {
  pinMode(SIM_908_PWR_PIN, OUTPUT);
  pinMode(BUTTON, INPUT);
  Serial.begin(115200);
  mySerial.begin(115200);

  check_stat();
  // delay(20000);
  // power_down();
}

void loop() {
  buttonState = digitalRead(BUTTON);
  if (buttonState == HIGH)
  {
    timer++;
    delay(10);
    analogWrite(SIM_908_PWR_PIN, SIM_908_PWR_RATE);
  } else {
    timer = 0;
    delay(10);
    digitalWrite(SIM_908_PWR_PIN, 0);
  }
  if (DEBUG == true && mySerial.available()) {
     Serial.write(mySerial.read());
  }
  if (Serial.available()) {
    mySerial.write(Serial.read());
  }
}
