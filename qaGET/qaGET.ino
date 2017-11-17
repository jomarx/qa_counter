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
#include <ESP8266WiFi.h>
#include "config.h"

char serverAddress[] = "192.168.143.23";  // server address
int port = 80;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);
int status = WL_IDLE_STATUS;
String response;
int statusCode = 0;

String contentType = "application/x-www-form-urlencoded";

int clientResponse = 0;

void setup() {
Serial.begin(9600);
// Connect to WPA/WPA2 network:
Serial.print("Attempting to connect to Network named: ");
wifiConnect();

// print the SSID of the network you're attached to:
Serial.print("SSID: ");
Serial.println(WiFi.SSID());

// print your WiFi shield's IP address:
IPAddress ip = WiFi.localIP();
Serial.print("IP Address: ");
Serial.println(ip);
}

void loop() {
	clientResponse = checkRingStatQuery();
	
	delayer(5);
	
	if (clientResponse > 0) {
		changeRingStatQuery(clientResponse);
	} else {
		Serial.println("no task available!");
	}
	
	delayer(5);
}

int checkRingStatQuery () {
	Serial.println("making POST request to check ringStat");
	
	String postData = "typer=1";
	//String postData = "typer=1&password=admin";

	client.post("/jm/qaquerytask.php", contentType, postData);

	// read the status code and body of the response
	statusCode = client.responseStatusCode();
	response = client.responseBody();

	Serial.print("Status code: ");
	Serial.println(statusCode);
	Serial.print("Response: ");
	Serial.println(response);

	//disconnect client
	client.stop();

	Serial.println("Wait five seconds");
	delayer(5);
	
	if (response != "") {
		clientResponse = response.indexOf(',');
		Serial.println("summoning found, beeping!");
		buzzerFunction(3);
	} else {
		//
		Serial.println("no task found, checking again after 60 secs!");
		delayer(60);
	}
	
	return clientResponse;
}

int changeRingStatQuery (int num) {
	Serial.println("making POST request to set ringStat to zero");
	String postData = "typer=2&taskID=";
	postData += num;
	Serial.print("postData: ");
	Serial.println(postData);

	client.post("/jm/qaquerytask.php", contentType, postData);

	// read the status code and body of the response
	statusCode = client.responseStatusCode();
	response = client.responseBody();

	Serial.print("Status code: ");
	Serial.println(statusCode);
	Serial.print("Response: ");
	Serial.println(response);

	//disconnect client
	client.stop();
	//
}

int buzzerFunction(int counter){
	for (int buzzerTimer = 1; buzzerTimer <= counter; buzzerTimer++){
		//set buzzer on
		Serial.println("BUZZ ON!");
		delayer(1);
		//set buzzer off
		Serial.println("BUZZ OFF!");
		delayer(1);
		
	}
}

int delayer(int dly){
	Serial.print("Delaying ");
	Serial.print(dly);
	Serial.print(" secs : ");
	for (int DelayDaw = 1; DelayDaw <= dly; DelayDaw++){
		delay(500);
		yield();
		delay(500);
		Serial.print(DelayDaw);
		Serial.print(".");
	}
	Serial.println("");
}

void wifiConnect () {
	// Connect to WPA/WPA2 network:
	WiFi.begin(ssid, pass);
	int ResetCounter = 0;
	Serial.print("Attempting to connect to Network ");
	Serial.print("Connecting to ");
	Serial.println(ssid);
	while ( WiFi.status() != WL_CONNECTED) {
		ESP.wdtFeed();
		Serial.print(".");
		ResetCounter++;
		yield();
		delay(300);
		if (ResetCounter >= 60) {
			Serial.print("ESP8266 reset!");
			ESP.restart();
		}
	}
}