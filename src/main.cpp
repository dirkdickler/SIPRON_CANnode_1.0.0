
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
//#include <TimeLib.h>
//#include <Wire.h>
//#include "pcf8563.h"
#include "index.html"
#include "main.h"
#include "define.h"
#include "constants.h"
#include "HelpFunction.h"
#include "Pin_assigment.h"
#include "esp_log.h"

//#include <ACAN_ESP32.h>
#include "driver/twai.h"

// Replace with your network credentials
// const char* ssid = "Grabcovi";
// const char* password = "40177298";
const char *soft_ap_ssid = "SipronCAN xx:xx:xx:xx:xx:xx";
const char *soft_ap_password = "Sipron";
// const char *ssid = "semiart";
// const char *password = "aabbccddff";
char NazovSiete[30];
char Heslo[30];

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

JSONVar myObject, myObject2, JSON_DebugMsg;
Ticker timer_1ms(Loop_1ms, 1, 0, MILLIS);
Ticker timer_10ms(Loop_10ms, 10, 0, MILLIS);
Ticker timer_100ms(Loop_100ms, 300, 0, MILLIS);
Ticker timer_1sek(Loop_1sek, 1000, 0, MILLIS);
Ticker timer_10sek(Loop_10sek, 10000, 0, MILLIS);

IPAddress local_IP(192, 168, 1, 14);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);	// optional
IPAddress secondaryDNS(8, 8, 4, 4); // optional

TaskHandle_t TWAI_RX_task_hndl;
TaskHandle_t TestovanieDosky_task_hndl;

char gloBuff[200];
bool LogEnebleWebPage = false;
FLAGS_t flg;
LOGBUFF_t LogBuffer;

VSTUP_t DIN[pocetDIN];
VSTUP_t ADR[pocetADR];
VYSTUP_t DO[pocetDO];
SEMAFOR_t semafor;
u8 CANadresa = 0;
u8 Obraz_DIN = 0;
u8 Obraz_DO = 0;

static const uint32_t DESIRED_BIT_RATE = 125UL * 1000UL; // 125kb/s

char TX_BUF[TX_RX_MAX_BUF_SIZE];
bool Task_test_inProces = false;

//------------------------------------------------------------------------------------------------------------------

/**********************************************************
 ***************        SETUP         **************
 **********************************************************/

void setup()
{
	Serial.begin(115200);
	Serial.println("        *********************************************************************************");
	Serial.println("        *                                                                               *");
	Serial.println("        *                            Spustam applikaciu 1                               *");
	Serial.println("        *                                                                               *");
	Serial.println("        *********************************************************************************");
	System_init();

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

	WiFi_init(); // este si odkomentuj  //WiFi_connect_sequencer(); v 10 sek loop
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

	xTaskCreatePinnedToCore(
		 TestovanieDosky_Task,		  // Task function
		 "task2",						  // Name
		 6000,							  // Stack size
		 nullptr,						  // Parameters
		 1,								  // Priority
		 &TestovanieDosky_task_hndl, // handle
		 0									  // CPU
	);
}

void loop()
{
	esp_task_wdt_reset();
	ws.cleanupClients();
	timer_1ms.update();
	timer_10ms.update();
	timer_100ms.update();
	timer_1sek.update();
	timer_10sek.update();
}

void Loop_1ms()
{
	static bool lenRaz = false;

	if (lenRaz == false)
	{
		delay(1000);
		lenRaz = true;
		for (u16 i = 0; i < 100; i++)
		{
			twai_message_t message;
			message.identifier = 345;
			message.extd = 0;
			message.data_length_code = 1;
			message.rtr = false;
			for (int i = 0; i < 1; i++)
			{
				message.data[i] = 5;
			}

			// Queue message for transmission
			if (0) // twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
			{
			}
			else
			{
				//	log_i("Failed to queue message for transmission\n");
			} /* code */
		}
	}
}

void Loop_10ms()
{
	Obraz_DIN = ScanInputs();
	CANadresa = Read_DIPAdress();
	for (int i = 0; i <pocetDIN;i++){
		DO[i].output = DIN[i].input;
	}
	
	Obraz_DO = Output_Handler();
}

void Loop_100ms(void)
{
	twai_message_t message;
	message.identifier = 0x333;
	message.extd = 0;
	message.data_length_code = 8;
	message.rtr = false;
	for (int i = 0; i < 8; i++)
	{
		message.data[i] = 4;
	}

	// Queue message for transmission
	if (0) // twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
	{
	}
	else
	{
		// log_i("Failed to queue message for transmission\n");
	}
}

