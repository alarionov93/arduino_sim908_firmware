char answ[300] = "+CMGL: 1,\"REC UNREAD\",\"+79655766572\",\"\",\"17/04/22,00:38:19+20\"\r\nGL\r\nOK";
int8_t answer = 0;

int i = 0;

char * pch;
char sms_idx_str[5]="";
int sms_idx = 0;
char sms_from_str[12]="";
char sms_mode[10]="";

void setup() {
	
}
void loop() {

	if (strstr(answ, "+CMGL") != NULL) 
	{
	  pch = strtok(answ, ":");
	  strcpy(sms_idx_str, strtok(NULL, ","));
	  strcpy(sms_mode, strtok(NULL, ","));
	  strcpy(sms_from_str, strtok(NULL, ","));
	  sms_idx = atoi(sms_idx_str);
	  Serial.println(answ);
	  Serial.print("SMS IDX: ");
	  Serial.print(sms_idx);
	  Serial.print("SMS FROM: ");
	  Serial.print(sms_from_str);
	} 
	else 
	{
	  Serial.println("ERROR GETTING LAST SMS INDEX OR NO UNREAD MESSAGES.");
	}
}