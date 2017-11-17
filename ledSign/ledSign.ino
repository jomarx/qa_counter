/*
MySQL Connector/Arduino Example : basic insert

This example demonstrates how to issue an INSERT query to store data in a
table. For this, we will create a special database and table for testing.
The following are the SQL commands you will need to run in order to setup
your database for running this sketch.

CREATE DATABASE test_arduino;
CREATE TABLE test_arduino.hello_arduino (
num integer primary key auto_increment,
message char(40),
recorded timestamp
);

Here we see one database and a table with three fields; a primary key that
is an auto_increment, a string, and a timestamp. This will demonstrate how
to save a date and time of when the row was inserted, which can help you
determine when data was recorded or updated.

INSTRUCTIONS FOR USE

1) Create the database and table as shown above.
2) Change the address of the server to the IP address of the MySQL server
3) Change the user and password to a valid MySQL user and password
4) Connect a USB cable to your Arduino
5) Select the correct board and port
6) Compile and upload the sketch to your Arduino
7) Once uploaded, open Serial Monitor (use 115200 speed) and observe
8) After the sketch has run for some time, open a mysql client and issue
the command: "SELECT * FROM test_arduino.hello_arduino" to see the data
recorded. Note the field values and how the database handles both the
auto_increment and timestamp fields for us. You can clear the data with
"DELETE FROM test_arduino.hello_arduino".

Note: The MAC address can be anything so long as it is unique on your network.

Created by: Dr. Charles A. Bell 
Modified by: f41ardu for use with nodemcu

*/
/* 
 * Demo for SSD1306 based 128x64 OLED module using Adafruit SSD1306 
 * library (https://github.com/adafruit/Adafruit_SSD1306).
 * 
 * See https://github.com/pacodelgado/arduino/wiki/SSD1306-based-OLED-connected-to-Arduino 
 * for more information.
 *
 */

 /*

 Udp NTP Client

 Get the time from a Network Time Protocol (NTP) time server
 Demonstrates use of UDP sendPacket and ReceivePacket
 For more on NTP time servers and the messages needed to communicate with them,
 see http://en.wikipedia.org/wiki/Network_Time_Protocol

 created 4 Sep 2010
 by Michael Margolis
 modified 9 Apr 2012
 by Tom Igoe
 updated for the ESP8266 12 Apr 2015 
 by Ivan Grokhotkov

 This code is in the public domain.

 */
 
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>


/* Create an instance for the SSD1306 OLED display in SPI mode 
 * connection scheme: 
 *   D0=sck=Pin 12 / 0 / 2
 *   D1=mosi=Pin 11 / 1 / 3
 *   CS=Pin 8  / 4 / 6
 *   DC=A0=Pin 9 / 3 / 5
 *   Reset=Pin 10 / 2 / 4
 */

//SQL read
#include <WiFiClient.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>


//IPAddress server_addr(192,168,143,132); // IP of the MySQL server here
IPAddress server_addr(192,168,143,220); // IP of the MySQL server here
char user[] = "jomar"; // MySQL user login username
char password[] = "magic44ever"; // MySQL user login password

// Sample query
//char query[] = "SELECT population FROM world.city WHERE name = 'New York'";
char QUERY_POP[] = "SELECT num, cellNo FROM kpi_mech.counter WHERE status = %d ORDER BY dateTime ASC limit 1; ";
char QUERY_UPDATE[] = "UPDATE kpi_mech.counter SET status = 1 WHERE num = %lu; ";
char query[256];

//SQL variables
long taskID = 0;
long cellLocation = 0;

WiFiClient client;
MySQL_Connection conn((Client *)&client);


//wifi
char ssid[] = "outsourcing1.25s"; // your SSID
char pass[] = "dbafe12345!!"; // your SSID Password
//char ssid[] = "jomarAP-SP";  //  your network SSID (name)
//char pass[] = "maquinay1";       // your network password
// const char* host = "utcnist2.colorado.edu";
const char* host = "192.168.143.1";
//const char* host = "192.168.42.185"; //laptop NTP server

int ln = 0;
String TimeDate = "";

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

//Adafruit_SSD1306 display(OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

//buzzer
const int buzzer = 15;

//button
const int startButton = 12;
const int cancelButton = 13;
int buttonState1 = 1;
int buttonState2 = 1;

//mechanic ID
int mechanicID = 49;


// Adafruit_NeoMatrix example for tiled NeoPixel matrices.  Scrolls
// 'Howdy' across three 10x8 NeoPixel grids that were created using
// NeoPixel 60 LEDs per meter flex strip.

#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#ifndef PSTR
 #define PSTR // Make Arduino Due happy
#endif

#define PIN 5

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(8, 8, 1, 1, PIN,
  NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_PROGRESSIVE,
  NEO_GRB + NEO_KHZ400);

const uint16_t colors[] = {
  matrix.Color(255, 0, 0), matrix.Color(0, 255, 0), matrix.Color(0, 0, 255) };

int x = matrix.width();
int pass1 = 0;

