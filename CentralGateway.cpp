#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024

#include <Wire.h>
#include <TinyGsmClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <BH1750.h>
#include "ClosedCube_HDC1080.h"
#include "ClosedCube_SHT31D.h"
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_ADS1X15.h>

#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 27
#define MODEM_RX 26
#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_SDA_2 18
#define I2C_SCL_2 19

#define SerialMon Serial
#define SerialAT Serial1
#define IP5306_ADDR 0x75
#define IP5306_REG_SYS_CTL0 0x00
#define SEALEVELPRESSURE_HPA (1013.25)
#define uSToSecondsFacotr 1000000UL  
#define PhoneNumber "+48796794794"

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

TwoWire I2CPower = TwoWire(0);
TwoWire I2C = TwoWire(1);
BH1750 lightMeter;
ClosedCube_HDC1080 hdc1080;
ClosedCube_SHT31D sht3xd;
Adafruit_BMP280 bme;
Adafruit_ADS1015 ads; 
TinyGsmClient client(modem);

const char apn[] = "internet";
const char gprsUser[] = "";
const char gprsPass[] = "";
const char simPIN[] = "";

const char Server[] = "moj-miod.pl";
const char CurrentTime[] = "/get-current-time.php";
const char PostSensorData[] = "/post-sht31.php"; 
const int Port = 80;

const char *clientTemperatureServer = "http://192.168.4.1/temperature";
const char *clientHumidityServer = "http://192.168.4.1/humidity";
const char *clientBatteryLevel = "http://192.168.4.1/batteryLevel";
const char *clientDeepSleepServer = "http://192.168.4.1/deepSleep";
const char *postServerName = "http://moj-miod.pl/post-sht31.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

const float maxVoltage = 3.45f;
const float minVoltage = 2.50f;
const float factor = maxVoltage - minVoltage; 

String _noData = "NoData";
String _currentTime = "CurrentTime:";

//const char *weightSsid = "UL_1";
char *weightSsid[] = {"UL_1", "UL_2"};

void setup()
{
	SerialMon.begin(115200);
	SetI2C();
	SetModemPins();
}

void loop()
{
	if(ConnectToMobileInternet())
	{
		//SynchronizeDataTransferTimeToServer();

		String temperature;
		String humidity;
		String batteryLevel;
		int weightArraySize = (sizeof(weightSsid) / sizeof(weightSsid[0]));
		SerialMon.print("weightArraySize: ");
		SerialMon.println(weightArraySize);

		int isSuccess[weightArraySize];	

		for (int i = 0; i <= weightArraySize; i++)
		{
			String weight = weightSsid[i];
			ConnectToScaleUsingWifi(weight);

			temperature = GetSensorDataFromScale(clientTemperatureServer);
			humidity = GetSensorDataFromScale(clientHumidityServer);
			batteryLevel = GetSensorDataFromScale(clientBatteryLevel);

			if (temperature != _noData && humidity != _noData)
			{
				String httpRequest = CreateHttpRequestDataBasedOnScaleReadings(weight, temperature, humidity, batteryLevel);
				SendSensorsDataToServer(httpRequest);
				isSuccess[i] = 1;
			}
			else
			{
				SendAlertSms(weight);
				isSuccess[i] = 0;
			}
		}	

		PutAllScalesToSleeAndPutYourselfToSleep(isSuccess, weightArraySize);
	}

	delay(5000);
}

void SynchronizeDataTransferTimeToServer()
{	
	String fullTime = GetCurrentTime();
	int minutes = fullTime.substring(3, 4).toInt();
	int seconds = 60 - fullTime.substring(5, 7).toInt();
	int buffer = 10;

	if (minutes == 59 && seconds > buffer)
	{
		delay(1000 * (seconds - buffer));
	}
	else if (minutes > 50 && minutes < 59)
	{
		int minuteToSeconds = (60 - minutes) * 60;
		delay(1000 * (minuteToSeconds + seconds - buffer));
	}
}

bool ConnectToMobileInternet()
{
	SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
	delay(3000);

	SerialMon.println("Initializing modem...");
	modem.restart();

	SerialMon.print("Connecting to Mobile Internet: ");
	SerialMon.print(apn);

	if (!modem.gprsConnect(apn, gprsUser, gprsPass))
	{
		SerialMon.println(" fail");
		return false;
	}
	else
	{
		SerialMon.println(" OK");
		return true;
	}
}

