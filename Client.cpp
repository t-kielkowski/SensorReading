#include <Wire.h>
#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "ClosedCube_SHT31D.h"
#include "ClosedCube_HDC1080.h"

ClosedCube_SHT31D sht3xd;
ClosedCube_HDC1080 hdc1080;
AsyncWebServer server(80);

const char* ssid = "UL_1";
const char* password = "123456789";
const char* sleepTimeArg = "deepSleep";
bool shouldDeepSleep = false;
String sleepTime;
long long deepSleepTime;
char* p;

float temperature = 0.0;
float humidity = 0.0;

void setup(){	
  Serial.begin(115200); 
  InitializeSensor();   
  
  Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP); 
  
  SetServerMethod();  
}

void InitializeSensor(){
    hdc1080.begin(0x40);
    sht3xd.begin(0x44);
}

void loop(){
	
	UpdateSensors(sht3xd.readTempAndHumidity(SHT3XD_REPEATABILITY_HIGH, SHT3XD_MODE_POLLING, 50));

	if(shouldDeepSleep)
	{
		WiFi.forceSleepBegin();  
		ESP.deepSleep(deepSleepTime);    
	}
	
	delay(5000); 	
}

void UpdateSensors(SHT31D result){
    t = result.t;
    h = result.rh;
}

void SetServerMethod(){
	
	server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temperature).c_str());
	});

	server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(humidity).c_str());
	});  

	server.on("/deepSleep",  HTTP_POST, [] (AsyncWebServerRequest *request){
		if (request->hasArg(sleepTimeArg))
		{
			sleepTime= request->arg(sleepTimeArg);			
			
			if(sleepTime != "0")
				GoDeepSleep(request, sleepTime);
			else
				SetDeepSleepAlarm(request);	
		}
		else 
			SetDeepSleepAlarm(request);	
		});

  server.begin();	
}

void GoDeepSleep(AsyncWebServerRequest *request, String sleepTime){
	request->send(200, "text/plain", "Server goes into deep sleep");
	deepSleepTime = strtoll(sleepTime.c_str(),&p, 10);
	shouldDeepSleep = true;
}

void SetDeepSleepAlarm(AsyncWebServerRequest *request){
	request->send(200, "text/plain", "Server does not go into deep sleep");
}