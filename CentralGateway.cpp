#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <TinyGsmClient.h>
#include "Adafruit_SHT31.h"
#include <Adafruit_ADS1X15.h>
#include <Adafruit_BMP280.h>
#include <BH1750.h>

#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 27
#define MODEM_RX 26
#define I2C_SDA_POWER 21
#define I2C_SCL_POWER 22
#define I2C_SDA 18
#define I2C_SCL 19

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
TinyGsmClient client(modem);

Adafruit_SHT31 sht31 = Adafruit_SHT31();
Adafruit_BMP280 bmp280;
BH1750 lightMeter;
Adafruit_ADS1015 ads;

const char *apn = "internet";
const char *gprsUser = "";
const char *gprsPass = "";

const char *server = "moj-miod.pl";
String apiKeyValue = "tPmAT5Ab3j7F9";
const int port = 80;
const char *currentTime = "/get-current-time.php";
const char *postWeightSensorData = "/post-weight-data.php"; 
const char *postGatewaySensorData = "/post-esp-data.php"; 

const char *clientWeightServer = "http://192.168.4.1/weight";
const char *clientTemperatureServer = "http://192.168.4.1/temperature";
const char *clientHumidityServer = "http://192.168.4.1/moisture";
const char *clientBatteryLevel = "http://192.168.4.1/batteryLevel";
const char *clientDeepSleepServer = "http://192.168.4.1/deepSleep";

const float maxVoltage = 3.45f;
const float minVoltage = 2.50f;
const float factor = maxVoltage - minVoltage; 

String _noData = "NoData";
String _currentTime = "CurrentTime:";

const char * weightSsids[] = {"UL_1"};
const int weightArraySize = (sizeof(weightSsids) / sizeof(weightSsids[0]));

void setup()
{
	SerialMon.begin(115200);
	SetI2C();
	SetModemPins();
	SynchronizeDataTransferTimeToServer();
}

void loop()
{
	String httpRequestData[weightArraySize];

	for (int i = 0; i < weightArraySize; i++)
	{
		auto weightSsid = weightSsids[i];
		ConnectToScaleUsingWifi(weightSsid);

		auto weightId = String(weightSsid);
		auto weight = GetSensorDataFromScale(clientWeightServer);
		auto temperature = GetSensorDataFromScale(clientTemperatureServer);
		auto moisture = GetSensorDataFromScale(clientHumidityServer);
		auto batteryLevel = GetSensorDataFromScale(clientBatteryLevel);

		DisconnectWiFi();

		if (temperature != _noData && moisture != _noData && batteryLevel != _noData && batteryLevel != _noData)
			httpRequestData[i] = CreateHttpRequestDataBasedOnScaleReadings(weightId, weight, temperature, moisture, batteryLevel);
		else
			httpRequestData[i] = _noData;		
	}

	if (ConnectToMobileInternet())
	{
		for (int i = 0; i < weightArraySize; i++)
		{
			if (httpRequestData[i] == _noData)
				SendAlertSms(String(weightSsids[i]));
			else
				SendSensorsDataToServer(httpRequestData[i], postWeightSensorData);
		}

		DisconnectMobileInternet();
	}

	SendReadingsFromYourSensors();
	PutAllScalesToSleeAndPutYourselfToSleep(httpRequestData);

	delay(60000);
}

void ConnectToScaleUsingWifi(const char * weightSsid)
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

void DisconnectWiFi()
{
	WiFi.disconnect();
	delay(100);
}