void Loop_1sek(void)
{
	static bool LEDka = false;
	// log_i("[1sek Loop]  mam 1 sek....  ");
	String sprava; // = rtc.getTime("\r\n[%H:%M:%S] karta a toto cas z PCF8563:");
						// unsigned long start = micros();
						// sprava += PCFrtc.formatDateTime(PCF_TIMEFORMAT_YYYY_MM_DD_H_M_S);
						// unsigned long end = micros();
						// unsigned long delta = end - start;
						// Serial.print("DELTA PCF8563: ");
						// Serial.println(delta);

	log_i("Takto vidim LED ku %u", digitalRead(LED_pin));
	log_i("Takto vidim DO8  %u", digitalRead(DO8_pin));
	if (LEDka == false) // digitalRead(LED_pin) == 0)
	{
		LEDka = true;
		digitalWrite(LED_pin, 1);
		log_i("LED davam na 1");
	}
	else
	{
		LEDka = false;
		digitalWrite(LED_pin, 0);
		log_i("LED davam na 0");
	}

	// log_i("posielam CAN frame");
	// twai_message_t message;
	// message.identifier = 0x123;
	// message.extd = 0;
	// message.data_length_code = 2;
	// message.rtr = false;
	// for (int i = 0; i < 2; i++)
	// {
	// 	message.data[i] = 2;
	// }

	// Queue message for transmission
	if (0) // twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
	{
		log_i("Message queued for transmission");
	}
	else
	{
		// log_i("Failed to queue message for transmission\n");
	}

	float flt = (float)ESP.getFreeHeap();
	flt /= 1000.0f;
	char locBuf[50];
	sprintf(locBuf, "%.3f", flt);
	log_i("HEAP free:%s - CAN adresa: %u - Vstupy: %u - Vystupy: %u", locBuf, CANadresa, Obraz_DIN, Obraz_DO);

	String rr = "[1sek Loop] signalu: " + (String)WiFi.RSSI() + "dBm  a Heap: " + locBuf + " kB " +
					" CAN adr: " + CANadresa + "  Vstupy: " + Obraz_DIN + "  Vystupy: " + Obraz_DO + "\r\n ";

	DebugMsgToWebSocket(rr);
}

void Loop_10sek(void)
{
	log_i("-mam 10 sek....");
	static u8_t loc_cnt_10sek = 0;
	// String sprava = String("\r\n[10sek Loop]  Mam Loop 10 sek....") + rtc.getDateTime(true);
	// Serial.println(sprava);

	{
		// float testVal = 23.456f;
		// float testVal2 = 34.567f;
		// log_i("Float hodnoty 1: %f    2:%f ", testVal, testVal2);
	}

	// WiFi_connect_sequencer();
}

void OdosliCasDoWS(void)
{
	JSONVar ObjDatumCas;
	ObjDatumCas["Cas"] = "--:--";
	String jsonString = JSON.stringify(ObjDatumCas);
	Serial.print("[10sek] Odosielam strankam ws Cas:");
	Serial.println(jsonString);
	ws.textAll(jsonString);
}

void DebugMsgToWebSocket(String textik)
{
	if (LogEnebleWebPage == true)
	{
		// String sprava = "--:--"; //rtc.getTime("%H:%M:%S ");
		JSON_DebugMsg["DebugMsg"] = textik; // + sprava;
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
					 

					 snprintf(ttt, sizeof(ttt),"Firmware :%s<br>"
									  "Sila signalu WIFI(-30 je akoze OK):%i<br>",
									  								firmware, WiFi.RSSI());

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
	Serial.print("APB CLOCK: ");
	Serial.print(APB_CLK_FREQ);
	Serial.println(" Hz");
	Serial.printf("%d cores Wifi %s%s\n",
					  chip_info.cores,
					  (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
					  (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
	Serial.printf("\r\nESP32 Chip Revision: %d\r\n ", chip_info.revision);
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
	Serial.print("ESP32 SDK: ");
	Serial.println(ESP.getSdkVersion());
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
			// log_i("Failed to receive message");
		}

		delay(10);
	}
}

void TestovanieDosky_Task(void *arg)
{
	delay(3000);
	log_i("Spustam Task Testovanie Dosky");
	u8 Error1 = 0;

	semafor.Task_test_inProces = true;
	while (1)
	{
		if (semafor.Task_test_inProces == true)
		{
			// zhodim vystupy
			log_i("Zhozdujem vsetky vystupy");
			for (int i = 0; i < pocetDIN; i++)
			{
				DO[i].output = false;
			}

			Output_Handler();
			delay(100);

			for (int i = 0; i < pocetDIN; i++)
			{
				// na zacatku musia byt ALL IN  = 0
				if (DIN[i].input == true)
				{
					sbi(Error1, i);
				}
			}

			if (Error1)
			{
				log_i("!!! Problem tu musia byt vystupy OFF< ale ich obraz je:%u", Error1);
			}
			else
			{
				log_i("JE to dobre vsetky vystupy si OFF");
			}

			log_i("Idem nahodit vystup jeden po druhem a otestovat ci sa danny vstup nahodi");
			for (int i = 0; i < pocetDIN; i++)
			{
				DO[i].output = true;
				Output_Handler();
				delay(50);
				if (DIN[i].input == false)
				{
					log_i("Nahadzujem vystup %u, ale jeho DIN je FALSE !!!  chyba ", i);
				}
				else
				{
					log_i("Nahadzujem vystup %u, a jeho DIN je TRUE  supeer to je OK", i);
				}
			}

			log_i("Konec testu davam Kill tasku, ");
			semafor.Task_test_inProces = false;
			vTaskDelete(TestovanieDosky_task_hndl);
		}
		// log_i("Loop Task Testovanie Dosky");
		delay(100);
	}
}