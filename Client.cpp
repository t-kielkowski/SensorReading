#include <Wire.h>
#include <WiFiClient.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "ClosedCube_HDC1080.h"

ClosedCube_HDC1080 hdc1080;
AsyncWebServer server(80);

const char* ssid = "UL_1";
const char* password = "123456789";
const char* sleepTimeArg = "sleepTime";
const float maxVoltage = 3.30f;
const float minVoltage = 2.45f;
const float factor = maxVoltage - minVoltage; 
bool shouldDeepSleep = false;
long long deepSleepTime;
char* p;

float weight = 0.0;
float temp = 0.0;
float hum = 0.0;
int battLev = 0;

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
}

void loop(){
	
	UpdateSensors();

	if(shouldDeepSleep)
	{
		WiFi.forceSleepBegin();  
		ESP.deepSleep(deepSleepTime);    
	}
	
	delay(5000); 	
}

void UpdateSensors(){
    temp = hdc1080.readTemperature();
	Serial.print("Temperature: ");
	Serial.println(temp);
    hum = hdc1080.readHumidity();
	Serial.print("Hum: ");
	Serial.println(hum);
	battLev = GetBaterryLevel();
}

void SetServerMethod(){
	
	server.on("/weight", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(weight).c_str());
	});
	
	server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(temp).c_str());
	});

	server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(hum).c_str());
	});  

	server.on("/batteryLevel", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(battLev).c_str());
	});  

	server.on("/deepSleep",  HTTP_POST, [] (AsyncWebServerRequest *request){
		if (request->hasArg(sleepTimeArg))
		{
			String sleepTime= request->arg(sleepTimeArg);	
			GoDeepSleep(request, sleepTime);
		}		
		else 
			SetDeepSleepAlarm(request);	
		});

  server.begin();	
}

void GoDeepSleep(AsyncWebServerRequest *request, String sleepTime){
	request->send(200, "text/plain", "Server goes into deep sleep");
	deepSleepTime = strtoll(sleepTime.c_str(),&p, 10);
	
	Serial.print("deepSleepTime: ");
	Serial.println(deepSleepTime);
	
	shouldDeepSleep = true;
}

void SetDeepSleepAlarm(AsyncWebServerRequest *request){
	request->send(200, "text/plain", "Server does not go into deep sleep");
}

int GetBaterryLevel()
{
  float voltage = (analogRead(A0) * maxVoltage) / 1024.0;
  return ((voltage - minVoltage) / factor) * 100; 
}