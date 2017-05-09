#include <ESP8266WiFi.h>
#include <SoftwareSerial.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

//SQL read
#include <WiFiClient.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

#include <WiFiUdp.h>
const char* host = "192.168.143.1"; //laptop NTP server

//start NTPClientMod

//mod
#include <RTClib.h>                       // RTC-Library
RTC_Millis RTC;                           // RTC (soft)
DateTime now;                             // current time
int ch,cm,cs,os,cdy,cmo,cyr,cdw;          // current time & date variables
int nh,nm,ns,ndy,nmo,nyr,ndw;             // NTP-based time & date variables

//end mod

unsigned int localPort = 2390;      // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;


//end NTP


//IPAddress server_addr(192,168,143,200); // Piplay
IPAddress server_addr(192,168,42,146); // IP of the MySQL server here
char user[] = "nodemcu1"; // MySQL user login username
char spassword[] = "secret"; // MySQL user login password

// Sample query
char QUERY_INSERT[] = "INSERT into kpi_mech.counter (dateTime, cellNo, status) values ((now()), %d, 0);";
char query[256];

WiFiClient client;
MySQL_Connection conn((Client *)&client);

//LCD
LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x27 for a 16 chars and 2 line display

const char* ssid = "jomarAP-SP";
const char* wpassword = "maquinay1";

//const char* ssid = "outsourcing1.25s";
//const char* wpassword = "dbafe12345!!!";

//buzzer
const int buzzer = D8;

//button
const int AttFunctionButton = D5;
const int startButton = D6;
const int cancelButton = D7;
int buttonState1 = 1;
int buttonState2 = 1;


//potentiometer selector
int potPin = A0;
int potVal = 0;       // variable to store the potValue coming from the sensor

int looper = 1;		//time loop
int looper2 = 1;	//select cell loop
int looper3 = 1;	//loop for sending SQL
int cellNo = 0; 	//cell number
int counter = 60; 	//counter before checking time

void setup() {
	
	Serial.begin(9600);

	Serial.println("Starting UDP");
	udp.begin(localPort);
	Serial.print("Local port: ");
	Serial.println(udp.localPort());
 
  // We start by connecting to a WiFi network
  Serial.println();
 /* 
  ClearLCD();
  lcd.setCursor(0,0);
  lcd.print("Connecting to ");
  lcd.setCursor(0,1);
  lcd.print("WIFI");
  */
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, wpassword);

  int ResetCounter = 0;
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.print(ResetCounter);
	  Serial.println(WiFi.status());
    ResetCounter++;
    if (ResetCounter >= 30) {
		Serial.print("\n");
		Serial.print("ESP8266 reset!");
		ESP.restart();
      }
}

  //LCD init
  lcd.init();                      // initialize the lcd 
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("QA Counter");
  lcd.setCursor(0,1);
  lcd.print("Kiosk by JM");
  delay(3000);
  buzzerFunction(3);

	pinMode(startButton, INPUT_PULLUP);
	pinMode(cancelButton, INPUT_PULLUP);
	pinMode(AttFunctionButton, INPUT);
		
	attachInterrupt(startButton, startButtonChange, CHANGE);
	attachInterrupt(cancelButton, cancelButtonChange, CHANGE);

  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ClearLCD();
  lcd.setCursor(0,0);
  lcd.print("WiFi connected");
  lcd.setCursor(0,1);
  lcd.print(WiFi.localIP());
  delay(1000);
  
  Serial.println("DB - Connecting...");
  
  ResetCounter = 0;
while (conn.connect(server_addr, 3306, user, spassword) != true) {
	delay(800);
    Serial.print( "." );
    Serial.print(ResetCounter);
    ResetCounter++;
    if (ResetCounter >= 60) {
		Serial.print("ESP8266 reset!");
		ESP.restart();
		}
    }
  
  ClearLCD();
  lcd.setCursor(0,0);
  lcd.print("SQL connected");
  delay(1000);

}