bool ConnectToMobileInternet()
{
	SerialAT.begin(115200, SERIAL_8N1, MODEM_RX, MODEM_TX);
	delay(3000);

	SerialMon.println("Initializing modem...");
	modem.restart();

	SerialMon.print("Connecting to Mobile Internet Orange");
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

void DisconnectMobileInternet()
{
	modem.gprsDisconnect();
	delay(100);
	SerialMon.println(F("GPRS disconnected"));
}

void SynchronizeDataTransferTimeToServer()
{
	if (ConnectToMobileInternet())
	{
		String fullTime = GetCurrentTime();
		DisconnectMobileInternet();

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

void SendSensorsDataToServer(String httpRequestData, const char *postUrl)
{
	SerialMon.print("Connecting to ");
	SerialMon.print(server);

	if (!client.connect(server, port))
	{
		SerialMon.println(" fail");
	}
	else
	{
		SerialMon.println(" OK");
		SerialMon.println("Performing HTTP POST request...");

		client.print(String("POST ") + postUrl + " HTTP/1.1\r\n");
		client.print(String("Host: ") + server + "\r\n");
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

void SendReadingsFromYourSensors()
{
	InitializeSensor();
	String httpRequest = CreateHttpRequestDataBasedOnYourSensorsReadings();
	SendSensorsDataToServer(httpRequest, postGatewaySensorData);
}

void PutAllScalesToSleeAndPutYourselfToSleep(String *httpRequestData)
{
	String fullTime = GetCurrentTime();

	int minutes = 60 - fullTime.substring(3, 4).toInt();
	int seconds = 60  - fullTime.substring(5, 7).toInt();

	int buffer = 30;
	int minutesToSeconds = (minutes) * 60; // +/- 3 hours
	int considerNumberOfScales = weightArraySize * 15;

	long long sleepTime;
	sleepTime = (minutesToSeconds + seconds - considerNumberOfScales - buffer) * uSToSecondsFacotr;

	for (int i = 0; i <= weightArraySize; i++)
	{
		if (httpRequestData[i] == _noData)
		{
			SendDeepSleepCommandToClient(weightSsids[i], sleepTime);
			sleepTime -= 15 * uSToSecondsFacotr;
		}
	}

	ESP.deepSleep(sleepTime);
}

void SendDeepSleepCommandToClient(const char * weightSsid, long long sleepTime)
{
	ConnectToScaleUsingWifi(weightSsid);
	WiFiClient client;
	HTTPClient httpClient;

	String data = "sleepTime=" + sleepTime;
	httpClient.begin(client, clientDeepSleepServer);
	httpClient.addHeader("Content-Type", "application/x-www-form-urlencoded");
	int httpResponseCode = httpClient.POST(data);
	String content = httpClient.getString();
	httpClient.end();

	Serial.print("ANSWER POST content: ");
	Serial.println(content);
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

int GetSoilMoisture()
{
  int16_t adc1;
  adc1 = ads.readADC_SingleEnded(1);

  return map(adc1, 1090.0f, 762.0f, 0, 100);
}

int GetBaterryLevel()
{
  int16_t adc0;      
  adc0 = ads.readADC_SingleEnded(0);

  float voltage = (adc0 * maxVoltage) / 1121.0;
  return ((voltage - minVoltage) / factor) * 100; 
}

String CreateHttpRequestDataBasedOnScaleReadings(String weightId, String weight, String temperature, String moisture, String batteryLevel)
{
	String httpRequestData =
		"api_key=" + apiKeyValue +
		"&WeightId=" + String(weightId) +
		"&Weight=" + String(weight) +
		"&Temperature=" + String(temperature) +
		"&Moisture=" + String(moisture) +
		"&BatteryLevel=" + String(batteryLevel) + "";

	return httpRequestData;
}

String CreateHttpRequestDataBasedOnYourSensorsReadings()
{
	String httpRequestData =
		"api_key=" + apiKeyValue +
		"&GatewayId=" + "Trawniki" +
		"&BH1750_Luks=" + String(lightMeter.readLightLevel()) +
		"&BMP280_Pressure=" + String(bmp280.readPressure() / 100.0f) +
		"&BMP280_Temp=" + String(bmp280.readTemperature()) +
		"&SHT31_Temp=" + String(sht31.readTemperature()) +
		"&SHT31_Mois=" + String(sht31.readHumidity()) +
		"&SoilMoisture=" + String(GetSoilMoisture()) +
		"&BatteryLevel=" + String(GetBaterryLevel()) + "";

	return httpRequestData;
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
	I2CPower.begin(I2C_SDA_POWER, I2C_SCL_POWER, 400000);

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
	Wire.begin(I2C_SDA, I2C_SCL);

	// Initialize BH1750
	lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23);
	// Initialize SHT31
	sht31.begin(0x44);
	// Initialize BMP280
	bmp280.begin(0x76);
	// Initialize ADS1X15
	ads.begin(0x48);
}

void SendAlertSms(String smsMessage)
{
	if (!modem.sendSMS(PhoneNumber, smsMessage))
	{
		SerialMon.println("SMS failed to send");
	}
}