void setup(){  

  matrix.begin();
  matrix.setTextWrap(false);
  matrix.setBrightness(40);
  matrix.setTextColor(colors[0]);
  Serial.begin(9600);

pinMode(startButton, INPUT_PULLUP);
pinMode(cancelButton, INPUT_PULLUP);

//  Serial.begin(9600);

//buzzer initialize
buzzerFunction(3);

//start NTP
Serial.begin(9600);
Serial.println();
Serial.println();

// We start by connecting to a WiFi network
Serial.print("Connecting to ");
Serial.println(ssid);
WiFi.begin(ssid, pass);

int ResetCounter = 0;
while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.print(ResetCounter);
    ResetCounter++;
    if (ResetCounter >= 30) {
		Serial.print("ESP8266 reset!");
		ESP.restart();
      }
}

Serial.println("");


Serial.println("WiFi connected");
Serial.println("IP address: ");
Serial.println(WiFi.localIP());
delayer(2);

Serial.println("Starting UDP");
udp.begin(localPort);
Serial.print("Local port: ");
Serial.println(udp.localPort());
//end NTP
 
//start SQL DB connection
Serial.println("DB - Connecting...");

ResetCounter = 0;
while (conn.connect(server_addr, 3306, user, password) != true) {
	delay(800);
    Serial.print( "." );
    Serial.print(ResetCounter);
    ResetCounter++;
    if (ResetCounter >= 60) {
		Serial.print("SQL cannot connect");
		Serial.print("ESP8266 reset!");
		ESP.restart();
      }
    }
}

void loop(){
	
	matrix.fillScreen(0);
	matrix.show();
	
	//NTP start
	//get a random server from the pool
	//WiFi.hostByName(ntpServerName, timeServerIP); 
	IPAddress timeServerIP(192, 168, 42, 185); //local IP address for NTP server

	sendNTPpacket(timeServerIP); // send an NTP packet to a time server
	// wait to see if a reply is available
	delay(500);
	  
	int cb = udp.parsePacket();
	  if (!cb) {
		  Serial.println("no packet yet");
		  
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

	}
	//NTP End

	//SQL start
	row_values *row = NULL;
	//char taskID
	delay(500);
	Serial.println("SQL query to search for available Cells");
	// Initiate the query class instance
	MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
	// Supply the parameter for the query
	// Here we use the QUERY_POP as the format string and query as the
	// destination. This uses twice the memory so another option would be
	// to allocate one buffer for all formatted queries or allocate the
	// memory as needed (just make sure you allocate enough memory and
	// free it when you're done!).
	sprintf(query, QUERY_POP, 0);
	// Execute the query
	cur_mem->execute(query);
	// Fetch the columns (required) but we don't use them.
	column_names *columns = cur_mem->get_columns();
	// Read the row (we are only expecting the one)
	do {
		row = cur_mem->get_next_row();
		if (row != NULL) {
			taskID = atol(row->values[0]);
			Serial.print("value of num = ");
			Serial.println(taskID);
			cellLocation = atol(row->values[1]);
			Serial.print("value of cellNo = ");
			Serial.println(cellLocation);
			}
		} while (row != NULL);
	// Deleting the cursor also frees up memory used
	//delete cur_mem;

	// Show the result
	if (cellLocation == 0) {
		Serial.println("No task,\n sleeping for 2mins");
		buzzerFunction(1);
		delayer(1);
		delay(100);
		ESP.deepSleep(60000000*2);
		//sleep esp8266 for 2min
		ESP.restart();
}
  
//screen
//task detected

Serial.println("Starting loop, printing initial display");

for (int looper = 0; looper<=1000; looper++){
	Serial.print("start up! ");
  matrix.fillScreen(0);
  matrix.setCursor(0, 0);
  matrix.print(cellLocation);
  if(--x < -36) {
    x = matrix.width();
    if(++pass1 >= 3) pass1 = 0;
    matrix.setTextColor(colors[pass1]);
  }
  matrix.show();
  delay(100);
  yield();
}

matrix.fillScreen(0);
matrix.show();


//SQL start
//row_values *row = NULL;
SQLserverConnect();
//char taskID
delay(500);
Serial.println("SQL query update task");
// Initiate the query class instance
MySQL_Cursor *cur_mem2 = new MySQL_Cursor(&conn);
sprintf(query, QUERY_UPDATE, taskID);
// Execute the query
cur_mem2->execute(query);
Serial.print("value of char = ");
Serial.println(query);
//delete cur_mem;
//conn.close();
cellLocation = atol(row->values[1]);
// SQL end
yield();
taskID = 0;
cellLocation = 0;

}



int delayer(int dly){ //delay for x seconds
	Serial.print("Delayer");
	for (int DelayDaw = 0; DelayDaw <= dly; DelayDaw++){
		delay(1000);
		Serial.print(DelayDaw);
		Serial.print(".");
	}
}

int buzzerFunction(int counter){
  for (int buzzerTimer = 1; buzzerTimer <= counter; buzzerTimer++){
  tone(buzzer, 5000); // Send 5KHz sound signal...
  delay(100);        // ...for .1 sec
  noTone(buzzer);     // Stop sound...
  delay(100);        // ...for .1sec
  }
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

int SQLserverConnect() {
	int ResetCounter = 0;
	Serial.print( "connecting to SQL server" );
while (conn.connect(server_addr, 3306, user, password) != true) {
	delay(800);
    Serial.print( "." );
    Serial.print(ResetCounter);
    ResetCounter++;
	
    }
	Serial.print( "connected!" );
}
