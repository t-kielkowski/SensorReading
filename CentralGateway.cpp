#define TINY_GSM_MODEM_SIM800
#define TINY_GSM_RX_BUFFER 1024

#include <Wire.h>
#include <TinyGsmClient.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>

#define MODEM_RST 5
#define MODEM_PWKEY 4
#define MODEM_POWER_ON 23
#define MODEM_TX 27
#define MODEM_RX 26

#define SerialMon Serial
#define SerialAT Serial1
#define uSToSecondsFacotr 1000000UL  
#define PhoneNumber "+48796794794"

#ifdef DUMP_AT_COMMANDS
#include <StreamDebugger.h>
StreamDebugger debugger(SerialAT, SerialMon);
TinyGsm modem(debugger);
#else
TinyGsm modem(SerialAT);
#endif

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
const char *clientDeepSleepServer = "http://192.168.4.1/deepSleep";
const char *postServerName = "http://moj-miod.pl/post-sht31.php";
String apiKeyValue = "tPmAT5Ab3j7F9";

String _noData = "NoData";
String _currentTime = "CurrentTime:";

//const char *weightSsid = "UL_1";
char *weightSsid[] = {"UL_1", "UL_2"};

void setup()
{
	SerialMon.begin(115200);
	SetModemPins();
}

void loop()
{
	if(ConnectToMobileInternet())
	{
		SynchronizeDataTransferTimeToServer();

		String temperature;
		String humidity;
		int weightArraySize = (sizeof(weightSsid) / sizeof(weightSsid[0]));
		int isSuccess[weightArraySize];	

		for (int i = 0; i <= weightArraySize; i++)
		{
			String weight = weightSsid[i];
			ConnectToScaleUsingWifi(weight);

			temperature = GetSensorDataFromScale(clientTemperatureServer);
			humidity = GetSensorDataFromScale(clientHumidityServer);

			if (temperature != _noData && humidity != _noData)
			{
				SendSensorsDataToServer(weight, temperature, humidity);
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

void SendSensorsDataToServer(String sensorName, String temperature, String humidity)
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

		String httpRequestData =
			"api_key=" + apiKeyValue +
			"&SHT31_Sensor=" + String(sensorName) +
			"&SHT31_Temp=" + String(temperature) +
			"&SHT31_Mois=" + String(humidity) +
			"&SHT31_BatteryLevel=" + String(map(analogRead(A0), 830.0f, 1040.0f, 0, 100)) + "";

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

void SetModemPins()
{
	pinMode(MODEM_PWKEY, OUTPUT);
	pinMode(MODEM_RST, OUTPUT);
	pinMode(MODEM_POWER_ON, OUTPUT);
	digitalWrite(MODEM_PWKEY, LOW);
	digitalWrite(MODEM_RST, HIGH);
	digitalWrite(MODEM_POWER_ON, HIGH);
}