void loop(){

while (looper == 1) {
	
	if (counter == 60) {
		//NTP start
		//get a random server from the pool
		//WiFi.hostByName(ntpServerName, timeServerIP); 
		IPAddress timeServerIP(192, 168, 42, 185); //local IP address for NTP server

		sendNTPpacket(timeServerIP); // send an NTP packet to a time server
		// wait to see if a reply is available
		delay(1000);
		  
		int cb = udp.parsePacket();
		if (!cb) {
			  Serial.println("no packet yet");
			  ClearLCD();
			  } else {
			  Serial.print("packet received, length=");
			  Serial.println(cb);
			  // We've received a packet, read the data from it
			  udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
		  
			  //the timestamp starts at byte 40 of the received packet and is four bytes,
			  // or two words, long. First, esxtract the two words:
		  
			  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
			  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
			  // combine the four bytes (two words) into a long integer
			  // this is NTP time (seconds since Jan 1 1900):
			  unsigned long secsSince1900 = highWord << 16 | lowWord;
			  Serial.print("Seconds since Jan 1 1900 = " );
			  Serial.println(secsSince1900);
		  
			  // now convert NTP time into everyday time:
			  Serial.print("Unix time = ");
			  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
			  const unsigned long seventyYears = 2208988800UL;
			  // subtract seventy years:
			  unsigned long epoch = secsSince1900 - seventyYears;
			  // print Unix time:
			  Serial.println(epoch);
		  
		  
			  // print the hour, minute and second:
			  Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
			  Serial.print((epoch  % 86400L) / 3600); // print the hour (86400 equals secs per day)
			  Serial.print(':');
			  if ( ((epoch % 3600) / 60) < 10 ) {
				// In the first 10 minutes of each hour, we'll want a leading '0'
				Serial.print('0');
			  }
			  Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
			  Serial.print(':');
			  if ( (epoch % 60) < 10 ) {
				// In the first 10 seconds of each minute, we'll want a leading '0'
				Serial.print('0');
			  }
			  Serial.println(epoch % 60); // print the second
			//mod

		int tz = 8;                                            // adjust for PH time
			  DateTime gt(epoch + (tz*60*60));                       // obtain date & time based on NTP-derived epoch...
			  DateTime ntime(epoch + (tz*60*60));                    // if in DST correct for GMT-4 hours else GMT-5
			  RTC.adjust(ntime);                                     // and set RTC to correct local time   
			  nyr = ntime.year()-2000;                       
			  nmo = ntime.month();
			  ndy = ntime.day();    
			  nh  = ntime.hour(); if(nh==0) nh=24;                   // adjust to 1-24            
			  nm  = ntime.minute();                     
			  ns  = ntime.second();                     

			  Serial.print(F("... NTP packet local time: [GMT + ")); Serial.print(tz); Serial.print(F("]: "));       // Local time at Greenwich Meridian (GMT) - offset  
			  if(nh < 10) Serial.print(F(" ")); Serial.print(nh);  Serial.print(F(":"));          // print the hour 
			  if(nm < 10) Serial.print(F("0")); Serial.print(nm);  Serial.print(F(":"));          // print the minute
			  if(ns < 10) Serial.print(F("0")); Serial.print(ns);                       // print the second

			  Serial.print(F(" on "));                                        // Local date
			  if(nmo < 10) Serial.print(F("0")); Serial.print(nmo);  Serial.print(F("/"));        // print the month
			  if(ndy < 10) Serial.print(F("0")); Serial.print(ndy); Serial.print(F("/"));                   // print the day
			  if(nyr < 10) Serial.print(F("0")); Serial.println(nyr);          // print the year
			  Serial.println();

			  ClearLCD();

			  if(nh < 10) lcd.print(F(" ")); lcd.print(nh);  lcd.print(F(":"));	// print the hour 
			  if(nm < 10) lcd.print(F("0")); lcd.print(nm);						// print the minute
			  lcd.print(F(" - "));                                        		// Local date
			  if(nmo < 10) lcd.print(F("0")); lcd.print(nmo);  lcd.print(F("/"));	// print the month
			  if(ndy < 10) lcd.print(F("0")); lcd.print(ndy); lcd.print(F("/"));	// print the day
			  if(nyr < 10) lcd.print(F("0")); lcd.print(nyr);          				// print the year
			  
			  lcd.setCursor(0,1);
			  lcd.print("Press Start");

		//mod end
		}
		//NTP End
		counter = 0;
	}
	Serial.print("looper value : ");
	Serial.println(looper);
	Serial.print("counter value : ");
	Serial.println(counter);
	counter++;
	delay(1000);
}

buzzerFunction(2);
delay(1000);
ClearLCD();
lcd.print("Select Cell :");

looper = 1;
looper2 = 1;

while (looper3 == 0) {
	while (looper2 == 1) {
		lcd.setCursor(0,1);
		potVal = analogRead(potPin);    // read the potValue from the sensor
		if (potVal > 260 && potVal < 274) {cellNo=1;}
		if (potVal > 282 && potVal < 296) {cellNo=2;}
		if (potVal > 304 && potVal < 318) {cellNo=3;}
		if (potVal > 326 && potVal < 340) {cellNo=4;}
		if (potVal > 348 && potVal < 362) {cellNo=5;}
		if (potVal > 370 && potVal < 384) {cellNo=6;}
		if (potVal > 392 && potVal < 406) {cellNo=7;}
		if (potVal > 414 && potVal < 428) {cellNo=8;}
		if (potVal > 436 && potVal < 450) {cellNo=9;}
		if (potVal > 458 && potVal < 472) {cellNo=10;}
		if (potVal > 480 && potVal < 494) {cellNo=11;}
		if (potVal > 502 && potVal < 516) {cellNo=12;}
		if (potVal > 524 && potVal < 538) {cellNo=13;}
		if (potVal > 546 && potVal < 560) {cellNo=14;}
		if (potVal > 568 && potVal < 582) {cellNo=15;}
		if (potVal > 590 && potVal < 604) {cellNo=16;}
		if (potVal > 612 && potVal < 626) {cellNo=17;}
		if (potVal > 634 && potVal < 648) {cellNo=18;}
		if (potVal > 656 && potVal < 670) {cellNo=19;}
		if (potVal > 678 && potVal < 692) {cellNo=20;}
		if (potVal > 700 && potVal < 714) {cellNo=21;}
		if (potVal > 722 && potVal < 736) {cellNo=22;}
		if (potVal > 744 && potVal < 758) {cellNo=23;}
		if (potVal > 766 && potVal < 780) {cellNo=24;}
		if (potVal > 788 && potVal < 802) {cellNo=25;}
		if (potVal > 810 && potVal < 824) {cellNo=26;}
		if (potVal > 832 && potVal < 846) {cellNo=27;}
		if (potVal > 854 && potVal < 868) {cellNo=28;}
		if (potVal > 876 && potVal < 890) {cellNo=29;}
		if (potVal > 898 && potVal < 912) {cellNo=30;}
		if (potVal > 920 && potVal < 934) {cellNo=31;}
		if (potVal > 942 && potVal < 956) {cellNo=32;}
		if (potVal > 964 && potVal < 978) {cellNo=33;}
		if (potVal > 986 && potVal < 1000) {cellNo=34;}
		if (potVal > 1008 && potVal < 1022) {cellNo=35;}
		if (cellNo > 0 && cellNo < 10){
			ClearLCD();
			lcd.print("Select Cell :");
			lcd.setCursor(0,1);
			}
		lcd.print(cellNo);
		delay(100);
		Serial.print("*");
	}
	//reset button state

	ClearLCD();
	lcd.print("Sending Cell :");
	lcd.setCursor(0,1);
	lcd.print(cellNo);
	
	Serial.println("Sending Start");
	//SQL start
	//row_values *row = NULL;
	SQLserverConnect();
	//char taskID
	delay(500);
	Serial.println("SQL query sending task");
	// Initiate the query class instance
	MySQL_Cursor *cur_mem2 = new MySQL_Cursor(&conn);
	sprintf(query, QUERY_INSERT, cellNo);
	// Execute the query
	cur_mem2->execute(query);
	
	Serial.print("value of char = ");
	Serial.println(query);
	delay(500);
	//delete cur_mem;
	//conn.close();
	// SQL end
	
	ClearLCD();
	lcd.print("Cell info sent!");
	
	looper = 1;
	looper2 = 1;
	looper3 = 1;
	counter = 60;	
}

delay(10000);
  
  yield();
  //end main loop
}

void ClearLCD() {
	lcd.clear();
	lcd.setCursor(0,0);
}

int buzzerFunction(int counter){
  for (int buzzerTimer = 1; buzzerTimer <= counter; buzzerTimer++){
  tone(buzzer, 5000); // Send 5KHz sound signal...
  delay(100);        // ...for .1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for .1sec
  }
}

int SQLserverConnect() {
	int ResetCounter = 0;
while (conn.connect(server_addr, 3306, user, spassword) != true) {
	delay(800);
    Serial.print( "." );
    Serial.print(ResetCounter);
    ResetCounter++;
	
    }
	Serial.println( "connected again!" );
}

void startButtonChange() {
	
	looper = 0;
	looper2 = 0;
	looper3 = 0;
	Serial.println("button press");
	
}

void cancelButtonChange() {
	
	ESP.restart();
	
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address){
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
