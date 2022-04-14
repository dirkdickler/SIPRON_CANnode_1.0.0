
#include <string.h>
#include <stdlib.h>
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <Arduino_JSON.h>
#include <esp_task_wdt.h>
#include "time.h"
#include <Ticker.h>
#include <EEPROM.h>
#include <TimeLib.h>
#include <Wire.h>
//#include "pcf8563.h"
#include "index.html"
#include "main.h"
#include "define.h"
#include "constants.h"
// #include "FS.h"
// #include "SD.h"
//#include "SPI.h"
//#include <SoftwareSerial.h>
//#include <ESP32Time.h>
#include "HelpFunction.h"
#include "Pin_assigment.h"
//#include "Middleware\Ethernet\WizChip_my_API.h"
#include "esp_log.h"

//#include <ACAN_ESP32.h>
#include "driver/twai.h"
//#include <ACAN_ESP32_CANRegisters.h>

// Replace with your network credentials
// const char* ssid = "Grabcovi";
// const char* password = "40177298";
const char *soft_ap_ssid = "aDum_Server";
const char *soft_ap_password = "aaaaaaaaaa";
// const char *ssid = "semiart";
// const char *password = "aabbccddff";
char NazovSiete[30];
char Heslo[30];

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

JSONVar myObject, myObject2, ObjDatumCas, ObjTopeni, JSON_DebugMsg;
Ticker timer_1ms(Loop_1ms, 1, 0, MILLIS);
Ticker timer_10ms(Loop_10ms, 10, 0, MILLIS);
Ticker timer_100ms(Loop_100ms, 300, 0, MILLIS);
Ticker timer_1sek(Loop_1sek, 1000, 0, MILLIS);
Ticker timer_10sek(Loop_10sek, 10000, 0, MILLIS);

ESP32Time rtc;
// PCF8563_Class PCFrtc;
IPAddress local_IP(192, 168, 1, 14);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);	// optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

TaskHandle_t TWAI_RX_task_hndl;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;		 // 3600;
const int daylightOffset_sec = 0; // 3600; //letny cas
struct tm MyRTC_cas;
bool Internet_CasDostupny = false; // to je ze dostava cas z Inernetu
bool RTC_cas_OK = false;			  // ze mam RTC fakt nastaveny bud z interneru, alebo nastaveny manualne
											  // a to teda ze v RTC mam fakr realny cas
											  // Tento FLAG, nastavi len pri nacitanie casu z internutu, alebo do buducna manualne nastavenie casu cew WEB

char gloBuff[200];
bool LogEnebleWebPage = false;
FLAGS_t flg;
LOGBUFF_t LogBuffer;

VSTUP_t DIN[pocetDIN];
VSTUP_t ADR[pocetADR];
u8 CANadresa = 0;
static const uint32_t DESIRED_BIT_RATE = 125UL * 1000UL; // 125kb/s

char TX_BUF[TX_RX_MAX_BUF_SIZE];
//------------------------------------------------------------------------------------------------------------------

/**********************************************************
 ***************        SETUP         **************
 **********************************************************/

void setup()
{
	Serial.begin(115200);
	Serial.println("Spustam applikaciu.a1");
	System_init();

	ESP_LOGW("", "est ESLP log W");
	ESP_LOGI("TEST SP log I", "storage usedd: %lld/%lld", 23, 24);
	log_i("TEST SP log I", "storage usedd: %lld/%lld", 23, 24);

	// attachInterrupt(digitalPinToInterrupt(ENCODER1), encoder, RISING);
	// pinMode(ENCODER1, INPUT);
	// pinMode(ENCODER2, INPUT);

	ESPinfo();

	myObject["Parameter"]["gateway"]["value"] = " 11:22 Strezda";
	myObject["tes"]["ddd"] = 42;
	myObject["hello"] = "11:22 Streda";
	myObject["true"] = true;
	myObject["x"] = 42;
	myObject2["Citac"] = "ahojj";

	String jsonString = JSON.stringify(myObject);
	Serial.print("JSON strinf dump");
	Serial.println(jsonString);

	Serial.print("myObject.keys() = ");
	Serial.println(myObject.keys());

	myObject2["Citac"] = myObject["Parameter"]["gateway"]["value"];
	jsonString = JSON.stringify(myObject2);
	Serial.print("JSON2 strinf dump");
	Serial.println(jsonString);

	NacitajEEPROM_setting();

	// WiFi_init();    //este si odkomentuj  //WiFi_connect_sequencer(); v 10 sek loop
	// configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

	timer_1ms.start();
	timer_10ms.start();
	timer_100ms.start();
	timer_1sek.start();
	timer_10sek.start();
	esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
	esp_task_wdt_add(NULL);					  // add current thread to WDT watch

	// RS485 musis spustit az tu, lebo ak ju das hore a ESP ceka na konnect wifi, a pridu nejake data na RS485, tak FreeRTOS =RESET  asi overflow;
	// Serial1.begin(9600);

	xTaskCreatePinnedToCore(
		 TWAI_RX_Task,			// Task function
		 "task1",				// Name
		 6000,					// Stack size
		 nullptr,				// Parameters
		 1,						// Priority
		 &TWAI_RX_task_hndl, // handle
		 0							// CPU
	);
}

