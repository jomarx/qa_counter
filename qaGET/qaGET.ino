/*
  Simple POST client for ArduinoHttpClient library
  Connects to server once every five seconds, sends a POST request
  and a request body
  note: WiFi SSID and password are stored in config.h file.
  If it is not present, add a new tab, call it "config.h" 
  and add the following variables:
  char ssid[] = "ssid";     //  your network SSID (name)
  char pass[] = "password"; // your network password
  created 14 Feb 2016
  by Tom Igoe
  
  this example is in the public domain
 */
#include <ArduinoHttpClient.h>
//#include <ESP8266WiFi.h>
#include <WiFi.h>
#include "config.h"

char serverAddress[] = "192.168.143.220";  // server address
int port = 80;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;
String response;
int statusCode = 0;

String contentType = "application/x-www-form-urlencoded";
String clientResponse = "";
//defines where the plant will ring
String NotifNo = "1";

const int relaOutput = 12;
const int ledLight = 4;

//watchdog timer
const int wdtTimeout = 10000;  //time in ms to trigger the watchdog
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule() {
  ets_printf("WDTreboot\n");
  esp_restart_noos();
}

void setup() {
	Serial.begin(115200);

	timer = timerBegin(0, 80, true);                  //timer 0, div 80
	timerAttachInterrupt(timer, &resetModule, true);  //attach callback
	timerAlarmWrite(timer, wdtTimeout * 1000, false); //set time in us
	timerAlarmEnable(timer);                          //enable interrupt

	WiFi.mode(WIFI_STA);

	// Connect to WPA/WPA2 network:
	wifiConnect();

	// print the SSID of the network you're attached to:
	Serial.print("SSID: ");
	Serial.println(WiFi.SSID());

	// print your WiFi shield's IP address:
	IPAddress ip = WiFi.localIP();
	Serial.print("IP Address: ");
	Serial.println(ip);

	pinMode(relaOutput, OUTPUT);
	pinMode(ledLight, OUTPUT);
}

void loop() {
	timerWrite(timer, 0); //reset timer (feed watchdog)
	digitalWrite(relaOutput, LOW);
	digitalWrite(ledLight, HIGH);
	
	clientResponse = checkRingStatQuery();
	Serial.print("clientResponse: ");
	Serial.println(clientResponse);
	digitalWrite(ledLight, LOW);
	if (clientResponse != "") {
		changeRingStatQuery(clientResponse);
		timerWrite(timer, 0); //reset timer (feed watchdog)
	} else {
		Serial.println("no task available!");
		digitalWrite(ledLight, HIGH);
		timerWrite(timer, 0); //reset timer (feed watchdog)
	}
	
	delayer(1);
	digitalWrite(ledLight, LOW);
	delayer(1);
	timerWrite(timer, 0); //reset timer (feed watchdog)
}

String checkRingStatQuery () {
	Serial.println("making POST request to check ringStat");
	timerWrite(timer, 0); //reset timer (feed watchdog)
	String postData = "tp=1&NotifNo=";
	postData += NotifNo;
	//String postData = "tp=1&password=admin";
	
	long loopTime = millis();
	Serial.print("postData CheckRSQ: ");
	Serial.println(postData);
	client.post("/qaqtk.php", contentType, postData);
	loopTime = millis() - loopTime;
  
	Serial.print("loop time is = ");
	Serial.println(loopTime); //should be under 3000

	// read the status code and body of the response
	statusCode = client.responseStatusCode();
	response = client.responseBody();
	
	timerWrite(timer, 0); //reset timer (feed watchdog)

	Serial.print("checkRingStatQuery-Status code: ");
	Serial.println(statusCode);
	Serial.print("checkRingStatQuery-Response: ");
	Serial.println(response);

	//disconnect client
	client.stop();

	Serial.println("Wait 1 seconds");
	delayer(2);
	
	if (response != "") {
		int firstCommaIndex = response.indexOf(',');
		clientResponse = response.substring(0, firstCommaIndex);
		Serial.println("summoning found, beeping!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
		buzzerFunction(3);
		timerWrite(timer, 0); //reset timer (feed watchdog)
	} else {
		//
		Serial.println("no task found, checking again after 5 secs!");
		client.stop();
		delayer(10);
		timerWrite(timer, 0); //reset timer (feed watchdog)
	}
	Serial.print("clientResponse: ");
	Serial.println(clientResponse);
	return clientResponse;
}

String changeRingStatQuery (String num) {
	timerWrite(timer, 0); //reset timer (feed watchdog)
	Serial.println("making POST request to set ringStat to zero");
	String postData = "tp=2&tid=";
	postData += num;
	postData += "&NotifNo=";
	postData += NotifNo;
	Serial.print("postData ChangeRSQ: ");
	Serial.println(postData);
	
	long loopTime = millis();
	client.post("/qaqtk.php", contentType, postData);
	timerWrite(timer, 0); //reset timer (feed watchdog)
	loopTime = millis() - loopTime;
  
	Serial.print("loop time is = ");
	Serial.println(loopTime); //should be under 10000
	
	// read the status code and body of the response
	statusCode = client.responseStatusCode();
	response = client.responseBody();

	Serial.print("changeRingStatQuery-Status code: ");
	Serial.println(statusCode);
	Serial.print("changeRingStatQuery-Response: ");
	Serial.println(response);

	//disconnect client
	client.stop();
	delay(800);
	//clientResponse = "";
	ESP.restart();
	//
}

int buzzerFunction(int counter){
	for (int buzzerTimer = 1; buzzerTimer <= counter; buzzerTimer++){
		//set buzzer on
		Serial.println("BUZZ ON!");
		digitalWrite(relaOutput, HIGH);
		digitalWrite(ledLight, HIGH);
		delay(800);		//set buzzer off
		Serial.println("BUZZ OFF!");
		digitalWrite(relaOutput, LOW);
		digitalWrite(ledLight, LOW);
		delay(800);
		
	}
}

int delayer(int dly){
	Serial.print("Delaying ");
	Serial.print(dly);
	Serial.print(" secs : ");
	int ledStatus = 0;
	for (int DelayDaw = 1; DelayDaw <= dly; DelayDaw++){
		delay(800);
		Serial.print(DelayDaw);
		Serial.print(".");
		timerWrite(timer, 0); //reset timer (feed watchdog)
		int skipper = 0;
		
		if ((ledStatus == 1)&&(skipper == 0)) {
			//lights off
			digitalWrite(ledLight, LOW);
			ledStatus = 0;
			skipper = 1;
		}
		
		if ((ledStatus == 0)&&(skipper == 0)) {
			//lights on
			digitalWrite(ledLight, HIGH);
			ledStatus = 1;
			skipper = 1;
		}
	}
	
	Serial.println("");
}

void wifiConnect () {
	// Connect to WPA/WPA2 network:
	WiFi.begin(ssid, pass);
	int ResetCounter = 0;
	Serial.print("Attempting to connect to Network ");
	while ( WiFi.status() != WL_CONNECTED) {
		
		Serial.print(".");
		timerWrite(timer, 0); //reset timer (feed watchdog)
		ResetCounter++;
		
		delay(300);
		if (ResetCounter >= 30) {
			Serial.print("ESP8266 reset!");
			ESP.restart();
		}
	}
}