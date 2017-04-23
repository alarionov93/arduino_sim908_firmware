/**
SIM 908 MODULE ARDUINO FIRMWARE
//make short description

TODO list:
1. Write setup for all features of module
2. Error handling, set critical error reporting
3. Write main loop to get gps and send it

*/

#include <LowPower.h>
#include <SoftwareSerial.h>
#include <avr/io.h>
#include <avr/interrupt.h>

// #define SIM_908_PWR_RATE 170
#define SS_RX           2
#define SS_TX           3
#define SIG_PIN         9
#define BUTTON          11
#define SWITCH_MODE_PIN 10
#define ERROR_PIN       12
#define OK_PIN          13
#define DEFAULT_WATCH_MODE_DELAY 900000 //15 mins
#define DEFAULT_TRACK_MODE_DELAY 45000 
#define TRACK_MODE      1
#define WATCH_MODE      0
#define DEVICE_ID       "13450633605839585280"
// #ifndef ISR
// ISR(USART_RX_vect) {
//   ledFlash(20, OK_PIN, 1);
//   reti();
// }
// #endif

int button_state = 0;
int timer = 0;
bool DEBUG = true;
uint8_t answer=0;
char aux_str[60];
//uint8_t MODE_DELAY = 80000;
int mode = TRACK_MODE;
int timer_interrupt_count = 0;
int sleep_counter = 0;
int is_sleep_in_watch_mode = 0;

char pin[4]="";
char apn[21]="internet.beeline.ru";
char user_name[10]="beeline";
char password[10]="beeline";
char url[30]="http://a-larionov.ru:5000/gps";
char error_url[30]="http://a-larionov.ru:5000/err";
char longitude[30]="";
char latitude[30]="";
char altitude[10]="";
char speed[11]="";
char frame[200]="";
char date[17]="";
char satellites[4]="";
char course[11]="";
char chgMode[2]="";
char percent[4]="";
char voltage[5]="";
//char bat_chg_info[100]="";

char SMS[5] = "";
int sms_idx = 0;
char sms_phone_from[12] = "";

SoftwareSerial SoftSerial(SS_RX, SS_TX); // RX, TX