void loop()
{
	esp_task_wdt_reset();
	// ws.cleanupClients();
	//  AsyncElegantOTA.loop();
	timer_1ms.update();
	timer_10ms.update();
	timer_100ms.update();
	timer_1sek.update();
	timer_10sek.update();
}

void Loop_1ms()
{
}

void Loop_10ms()
{
	ScanInputs();
	Read_DIPAdress(&CANadresa);
}

void Loop_100ms(void)
{
}

void Loop_1sek(void)
{
	// log_i("[1sek Loop]  mam 1 sek....  ");
	String sprava; // = rtc.getTime("\r\n[%H:%M:%S] karta a toto cas z PCF8563:");
						// unsigned long start = micros();
	// sprava += PCFrtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S);
	// unsigned long end = micros();
	// unsigned long delta = end - start;
	// Serial.print("DELTA PCF8563: ");
	// Serial.println(delta);

	if (digitalRead(LED_pin) == 1)
	{
		digitalWrite(LED_pin, 0);
	}
	else
	{
		digitalWrite(LED_pin, 1);
	}

	log_i("posielam CAN frame");
	twai_message_t message;
	message.identifier = 0x123;
	message.extd = 0;
	message.data_length_code = 2;
	message.rtr = false;
	for (int i = 0; i < 2; i++)
	{
		message.data[i] = 2;
	}

	// Queue message for transmission
	if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
	{
		log_i("Message queued for transmission\n");
	}
	else
	{
		log_i("Failed to queue message for transmission\n");
	}
}

void Loop_10sek(void)
{
	static u8_t loc_cnt_10sek = 0;
	// String sprava = String("\r\n[10sek Loop]  Mam Loop 10 sek....") + rtc.getDateTime(true);
	// Serial.println(sprava);

	{
		float testVal = 23.456f;
		float testVal2 = 34.567f;
		log_i("Float hodnoty 1: %f    2:%fP log I", testVal, testVal2);
	}

	// WiFi_connect_sequencer();
}

void OdosliCasDoWS(void)
{
	String DenvTyzdni = "! Čas nedostupný !";
	char loc_buf[60];
	DenvTyzdni = ConvetWeekDay_UStoSK(&MyRTC_cas);
	char hodiny[5];
	char minuty[5];
	strftime(loc_buf, sizeof(loc_buf), " %H:%M    %d.%m.%Y    ", &MyRTC_cas);

	ObjDatumCas["Cas"] = loc_buf + DenvTyzdni; // " 11:22 Stredaaaa";
	String jsonString = JSON.stringify(ObjDatumCas);
	Serial.print("[10sek] Odosielam strankam ws Cas:");
	Serial.println(jsonString);
	ws.textAll(jsonString);
}

void DebugMsgToWebSocket(String textik)
{
	if (LogEnebleWebPage == true)
	{
		String sprava = rtc.getTime("%H:%M:%S ");
		JSON_DebugMsg["DebugMsg"] = sprava + textik;
		String jsonString = JSON.stringify(JSON_DebugMsg);
		ws.textAll(jsonString);
	}
}

