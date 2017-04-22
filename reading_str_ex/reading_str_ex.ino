//char answ[300] = "+CMGL: 1,\"REC UNREAD\",\"+79655766572\",\"\",\"17/04/22,00:38:19+20\"\r\nGL\r\nOK";
int8_t answer = 0;

int i = 0;

char * pch;
uint8_t sms_idx_str[3]="";
int sms_idx = 0;
uint8_t sms_from[15]="";
// uint8_t sms_mode[10]="";

void setup() {
	Serial.begin(19200);
}
void loop() {
	char answ[300] = "+CMGL: 1,\"REC UNREAD\",\"+79655766572\",\"\",\"17/04/22,00:38:19+20\"\r\nGL\r\nOK";
	if (strstr(answ, "+CMGL") != NULL) 
	{
		char data[15] = "";

		/* get the first token */
		pch = strtok(answ, ":");

		/* walk through other tokens */
		int x = 0;
		while( pch != NULL ) 
		{
		  memset(data, '\0', 20);	
		  sprintf(data, " %s\n", pch);
		  // Serial.println(data);
		  switch (x) 
		  {
		  	case 1:
		  		sms_idx = atoi(data);
		  		break;
		  	case 3:
		  		sms_from = data;
		  		break;
		  	case 6:
		  		sms_text = data;
		  		break;
		  }
		  // Serial.println(pch);
		  pch = strtok(NULL, ",");
		  x++;
		}
		Serial.println(sms_idx);
		Serial.println(sms_from);
		Serial.println(sms_text);

		// pch = strtok(answ, ":");
  //   	Serial.print(pch);
		// strcpy(sms_idx_str, strtok(NULL, ","));
		// strcpy(sms_mode, strtok(NULL, ","));
		// strcpy(sms_from_str, strtok(NULL, ","));
		// sms_idx = atoi(sms_idx_str);
		// Serial.println(answ);
		// Serial.print("SMS IDX: ");
		// Serial.print(sms_idx);
	 //    Serial.print(", ");
	 //    Serial.print(sms_idx_str);
		// Serial.print("SMS FROM: ");
		// Serial.print(sms_from_str);
  //   	Serial.print(sms_mode);
	} 
	else 
	{
		Serial.println("ERROR GETTING LAST SMS INDEX OR NO UNREAD MESSAGES.");
	}
}