void ConnectToScaleUsingWifi(String weightSsid)
{
	Serial.print("Connecting to ");
	Serial.print(weightSsid);
	Serial.print("   ");

	WiFi.begin(weightSsid);

	while (WiFi.status() != WL_CONNECTED)
	{
		delay(500);
		Serial.print(".");
	}

	Serial.print("  Connected to WiFi ");
	Serial.println(weightSsid);
}

String GetSensorDataFromScale(const char *clientServerName)
{
	WiFiClient client;
	HTTPClient http;

	http.begin(client, clientServerName);
	int httpResponseCode = http.GET();

	String result = _noData;

	if (httpResponseCode > 0)
	{
		result = http.getString();
	}
	else
	{
		Serial.print("Error code: ");
		Serial.println(httpResponseCode);
	}

	http.end();
	return result;
}

String CreateHttpRequestDataBasedOnScaleReadings(String sensorName, String temperature, String humidity, String batteryLevel)
{
	String httpRequestData =
		"api_key=" + apiKeyValue +
		"&SHT31_Sensor=" + String(sensorName) +
		"&SHT31_Temp=" + String(temperature) +
		"&SHT31_Mois=" + String(humidity) +
		"&SHT31_BatteryLevel=" + String(batteryLevel) + "";

	return httpRequestData;
}

void SendSensorsDataToServer(String httpRequestData)
{
	SerialMon.print("Connecting to ");
	SerialMon.print(Server);

	if (!client.connect(Server, Port))
	{
		SerialMon.println(" fail");
	}
	else
	{
		SerialMon.println(" OK");
		SerialMon.println("Performing HTTP POST request...");

		client.print(String("POST ") + PostSensorData + " HTTP/1.1\r\n");
		client.print(String("Host: ") + Server + "\r\n");
		client.println("Connection: close");
		client.println("Content-Type: application/x-www-form-urlencoded");
		client.print("Content-Length: ");
		client.println(httpRequestData.length());
		client.println();
		client.println(httpRequestData);

		unsigned long timeout = millis();
		while (client.connected() && millis() - timeout < 10000L)
		{
			while (client.available())
			{
				char c = client.read();
				SerialMon.print(c);
				timeout = millis();
			}
      }
      SerialMon.println();

      client.stop();
      SerialMon.println(F("Server disconnected"));
	}
}

String GetCurrentTime()
{
	SerialMon.print("Connecting to ");
	SerialMon.print(Server);
	if (!client.connect(Server, Port))
	{
		SerialMon.println(" fail");
	}
	else
	{
		SerialMon.println(" OK");
		SerialMon.println("Performing HTTP GET request...");

		client.print(String("GET ") + CurrentTime + " HTTP/1.1\r\n");
		client.print(String("Host: ") + Server + "\r\n");
		client.print("Connection: keep-alive\r\n\r\n");

		String response = "";
		unsigned long timeout = millis();
		while (client.connected() && millis() - timeout < 10000L)
		{
			while (client.available())
			{
				char c = client.read();
				response += c;
				timeout = millis();
			}
		}
		int index = response.indexOf(_currentTime) + _currentTime.length() + 1;
		String fullTime = response.substring(index, index + 8);
		SerialMon.println(fullTime);

		client.stop();
		SerialMon.println(F("Server disconnected"));

		return fullTime;
	}
}

void SendReadingsFromYourSensors()
{
	InitializeSensor();
	String httpRequest = CreateHttpRequestDataBasedOnYourSensorsReadings(sht3xd.readTempAndHumidity(SHT3XD_REPEATABILITY_HIGH, SHT3XD_MODE_POLLING, 50));
	SendSensorsDataToServer(httpRequest);
}

