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


IPAddress server_addr(192,168,143,200); // Piplay
//IPAddress server_addr(192,168,42,146); // IP of the MySQL server here
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
const int startButton = D6;
const int cancelButton = D7;
int buttonState1 = 1;
int buttonState2 = 1;

//potentiometer selector
int potPin = A0;
int potVal = 0;       // variable to store the potValue coming from the sensor


void setup() {

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
  lcd.init();
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Report Problem");
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
	ClearLCD();
	lcd.print("Please Scan ID");

}

void loop(){

//NTP start
//get a random server from the pool
//WiFi.hostByName(ntpServerName, timeServerIP); 
IPAddress timeServerIP(192, 168, 143, 1); //local IP address for NTP server

sendNTPpacket(timeServerIP); // send an NTP packet to a time server
// wait to see if a reply is available
delay(1000);
  
int cb = udp.parsePacket();
  if (!cb) {
	  Serial.println("no packet yet");
	  displayClear();
	  display.print("\n");
	  display.display();
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

displayClear();

      if(nh < 10) display.print(F(" ")); display.print(nh);  display.print(F(":"));          // print the hour 
      if(nm < 10) display.print(F("0")); display.print(nm);  display.print(F(":"));          // print the minute
      if(ns < 10) display.print(F("0")); display.print(ns);                       // print the second

      display.print(F(" - "));                                        // Local date
      if(nmo < 10) display.print(F("0")); display.print(nmo);  display.print(F("/"));        // print the month
      if(ndy < 10) display.print(F("0")); display.print(ndy); display.print(F("/"));                   // print the day
      if(nyr < 10) display.print(F("0")); display.print(nyr);          // print the year
      display.println();      

display.display();
//mod end
}
//NTP End
	
	//reset button state
	buttonState1 = 1;
	buttonState2 = 1;

  receivedTag=false;
  int TNLeaveLoop = 0;
  int countToFive = 0;
  int tempTimer = 0;
  int cellLocation = 0;

	delay (500);
    Serial.print("Start loop / "); 
	Serial.print(countToTwo); 
	Serial.print(" / ");
	Serial.print(countToloop); 
	Serial.print(" \n ");
	//lcd.noBacklight();

  delay(400);
  
  yield();
  
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
	
	buttonState1 = 0;
	
}

void cancelButtonChange() {
	
	buttonState2 = 0;
	
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