void FuncServer_On(void)
{
	server.on("/",
				 HTTP_GET,
				 [](AsyncWebServerRequest *request)
				 {
					 // if (!request->authenticate("ahoj", "xxxx"))
					 // return request->requestAuthentication();
					 // request->send_P(200, "text/html", index_html, processor);
					 request->send_P(200, "text/html", Main);
				 });

	server.on("/nastavip",
				 HTTP_GET,
				 [](AsyncWebServerRequest *request)
				 {
					 if (!request->authenticate("admin", "adum"))
						 return request->requestAuthentication();
					 request->send(200, "text/html", handle_Zadavanie_IP_setting());
				 });

	server.on("/Nastaveni",
				 HTTP_GET,
				 [](AsyncWebServerRequest *request)
				 {
					 handle_Nastaveni(request);
					 request->send(200, "text/html", "Nastavujem a ukladam do EEPROM");
					 Serial.println("Idem resetovat ESP");
					 delay(2000);
					 esp_restart();
				 });

	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request)
				 {
					 char ttt[500];
					 //u16_t citac = EEPROM.readUShort (EE_citacZapisuDoEEPORM);
					 //u16_t citac2 = EEPROM.readUShort (EE_citac2_ZapisuDoEEPORM);

					 char loc_buf[20];
					 char loc_buf1[60];
					 char loc_buf2[100];
					 if (Internet_CasDostupny == true)
					 {
						 sprintf(loc_buf, "dostupny :-)");
					 }
					 else
					 {
						 sprintf(loc_buf, "nedostupny!!");
					 }

					 if (RTC_cas_OK == false)
					 {
						 sprintf(loc_buf2, "[RTC_cas_OK == flase] RTC NE-maju realny cas!!. RTC hodnota: ");
					 }
					 else
					 {
						 sprintf(loc_buf2, "[RTC_cas_OK == true] RTC hodnota: ");
					 }
					 strftime(loc_buf1, sizeof(loc_buf1), " %H:%M:%S    %d.%m.%Y    ", &MyRTC_cas);

					 sprintf(ttt, "Firmware :%s<br>"
									  "Sila signalu WIFI(-30 je akoze OK):%i<br>"
									  "Internet cas: %s<br>"
									  "%s %s",
								firmware, WiFi.RSSI(), loc_buf, loc_buf2, loc_buf1);

					 request->send(200, "text/html", ttt); });

	server.on("/reset",
				 HTTP_GET,
				 [](AsyncWebServerRequest *request)
				 {
					 if (!request->authenticate("admin", "radecek78"))
						 return request->requestAuthentication();

					 request->send(200, "text/html", "resetujem!!!");
					 delay(1000);
					 esp_restart();
				 });

	server.on("/vytapeni",
				 HTTP_GET,
				 [](AsyncWebServerRequest *request)
				 {
					 request->send_P(200, "text/html", vytapeni);
				 });

	server.on("/zaluzie_Main",
				 HTTP_GET,
				 [](AsyncWebServerRequest *request)
				 {
					 request->send_P(200, "text/html", zaluzie_Main);
				 });
	server.on("/debug",
				 HTTP_GET,
				 [](AsyncWebServerRequest *request)
				 {
					 LogEnebleWebPage = true;
					 request->send_P(200, "text/html", DebugLog_html);
				 });
}

//***********************************************  Hepl function ********************************************/

void ESPinfo(void)
{
	esp_chip_info_t chip_info;
	esp_chip_info(&chip_info);
	Serial.println("\r\n*******************************************************************");
	Serial.print("ESP Board MAC Address:  ");
	Serial.println(WiFi.macAddress());
	Serial.println("\r\nHardware info");
	Serial.printf("%d cores Wifi %s%s\n",
					  chip_info.cores,
					  (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
					  (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
	Serial.printf("\r\nSilicon revision: %d\r\n ", chip_info.revision);
	Serial.printf("%dMB %s flash\r\n",
					  spi_flash_get_chip_size() / (1024 * 1024),
					  (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embeded" : "external");

	Serial.printf("\r\nTotal heap: %d\r\n", ESP.getHeapSize());
	Serial.printf("Free heap: %d\r\n", ESP.getFreeHeap());
	Serial.printf("Total PSRAM: %d\r\n", ESP.getPsramSize());
	Serial.printf("Free PSRAM: %d\r\n", ESP.getFreePsram()); // log_d("Free PSRAM: %d", ESP.getFreePsram());
	Serial.print("Alokujem buffer psdRamBuffer  500kB PSRAM");
	byte *psdRamBuffer = (byte *)ps_malloc(500000);
	Serial.printf(" -Free PSRAM: %d\r\n", ESP.getFreePsram());
	Serial.print("Uvolnujem buffer psdRamBuffer 500kz PSRAM ");
	free(psdRamBuffer);
	Serial.printf(" Free PSRAM po uvolneni : %d\r\n", ESP.getFreePsram()); // log_d("Free PSRAM: %d", ESP.getFreePsram());
	Serial.println("\r\n*******************************************************************");
}

void TWAI_RX_Task(void *arg)
{
	log_i("Spustam TWAI_RX_Task");

	log_i("Configure ESP32 CAN");
	twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_34, GPIO_NUM_33, TWAI_MODE_NORMAL);
	twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS();
	twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

	// Install TWAI driver
	if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK)
	{
		log_i("Driver installed");
	}
	else
	{
		log_i("Failed to install driver");
	}

	// Start TWAI driver
	if (twai_start() == ESP_OK)
	{
		log_i("Driver started");
	}
	else
	{
		log_i("Failed to start driver");
	}

	while (1)
	{
		twai_message_t message;
		if (twai_receive(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
		{
			log_i("Message received");
			// Process received message
			if (message.extd)
			{
				log_i("Message is in Extended Format");
			}
			else
			{
				log_i("Message is in Standard Format");
			}

			log_i("ID is %d", message.identifier);

			if (!(message.rtr))
			{
				for (int i = 0; i < message.data_length_code; i++)
				{
					log_i("Data byte %d = %d", i, message.data[i]);
				}
			}
		}
		else
		{
			//log_i("Failed to receive message");
		}

		
		delay(10);
	}
}