int8_t sendATcommand(const char* ATcommand, const char* expected_answer, unsigned int timeout){

    uint8_t x=0,  answer=0;
    char response[100];
    unsigned long previous;

    memset(response, '\0', 100);    // Initialize empty string

    delay(100);

    while (Serial.available() > 0) // 1
    {
      Serial.read();    // Clean the input buffer
    }

    Serial.print(ATcommand);    // Send the AT command 
    Serial.print("\r\n");

    x = 0;
    previous = millis();

    // this loop waits for the answer
    // 2
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

void report_err(int try_num, char msg[40]) { // USE ONLY AFTER GETTING GPRS BEARER !!
 bool success = false;
 answer = sendATcommand("AT+HTTPINIT", "OK", 5000);
     if (answer == 1)
     {
     // Sets CID parameter
       answer = sendATcommand("AT+HTTPPARA=\"CID\",1", "OK", 2000);
       if (answer == 1)
       {
         // Sets url 
         sprintf(aux_str, "AT+HTTPPARA=\"URL\",\"%s", error_url);
         Serial.print(aux_str);
         // answer = sendATcommand(aux_str, "OK", 2000);
         sprintf(frame, "?error=%s&device=%s", msg, DEVICE_ID);
         Serial.print(frame);
         answer = sendATcommand("\"", "OK", 5000);
         if (answer == 1)
         {
           answer = sendATcommand("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"", "OK", 2000);
           if(answer == 1) {
             // Starts GET action
             answer = sendATcommand("AT+HTTPACTION=0", "+HTTPACTION:0,200", 10000);
             if (answer == 1) {
               success = true;
             } else {
               SoftSerial.println("ERR GET URL\n"); // TODO: SHOW ERR
               //here if url is not available, set old url a-larionov.ru:2222/data_parser.php
               ledFlash(1000, ERROR_PIN, 10); //this is set to understand that something went wrong on data sending - the last step!
             }
           } else { //TODO: ALL HERE SHOW ERR
             SoftSerial.println("ERR CON TYPE\n");
             ledFlash(1000, ERROR_PIN, 10);
           }
         } else {
           SoftSerial.println("ERR SET URL\n");
           ledFlash(1000, ERROR_PIN, 10);
         }
       } else {
         SoftSerial.println("ERR SET CID\n");
         ledFlash(1000, ERROR_PIN, 10);
       }    
     } else {
       SoftSerial.println("ERR INIT\n");
       ledFlash(1000, ERROR_PIN, 10);
     }
   sendATcommand("AT+HTTPTERM", "OK", 5000);
   if (success == true)
   {
     ledFlash(200, ERROR_PIN, 7); // TODO: SHOW OK
   }
}

void ledFlash(int del, int pin, int count) { // 3
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

void sleep_module() {
  uint8_t answer=0;

  answer = sendATcommand("AT+CFUN=0", "OK", 5000);
  if (answer == 0) {
    while(answer == 0){     // Send AT every two seconds and wait for the answer
     answer = sendATcommand("AT+CFUN=0", "OK", 5000);
    }
  }
}

void wake_up() {
  uint8_t answer=0;

  answer = sendATcommand("AT+CFUN=1", "OK", 5000);
  if (answer == 0) {
    while(answer == 0){     // Send AT every two seconds and wait for the answer
     answer = sendATcommand("AT+CFUN=1", "OK", 5000);
    }
  }
  // TODO: temp comment and then uncomment this lines !!
  delay(100);
  // gprs_up();
  delay(100);
  // gps_up();
  SoftSerial.print("WAKE UP DONE.\n");
  ledFlash(200, OK_PIN, 3);
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
      ledFlash(200, ERROR_PIN, 5);
      answer = sendATcommand("AT", "OK", 2000);
    }
  }
  if (answer == 1) {
    ledFlash(500, ERROR_PIN, 5);
  }
}

void gps_up() {
 uint8_t pwr=0;
 uint8_t rst=0;
 uint8_t stat=0;
 uint8_t gps_pwr_stat = 0;
 int x=0;

  gps_pwr_stat = sendATcommand("AT+CGPSPWR?", "1", 2000); 

  if(gps_pwr_stat == 0) {
    ledFlash(200, OK_PIN, 5);
    pwr = sendATcommand("AT+CGPSPWR=1", "OK", 2000);
    rst = sendATcommand("AT+CGPSRST=1", "OK", 2000);

    while (pwr == 0 && rst == 0) { 
      pwr = sendATcommand("AT+CGPSPWR=1", "OK", 2000);
      rst = sendATcommand("AT+CGPSRST=1", "OK", 2000); //TODO: ERR???
      ledFlash(200, OK_PIN, 5); // TODO: set flashing for 1 second as a signal that smthng went wrong! //report critical error here!
    }
  } else {
    pwr = 1;
    rst = 1;
  }
  if(pwr == 1 && rst == 1) {
    stat = sendATcommand("AT+CGPSSTATUS?", "D Fix", 2000);
    while(stat == 0) {
      ledFlash(200, OK_PIN, 5);
      delay(20000); //TODO: move this before sending command
      stat = sendATcommand("AT+CGPSSTATUS?", "D Fix", 2000);
      x++;
      if (x >= 4)
      {
       report_err(x, "Can_not_fix_location"); // IMPORTANT !! NOT use characters that may cause URL breaks!!
      }
    }
    if (stat) {
      digitalWrite(OK_PIN, HIGH); // if got gps satellites - gps led is ON // 4
      delay(2000);
      digitalWrite(OK_PIN, LOW);
    }
  } else {
    // critical error here!
    digitalWrite(ERROR_PIN, HIGH);
    delay(10000);
  }
}

void gsm_up() {
 uint8_t reg=0;
 uint8_t pin=0;
 uint8_t x=0;

  pin = sendATcommand("AT+CPIN?", "READY", 2000);
  reg = sendATcommand("AT+CREG?", "1", 1000);

  if (!pin) {
    pin = sendATcommand("AT+CPIN=1777", "READY", 2000); //TODO: pin code - ?
  }
  while (!reg) { 
    // x++;
    reg = sendATcommand("AT+CREG?", "1", 1000);
    // if (x == 10)
    // {
    //   break;
    // }
  }
  if (pin && reg) {
    SoftSerial.write("PIN OK, GSM OK.\n");
    digitalWrite(OK_PIN, HIGH); // TODO: SHOW OK
    delay(2000);
    digitalWrite(OK_PIN, LOW);
  }
  if (pin && reg == 0) {
    SoftSerial.write("PIN OK, GSM ERR.\n");
    ledFlash(100, ERROR_PIN, 5);
  }
}

void gprs_up() {
  uint8_t gprs = 0;
  uint8_t stat_gprs = 0;
  char tmp[10] = "";
  int tr = 1; // trying counter
    // sendATcommand("AT+CSTT=\"internet.beeline.ru\",\"beeline\",\"beeline\"", "OK", 2000);
    // stat_gprs = sendATcommand("AT+SAPBR=4,1", "internet.beeline.ru", 2000);
    // if(stat_gprs == 1) {
    //   gprs = sendATcommand("AT+SAPBR=0,1", "OK", 2000);
    //   ledFlash(500, ERROR_PIN, 3);
    // }
    ledFlash(200, OK_PIN, 5);
    SoftSerial.write("SET APN.\n");
    sendATcommand("AT+CSTT=\"internet.beeline.ru\",\"beeline\",\"beeline\"", "OK", 2000); //for sim with 963016...0!

    // sendATcommand("AT+SAPBR=0,1", "OK", 2000); //always turn gprs off - to avoid if AT+SAPBR=1,1 returning ERROR
    // not needed for the first time after turn module on!!!

    // sendATcommand("AT+SAPBR=3,1,\"APN\",\"internet.beeline.ru\"", "OK", 2000);
    
    // sendATcommand("AT+SAPBR=3,1,\"USER\",\"beeline\"", "OK", 2000);
    
    // sendATcommand("AT+SAPBR=3,1,\"PWD\",\"beeline\"", "OK", 2000);

    gprs = sendATcommand("AT+SAPBR=1,1", "OK", 45000); //30 seconds is the minimal value!
    while (gprs == 0)
    {
      tr++;
      ledFlash(200, OK_PIN, 5); //TODO: SHOW 
      SoftSerial.println(sprintf(tmp, "SET APN:%d.\n", tr));
      sendATcommand("AT+CSTT=\"internet.beeline.ru\",\"beeline\",\"beeline\"", "OK", 2000);
      sendATcommand("AT+SAPBR=0,1", "OK", 2000); //always turn gprs off - to avoid if AT+SAPBR=1,1 returning ERROR

      // sendATcommand("AT+SAPBR=3,1,\"APN\",\"internet.beeline.ru\"", "OK", 2000);
    
      // sendATcommand("AT+SAPBR=3,1,\"USER\",\"beeline\"", "OK", 2000);
    
      // sendATcommand("AT+SAPBR=3,1,\"PWD\",\"beeline\"", "OK", 2000);

      gprs = sendATcommand("AT+SAPBR=1,1", "OK", 40000);
    }
    if (gprs == 1) {
      SoftSerial.write("GPRS UP.\n");
      digitalWrite(OK_PIN, HIGH); // TODO: SHOW OK
      delay(2000);
      digitalWrite(OK_PIN, LOW);
    }
}

int8_t getCoordinates() { //TODO add reporting error if location is missing !

   int8_t counter, answer;
   long previous;

   // First get the NMEA string
   // Clean the input buffer
   while( Serial.available() > 0) Serial.read(); 
   // request Basic string
   sendATcommand("AT+CGPSINF=0", "AT+CGPSINF=0\r\n\r\n", 2000);

   counter = 0;
   answer = 0;
   memset(frame, '\0', 100);    // Initialize the string
   previous = millis();
   // this loop waits for the NMEA string
   do {
       if(Serial.available() != 0){    
           frame[counter] = Serial.read();
           counter++;
           // check if the desired answer is in the response of the module
           if (strstr(frame, "OK") != NULL)    
           {
               answer = 1;
           }
       }
       // Waits for the asnwer with time out
   }
   while((answer == 0) && ((millis() - previous) < 2000));  

   frame[counter-3] = '\0'; 
   
   // Parses the string 
   strtok(frame, ",");
   strcpy(longitude,strtok(NULL, ",")); // Gets longitude
   strcpy(latitude,strtok(NULL, ",")); // Gets latitude
   strcpy(altitude,strtok(NULL, ".")); // Gets altitude 
   strtok(NULL, ",");    
   strcpy(date,strtok(NULL, ".")); // Gets date
   strtok(NULL, ",");
   strtok(NULL, ",");  
   strcpy(satellites,strtok(NULL, ",")); // Gets satellites
   strcpy(speed,strtok(NULL, ",")); // Gets speed over ground. Unit is knots.
   strcpy(course,strtok(NULL, "\r")); // Gets course
   
   return answer;
}

void alarm() {

}

//void getBatChgLvl() {
//  
//  int8_t counter, answer;
//  long previous;
//  char * pch;
//  // Clean the input buffer
//  while( Serial.available() > 0) Serial.read(); 
//  // request Basic string
////  sendATcommand("AT+CBC", "OK", 80);
//  Serial.println("AT+CBC");
//
//  counter = 0;
//  answer = 0;
//  memset(bat_chg_info, '\0', 100);    // Initialize the string
//  previous = millis();
//  do {
//    if(Serial.available() != 0){    
//      SoftSerial.write(Serial.read());
//    }
//  } while ((millis() - previous) < 500);
//
//  bat_chg_info[counter-3] = '\0'; 
////  int x = 0;
////  while(x < 60) {
////    if (bat_chg_info[x] != '\0') {
////      SoftSerial.write(bat_chg_info[x]);
////    }
////  }
//  // pch = strtok (str," ,.-");
//  // while (pch != NULL)
//  // {
//  //   printf ("%s\n",pch);
//  //   pch = strtok (NULL, " ,.-");
//  // }
//  
//  // Parses the string 
//  strtok(bat_chg_info, ",");
//  strcpy(chgMode,strtok(NULL, ",")); // Gets battery charging mode (0-NOT charging, 1-charging, 2-charged)
//  strcpy(percent,strtok(NULL, ",")); // Gets battery percentage
//  strcpy(voltage,strtok(NULL, "\r")); // Gets battery voltage
//  
//  return answer;
//}

void sendBatChgLvl() {
  // // to cause interrupt on reciever device, because interrupts by incoming bytes in UART on it are disabled =(
  // digitalWrite(SIG_PIN, HIGH);
  // delay(200);
  // digitalWrite(SIG_PIN, LOW);
  
  // SoftSerial.print(batLvlStr);
  SoftSerial.print(chgMode);
  SoftSerial.print(percent);
  // SoftSerial.print(voltage);
}

// void sendCoordsInSMS() { //with test data now
//   bool success = false;
//   if (atof(longitude) > 0 && atof(latitude) > 0) {
//     char longitude[30]="54.182";
//     char latitude[30]="53.218";
//     char altitude[10]="34.02";
//     char speed[11]="1.4";
//     // answer = sendATcommand(aux_str, "OK", 2000);
//     sprintf(frame, "%s;%s;%s;%s", latitude, longitude, altitude, speed);
//     answer = sendSMS(frame);
//     if (answer == 1)
//     {
//       ledFlash(100, OK_PIN, 2); // TODO: SHOW OK
//     } else {
//       ledFlash(100, ERROR_PIN, 2); // TODO: SHOW ERR
//     }
//   }
// }

void sendCoordinates() {
  bool success = false;
  if (atof(longitude) > 0 && atof(latitude) > 0) {
    answer = sendATcommand("AT+HTTPINIT", "OK", 5000);
      if (answer == 1)
      {
      // Sets CID parameter
        answer = sendATcommand("AT+HTTPPARA=\"CID\",1", "OK", 2000);
        if (answer == 1)
        {
          // Sets url 
          sprintf(aux_str, "AT+HTTPPARA=\"URL\",\"%s", url);
          Serial.print(aux_str);
          // answer = sendATcommand(aux_str, "OK", 2000);
          sprintf(frame, "?lat=%s&lon=%s&alt=%s&speed=%s&device=%s", latitude, longitude, altitude, speed, DEVICE_ID);
          Serial.print(frame);
          answer = sendATcommand("\"", "OK", 5000);
          if (answer == 1)
          {
            answer = sendATcommand("AT+HTTPPARA=\"CONTENT\",\"application/x-www-form-urlencoded\"", "OK", 2000);
            if(answer == 1) {
              // Starts GET action
              answer = sendATcommand("AT+HTTPACTION=0", "+HTTPACTION:0,200", 10000);
              if (answer == 1) {
                success = true;
              } else {
                SoftSerial.println("ERR GET URL\n"); // TODO: SHOW ERR
                //here if url is not available, set old url a-larionov.ru:2222/data_parser.php
                ledFlash(1000, ERROR_PIN, 10); //this is set to understand that something went wrong on data sending - the last step!
              }
            } else { //TODO: ALL HERE SHOW ERR
             SoftSerial.println("ERR CON TYPE\n");
             ledFlash(1000, ERROR_PIN, 10);
            }
          } else {
           SoftSerial.println("ERR SET URL\n");
           ledFlash(1000, ERROR_PIN, 10);
          }
        } else {
         SoftSerial.println("ERR SET CID\n");
         ledFlash(1000, ERROR_PIN, 10);
        }    
     } else {
       SoftSerial.println("ERR INIT\n");
       ledFlash(1000, ERROR_PIN, 10);
     }
    sendATcommand("AT+HTTPTERM", "OK", 5000);
    if (success == true)
    {
      ledFlash(100, OK_PIN, 2); // TODO: SHOW OK
    }
  } else {
    // report missing location HERE
    report_err(1, "Location_is_lost");
    ledFlash(100, OK_PIN, 3);// TODO: SHOW near OK
  }
}

// int sendSMS(char sms_text[]) {
//     int8_t answer;
//     char aux_string[30];
//     char phone_number[]="+79655766572";   // ********* is the number to call
//     // while( (sendATcommand("AT+CREG?", "+CREG: 0,1", 500) || 
//     //         sendATcommand("AT+CREG?", "+CREG: 0,5", 500)) == 0 );

//     sendATcommand("AT+CMGF=1", "OK", 1000);    // sets the SMS mode to text
    
//     sprintf(aux_string,"AT+CMGS=\"%s\"", phone_number);
//     answer = sendATcommand(aux_string, ">", 2000);    // send the SMS number
//     if (answer == 1)
//     {
//         Serial.println(sms_text);
//         Serial.write(0x1A);
//         answer = sendATcommand("", "OK", 20000);
//         if (answer == 1)
//         {
//             SoftSerial.print("Sent ");
//             ledFlash(50, OK_PIN, 3);  
//             return 1;  
//         }
//         else
//         {
//             SoftSerial.print("Error ");
//             ledFlash(50, ERROR_PIN, 3);
//             return 0;
//         }
//     }
//     else
//     {
//         SoftSerial.print("Error ");
//         SoftSerial.println(answer, DEC);
//         ledFlash(50, ERROR_PIN, 3);
//         return 0;
//     }

// }

void getLastSMSIndex() {
  char buff[90]="";
  int8_t answer = 0;
  uint8_t x = 0;
  long previous;
  sendATcommand("AT+CMGF=1", "OK", 800);    // sets the SMS mode to text
  sendATcommand("AT+CPMS=\"SM\",\"SM\",\"SM\"", "OK", 800); // choose sim card memory
  

  memset(buff, '\0', 70);
    
  int i = 0;
  
  Serial.println("AT+CMGL=\"REC UNREAD\", 0"); // choose unread sms

  previous = millis();
  answer = 0;
  // this loop waits for the NMEA string
  do {
      if(Serial.available() != 0){    
          buff[i] = Serial.read();
          i++;
          // check if the desired answer is in the response of the module
          if (strstr(buff, "OK") != NULL)    
          {
              answer = 1;
          }
      }
      // Waits for the asnwer
  }
  while(answer == 0);

  if (strstr(buff, "+CMGL") != NULL) 
  {    
    // char date[10] = "";
    // char time[15] = "";
    // char msg[15] = "";
    sscanf(buff, "%*[^:]: %d, %*[^,], %[^,], %*[^,], %*[^,], %*s\"\n%*s", &sms_idx, sms_phone_from);

    SoftSerial.println();
    SoftSerial.print("SMS IDX: ");
    SoftSerial.print(sms_idx);
    SoftSerial.print("SMS FROM: ");
    SoftSerial.print(sms_phone_from);
    SoftSerial.println();
  } 
  else 
  {
    SoftSerial.println("ERROR GETTING LAST SMS INDEX OR NO UNREAD MESSAGES.");
  }

  // if (answer == 1) 
  // {
  //     answer = 0;
  //     while(Serial.available() == 0);
  //     // this loop reads the data of the SMS
  //     do{
  //         // if there are data in the UART input buffer, reads it and checks for the asnwer
  //         if(Serial.available() > 0){    
  //             buffer[x] = Serial.read();
  //             x++;
  //             // check if the desired answer (OK) is in the response of the module
              
  //         }
  //     }while(answer == 0);    // Waits for the asnwer with time out
      
  //     buffer[x] = '\0';
      
  //     SoftSerial.print(buffer);
  // } else {
  //     SoftSerial.println("ERROR OR NO NEW MESSAGES");
  // }
}

void readSMS(int index) {
  memset(SMS, '\0', 4);
  int8_t answer;
  uint8_t x = 0;
  char cmd[15] = "";
  SoftSerial.print("IDX RECV:");
  SoftSerial.print(index);
  
  if (index > 0 && strstr(sms_phone_from, "9655766572") != NULL)
  {
    SoftSerial.print("COND IS OK, READ MSG.\n");
    sendATcommand("AT+CMGF=1", "OK", 1000);    // sets the SMS mode to text
    sendATcommand("AT+CPMS=\"SM\",\"SM\",\"SM\"", "OK", 1000);    // selects the memory
    //TODO: read the LAST (!!!) SMS message, NOT FIRST !!!
    sprintf(cmd, "AT+CMGR=%d\r\n", index);
    SoftSerial.print(cmd);
    SoftSerial.print("BEFORE READ MSG.\n");
    answer = sendATcommand(cmd, "+CMGR:", 2000);    // reads the first SMS
    if (answer == 1)
    {
        SoftSerial.print("READ INC MSG.\n");
        ledFlash(100, OK_PIN, 6);
        answer = 0;
        while(Serial.available() == 0);
        // this loop reads the data of the SMS
        do{
            // if there are data in the UART input buffer, reads it and checks for the asnwer
            if(Serial.available() > 0){    
                SMS[x] = Serial.read();
                x++;
                // check if the desired answer (OK) is in the response of the module
                if (strstr(SMS, "OK") != NULL)    
                {
                    answer = 1;
                    ledFlash(100, OK_PIN, 15);
                }
            }
        } while(answer == 0);    // Waits for the asnwer with time out
        
        SMS[x] = '\0';
        
        SoftSerial.print(SMS);

        memset(cmd, '\0', 14);
        sprintf(cmd, "AT+CMGD=%d", index); // delete read message
        answer = 0;
        answer = sendATcommand(cmd, "OK", 1000);
        if (answer == 1)
        {
          SoftSerial.print("RM MSG OK");
        } 
        else
        {
          SoftSerial.print("ERROR RM MSG");
        }
    }
    else
    {
        SoftSerial.print("ERR READ MSG.");
        SoftSerial.println(answer, DEC);
        ledFlash(100, ERROR_PIN, 10);
    }
  }
  else 
  {
    if (index > 0)
    {
        memset(cmd, '\0', 10);
        sprintf(cmd, "AT+CMGD=%d", index); // delete read message
        answer = 0;
        answer = sendATcommand(cmd, "OK", 1000);
        if (answer == 1)
        {
          SoftSerial.print("RM NOT OWNER MSG OK");
        } 
        else
        {
          SoftSerial.print("ERROR RM NOT OWNER MSG");
        }
    }
    SoftSerial.println("NO SMS IDX.");
  }
}

void setup() {
  
  //configure pins
  pinMode(OK_PIN, OUTPUT);
  pinMode(ERROR_PIN, OUTPUT);
  pinMode(SIG_PIN, OUTPUT);
  pinMode(SWITCH_MODE_PIN, INPUT);
  mode = TRACK_MODE;

  // initialize timer1 
  // interrupts was not working BECAUSE OF THIS LINE?
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;

  OCR1A = 31250;            // compare match register 16MHz/256/2Hz
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= (1 << CS12);
  TCCR1B |= (1 << CS10);    // 1024 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
  interrupts();             // enable all interrupts

  //configure serials
  Serial.begin(19200);
  SoftSerial.begin(19200);

  //configure module

  delay(2000);
  SoftSerial.println("Conf GSM.");
  gsm_up();
  // ledFlash(50, ERROR_PIN, 3);

  delay(100);
  SoftSerial.println("Conf GPRS.");
  gprs_up();
  // test
  is_sleep_in_watch_mode = 1;
  // ledFlash(50, ERROR_PIN, 3);

  // TEST LINES BELOW
  // SoftSerial.println("Configure module: Test SMS read SMS here.");
  // if (strstr(SMS, "GL") != NULL) // found incoming SMS with coordinates request
  // {
  //     SoftSerial.println("Configure module: Test SMS found target sequence.");
  //     digitalWrite(OK_PIN, HIGH); 
  //     // sendCoordsInSMS();
  //     // ledFlash(50, OK_PIN, 10); //before sending coordinates 10 flashes!

  //     // getCoordinates();
  //     // sendCoordinates();
  // } else {
  //     SoftSerial.println("Configure module: Test SMS target sequence NOT found.");
  //     digitalWrite(ERROR_PIN, HIGH); 
  // }
  
  // getBatChgLvl();
  // delay(200);
  // sendBatChgLvl();
  
  delay(100);
  // gps_up();
  // ledFlash(50, ERROR_PIN, 3);
}

void loop() {
  SoftSerial.println("SMS GETTING INDEX.");
  getLastSMSIndex();
  SoftSerial.println("SMS READ NEW.");
  readSMS(sms_idx);
  delay(100);
  if (strstr(SMS, "GL") != NULL) // found incoming SMS with coordinates request
  {
      // sendCoordsInSMS();
      // ledFlash(50, OK_PIN, 10); //before sending coordinates 10 flashes!

    delay(5000); // as getCoordinates();
    delay(10000); // as sendCoordinates();
    SoftSerial.print("SENT LOCATION BY SMS.");
  } else {
      // send coords every 4th time
      if (sleep_counter > 4)
      {
        // ledFlash(50, OK_PIN, 10); //before sending coordinates 10 flashes!

        delay(5000); // as getCoordinates();
        delay(10000); // as sendCoordinates();
        SoftSerial.print("SENT LOCATION REGULAR.");
        sleep_counter = 0;
      }
      sleep_counter++;
  }
  // test
  SoftSerial.print("AFTER SEND LOCATION.\n");
  button_state = digitalRead(SWITCH_MODE_PIN); // TODO: read "WM" from sms to activate watch mode and "TM" for track mode
  
  // if(button_state == HIGH) {
  //   mode = TRACK_MODE;
  // } else {
  //   mode = WATCH_MODE;
  // }
  if (mode == TRACK_MODE)
  {
    // delay(DEFAULT_TRACK_MODE_DELAY); 
    // TODO: test working of powerDown after solving SMS, GPRS and read messages problems !!!
    SoftSerial.print("TRACK MODE ACTIVE.\n");
    for (int i = 0; i < 5; i++) //sleep for 40 seconds
    {
      // LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      delay(8000);
    }
  }
  else 
  {
    SoftSerial.print("WATCH MODE ACTIVE.\n");

    sleep_module();
    // delay(DEFAULT_WATCH_MODE_DELAY);
    is_sleep_in_watch_mode = 1;
    for (int i = 0; i < 100; i++) //sleep for 800 seconds
    {
      // LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
      delay(8000);
    }
    is_sleep_in_watch_mode = 0;
    wake_up();
  }


  //TODO: coordinates difference should depend on calculation this value!!

  //TODO: use check_stat and gsm_up here, to report if smthng is down during normal work of module

  // delay(2000);
  // if (DEBUG == true && Serial.available()) {
  //    Serial.write(Serial.read());
  // }
  // if (Serial.available()) {
  //   Serial.write(Serial.read());
  // }
}

ISR(TIMER1_COMPA_vect) {
  noInterrupts();
  // TODO: after, uncomment this condition
  SoftSerial.print("IN ISR.\n");
  if (is_sleep_in_watch_mode == 1)
  {
    ledFlash(30, OK_PIN, 4);
  }
//   if (timer_interrupt_count == 0 || (timer_interrupt_count % 50) == 0) {
// //    getBatChgLvl();
//   }
  if ((timer_interrupt_count % 2) == 0)
  {
    digitalWrite(SIG_PIN, HIGH);
//    Serial.println("Bat chg here.");
    Serial.println("AT+CBC");
    do {  
      SoftSerial.write(Serial.read());
    } while (Serial.available() != 0);
    sendBatChgLvl();
//    SoftSerial.println("Bat chg here!");
  } else {
    digitalWrite(SIG_PIN, LOW);
  }
  // if ((timer_interrupt_count % 4) == 0)
  // {
  //   /* check sms here */
  // }

  timer_interrupt_count++;
  if (timer_interrupt_count > 1000) {
    timer_interrupt_count = 0;
  }
  interrupts();
}