String CreateHttpRequestDataBasedOnYourSensorsReadings(SHT31D result)
{
	String httpRequestData =
		"api_key=" + apiKeyValue +
		"&GatewayId=" + gatewayId +
		"&BH1750_Luks=" + String(lightMeter.readLightLevel()) +
		"&BMP280_Pressure=" + String((bme.readPressure() / 100.0f)) +
		"&BMP280_Temp=" + String(bme.readTemperature()) +
		"&HDC1080_Temp=" + String(hdc1080.readTemperature()) +
		"&HDC1080_Mois=" + String(hdc1080.readHumidity()) +
		"&SHT31_Temp=" + String(result.t) +
		"&SHT31_Mois=" + String(result.rh) +
		"&SoilMoisture=" + String(GetSoilMoisture()) +
		"&BatteryLevel=" + String(GetBaterryLevel()) + "";

	return httpRequestData;
}

void PutAllScalesToSleeAndPutYourselfToSleep(int isSuccess[], int weightArraySize)
{
	String fullTime = GetCurrentTime();
	modem.gprsDisconnect();
	SerialMon.println(F("GPRS disconnected"));

	int minutes = 60 - fullTime.substring(3, 4).toInt();
	int seconds = 60  - fullTime.substring(5, 7).toInt();

	int buffer = 30;
	int minutesToSeconds = (minutes) * 60; // +/- 3 hours
	int considerNumberOfScales = weightArraySize * 15;

	long long sleepTime;
	sleepTime = (minutesToSeconds + seconds - considerNumberOfScales - buffer) * uSToSecondsFacotr;

	for (int i = 0; i <= weightArraySize; i++)
	{		
		if (isSuccess[i] == 1)
		{
			SendDeepSleepCommandToClient(weightSsid, sleepTime);
			sleepTime -= 15 * uSToSecondsFacotr;
		}
	}

	WiFi.forceSleepBegin();
	ESP.deepSleep(sleepTime);
}

void SendDeepSleepCommandToClient(String weightSsid, long long sleepTime)
{
	ConnectToScaleUsingWifi(weightSsid);
	WiFiClient client;
	HTTPClient httpClient;

	String data = "sleepTime=" + String(sleepTime);
	httpClient.begin(client, clientDeepSleepServer);
	httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
	int httpResponseCode = httpClient.POST(data);
	String content = httpClient.getString();
	httpClient.end();

	Serial.print("ANSWER POST content: ");
	Serial.println(content);
}

void SendAlertSms(String smsMessage){

	if (!modem.sendSMS(PhoneNumber, smsMessage))
	{
		SerialMon.println("SMS failed to send");
	}
}

int GetBaterryLevel()
{
  int16_t adc1;
  adc1 = ads.readADC_SingleEnded(1); 
  
  float voltage = (adc1 * maxVoltage) / 1121.0;
  return ((voltage - minVoltage) / factor) * 100; 
}

int GetSoilMoisture()
{
  int16_t adc0;      
  adc0 = ads.readADC_SingleEnded(0);  
  return map(adc0, 1090.0f, 762.0f, 0, 100);
}

void SetModemPins()
{
	pinMode(MODEM_PWKEY, OUTPUT);
	pinMode(MODEM_RST, OUTPUT);
	pinMode(MODEM_POWER_ON, OUTPUT);
	digitalWrite(MODEM_PWKEY, LOW);
	digitalWrite(MODEM_RST, HIGH);
	digitalWrite(MODEM_POWER_ON, HIGH);
}

void SetI2C()
{
	I2CPower.begin(I2C_SDA, I2C_SCL, 400000);
	I2C.begin(I2C_SDA_2, I2C_SCL_2, 400000);
	
	bool isOk = setPowerBoostKeepOn(1);
	SerialMon.println(String("IP5306 KeepOn ") + (isOk ? "OK" : "FAIL"));
}

bool setPowerBoostKeepOn(int en)
{
	I2CPower.beginTransmission(IP5306_ADDR);
	I2CPower.write(IP5306_REG_SYS_CTL0);

	if (en)	
		I2CPower.write(0x37);	
	else	
		I2CPower.write(0x35);

	return I2CPower.endTransmission() == 0;
}

void InitializeSensor()
{	
	Wire.begin();

    // Initialize BH1750
    lightMeter.begin();
    // Initialize HDC1080
    hdc1080.begin(0x40);
    // Initialize SHT31
    sht3xd.begin(0x44);
    // Initialize BMP280
    bme.begin(0x76);
	// Initialize ADS1X15
    ads.begin();	
}
