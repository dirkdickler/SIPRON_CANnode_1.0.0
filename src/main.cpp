
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
#include "MyBlinker.h"

// Replace with your network credentials
// const char* ssid = "Grabcovi";
// const char* password = "40177298";
const char *soft_ap_ssid = "SipronCAN xx:xx:xx:xx:xx:xx";
const char *soft_ap_password = "SipronCAN";
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
Ticker timer_100ms(Loop_100ms, 100, 0, MILLIS);
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

VSTUP_t DIN[pocetDIN];
VSTUP_t ADR[pocetADR];
VYSTUP_t DO[pocetDO];
SEMAFOR_t semafor;
u8 CANadresa = 255;
u8 Obraz_DIN = 0;
u8 Obraz_DO = 0;
twai_message_t messageSend;

static const uint32_t DESIRED_BIT_RATE = 125UL * 1000UL; // 125kb/s

char TX_BUF[TX_RX_MAX_BUF_SIZE];
bool Task_test_inProces = false;

LedBlinker led(LED_pin, COMMON_NEGATIVE);
LedBlinker kontrolka(DO8_pin, COMMON_NEGATIVE);
//------------------------------------------------------------------------------------------------------------------

/**********************************************************
 ***************            SETUP            **************
 **********************************************************/
void setup()
{
	Serial.begin(115200);
	Serial.println("        *********************************************************************************");
	Serial.println("        *                                                                               *");
	Serial.println("        *                            Spustam applikaciu                                 *");
	Serial.println("        *                                                                               *");
	Serial.println("        *********************************************************************************");
	System_init();

	ESPinfo();

	NacitajEEPROM_setting();

	timer_1ms.start();
	timer_10ms.start();
	timer_100ms.start();
	timer_1sek.start();
	timer_10sek.start();
	esp_task_wdt_init(WDT_TIMEOUT, true); // enable panic so ESP32 restarts
	esp_task_wdt_add(NULL);					  // add current thread to WDT watch

	// RS485 musis spustit az tu, lebo ak ju das hore a ESP ceka na konnect wifi, a pridu nejake data na RS485, tak FreeRTOS =RESET  asi overflow;
	// Serial1.begin(9600);

	kontrolka.blink(200 /* time on */,
						 200 /* time off */,
						 1 /* cycles */,
						 1000 /* pause between secuences */,
						 0 /* secuences */,
						 NULL /* function to call when finished */
	);

	xTaskCreatePinnedToCore(
		 TWAI_RX_Task,			// Task function
		 "task1",				// Name
		 6000,					// Stack size
		 nullptr,				// Parameters
		 1,						// Priority
		 &TWAI_RX_task_hndl, // handle
		 0							// CPU
	);

	// xTaskCreatePinnedToCore(
	// 	 TestovanieDosky_Task,		  // Task function
	// 	 "task2",						  // Name
	// 	 6000,							  // Stack size
	// 	 nullptr,						  // Parameters
	// 	 1,								  // Priority
	// 	 &TestovanieDosky_task_hndl, // handle
	// 	 0									  // CPU
	// );
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
	led.update();
	kontrolka.update();
}

void Loop_1ms()
{
	static bool lenRaz = false;

	if (0) // digitalRead(Boot_pin) == 0) // if (lenRaz == false)
	{
		log_i("DOBRE PUSTI IDEMP POSLAT TEN CAN MSG");
		delay(1000);
		lenRaz = true;
		for (u16 i = 0; i < 1; i++)
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
			if (twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
			{
				log_i("CAN send OK for transmission\n");
			}
			else
			{
				log_i("Failed to queue message for transmission\n");
				{
					twai_clear_transmit_queue();
					twai_initiate_recovery();
					twai_stop();
					twai_start();
				}
			} /* code */
		}
	}

	if (myTimer.CAN_prima_data)
	{
		if (--myTimer.CAN_prima_data == 0)
		{
			kontrolka.blink(200 /* time on */,
								 200 /* time off */,
								 1 /* cycles */,
								 1000 /* pause between secuences */,
								 0 /* secuences */,
								 NULL /* function to call when finished */
			);

			{
				twai_clear_transmit_queue();
				twai_initiate_recovery();
				twai_stop();
				twai_start();
			}
			for (u8 i = 0; i < pocetDO; i++)
			{
				DO[i].output = false;
			}
		}
	}

	if (myTimer.timeToSendCANReply)
	{
		if (--myTimer.timeToSendCANReply == 0)
		{
			if (twai_transmit(&messageSend, pdMS_TO_TICKS(1000)) == ESP_OK)
			{
				log_i("Message queued for transmission");
			}
			else
			{
				log_i("Failed to queue message for transmission\n");
			}
		}
	}
}

void Loop_10ms()
{
	Obraz_DIN = ScanInputs();
	CANadresa = Read_DIPAdress();
	// for (int i = 0; i < pocetDIN; i++)
	// {
	// 	if (DIN[i].input == true)
	// 		DO[i].output = true;
	// }
	Obraz_DO = Output_Handler();

	if (digitalRead(Boot_pin) == 0)
	{
		if (myTimer.Wifi_ON_timeout == 0 && myTimer.Wifi_zapsi_za_X_sekund == 0)
		{
			led.blink(200 /* time on */,
						 200 /* time off */,
						 3 /* cycles */,
						 1000 /* pause between secuences */,
						 0 /* secuences */,
						 NULL /* function to call when finished */
			);

			log_i("!!!!  Zapinam WIFI !!! ");
			myTimer.Wifi_zapsi_za_X_sekund = 3;
		}
	}
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
	// if ( flg.Wifi_zapnuta == true) { LEDblinker();}
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

	if (myTimer.CAN_reinitDriver)
	{
		if (--myTimer.CAN_reinitDriver == 0)
		{
			twai_clear_transmit_queue();
				twai_initiate_recovery();
				twai_stop();
				twai_start();
				myTimer.CAN_reinitDriver = 10;
		}
	}

	if (myTimer.Wifi_ON_timeout)
	{
		if (digitalRead(Boot_pin) != 0) // ak je jumper close, tak neodpocitavam cas
		{
			if (--myTimer.Wifi_ON_timeout == 0)
			{
				log_i("Ubehol cas zapnutia Wifi - vypinam Wifinu");
				// WiFi.softAPdisconnect(false);
				// delay(1000);
				WiFi.enableAP(false);
				// WiFi.disconnect(true);
				flg.Wifi_zapnuta = false; //

				led.blink(200 /* time on */,
							 200 /* time off */,
							 1 /* cycles */,
							 1000 /* pause between secuences */,
							 0 /* secuences */,
							 NULL /* function to call when finished */
				);
			}
		}
	}

	if (myTimer.Wifi_zapsi_za_X_sekund)
	{
		//"sequencer"
		if (--myTimer.Wifi_zapsi_za_X_sekund == 0)
		{
			WiFi.enableAP(true);
			myTimer.Wifi_ON_timeout = 60 * 10; // sekund
		}

		// po 10 sek od zapnutia nahodim "inicializujem Wifi", lebo na zaciatku som mal nastavene Wifi_zapsi_za_X_sekund = 50;
		if (myTimer.Wifi_zapsi_za_X_sekund == 40)
		{
			Serial.println("Davam WiFI init..");
			myTimer.Wifi_zapsi_za_X_sekund = 0;
			myTimer.Wifi_ON_timeout = 10; // po inicializacii ju necham zapnutu na 10 sekund
			WiFi_init(FirstInit);
			flg.Wifi_zapnuta = true;
			led.blink(200 /* time on */,
						 200 /* time off */,
						 3 /* cycles */,
						 1000 /* pause between secuences */,
						 0 /* secuences */,
						 NULL /* function to call when finished */
			);
		}
	}
	// if ( flg.Wifi_zapnuta == false) { LEDblinker();}

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

	// // Queue message for transmission
	// if ( twai_transmit(&message, pdMS_TO_TICKS(1000)) == ESP_OK)
	// {
	// 	log_i("Message queued for transmission");
	// }
	// else
	// {
	// 	 log_i("Failed to queue message for transmission\n");
	// }

	float flt = (float)ESP.getFreeHeap();
	flt /= 1000.0f;
	char locBuf[50];
	sprintf(locBuf, "%.3f", flt);
	log_i("HEAP free:%s - CAN adresa: %u - Vstupy: %u - Vystupy: %u", locBuf, CANadresa, Obraz_DIN, Obraz_DO);
	Serial.printf("HEAP free:%s - CAN adresa: %u - Vstupy: %u - Vystupy: %u\r\n", locBuf, CANadresa, Obraz_DIN, Obraz_DO);

	// String rr = "[1sek Loop] signal: " + (String)WiFi.RSSI() + "dBm" +
	//				"  IN: " + Obraz_DIN + "  OUT: " + Obraz_DO + "  Wifi OFF za: " + myTimer.Wifi_ON_timeout + "\r\n ";

	twai_status_info_t CANstatus_info;
	twai_get_status_info(&CANstatus_info);
	Serial.printf("CAN Tx err cnt :%lu\r\nmsg to TX :%lu\r\ntx failed :%lu\r\nbus err cnt :%lu\r\n",
					  CANstatus_info.tx_error_counter, CANstatus_info.msgs_to_tx, CANstatus_info.tx_failed_count, CANstatus_info.bus_error_count);

	String rr = "[1sek Loop]  IN: " + (String)Obraz_DIN + "  OUT: " + (String)Obraz_DO +
					"  Wifi OFF za: " + (String)myTimer.Wifi_ON_timeout +
					"\r\nCAN Tx err cnt : " + (String)CANstatus_info.tx_error_counter +
					"\r\nCAN msg to TX  : " + (String)CANstatus_info.msgs_to_tx +
					"\r\nCAN tx failed  : " + (String)CANstatus_info.tx_failed_count +
					"\r\nCAN bus err cnt  : " + (String)CANstatus_info.bus_error_count +
					"\r\n ";
	DebugMsgToWebSocket(rr);
}

void Loop_10sek(void)
{
	Serial.println("-mam 10 sek....");
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
		JSONVar locObj;
		float flt = (float)ESP.getFreeHeap();
		flt /= 1000.0f;
		locObj["HeapFree"] = flt;
		locObj["CANadresa"] = CANadresa;
		locObj["Firmware"] = firmware;
		locObj["DebugMsg"] = textik; // + sprava;
		String jsonString = JSON.stringify(locObj);
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
	server.on("/IOpage",
				 HTTP_GET,
				 [](AsyncWebServerRequest *request)
				 {
					 // if (!request->authenticate("ahoj", "xxxx"))
					 // return request->requestAuthentication();
					 // request->send_P(200, "text/html", index_html, processor);
					 request->send_P(200, "text/html", IOpage);
				 });

	server.on("/nastavip",
				 HTTP_GET,
				 [](AsyncWebServerRequest *request)
				 {
					 if (!request->authenticate("admin", "SipronCAN"))
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
					 if (!request->authenticate("admin", "SipronCAN"))
						 return request->requestAuthentication();

					 request->send(200, "text/html", "resetujem!!!");
					 delay(1000);
					 esp_restart();
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
	Serial.println("\r\\r\n*******************************************************************\r\n");
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
	Serial.print("ESP32 SDK: ");
	Serial.println(ESP.getSdkVersion());
	Serial.println("\r\n*******************************************************************\r\n");
}

void TWAI_RX_Task(void *arg)
{
	log_i("Spustam TWAI_RX_Task");

	log_i("Configure ESP32 CAN");
	twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_33, GPIO_NUM_34, TWAI_MODE_NORMAL);
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

			u32 locadresa = message.identifier >> 4; //
			log_i("CAN adresa je %d", locadresa);
			u32 opCode = message.identifier & 0b00000001111;
			log_i("OpCode  je %d", opCode);

			if (locadresa == 0 || locadresa == CANadresa)
			{
				if (opCode == 0 && message.rtr == 0) // nahodit Vystupy
				{
					if (myTimer.CAN_prima_data == 0)
					{
						kontrolka.blink(200 /* time on */,
											 200 /* time off */,
											 2 /* cycles */,
											 1000 /* pause between secuences */,
											 0 /* secuences */,
											 NULL /* function to call when finished */
						);
					}

					myTimer.CAN_prima_data = CAN_Rx_timeout;
					myTimer.CAN_reinitDriver = 10;

					for (u8 i = 0; i < pocetDO; i++)
					{
						if (isbit(message.data[0], i))
						{
							DO[i].output = true;
						}
						else
						{
							DO[i].output = false;
						}
					}

					if (message.data_length_code == 2 && message.data[1] == 1)
					{
						opCode = 1; // nastavym na tvrdo ze ma poslat odpoved po nastaveni OUT  hned tu dole nasledne
						message.data_length_code = 0;
						message.rtr = 1;
					}
				}

				if (opCode == 1 && message.data_length_code == 0 && message.rtr == 1) // vrat vstupy a vystupy
				{
					log_i("posielam CAN frame");

					messageSend.identifier = CANadresa;
					messageSend.identifier <<= 4;
					messageSend.identifier += opCode;
					messageSend.extd = 0;
					messageSend.data_length_code = 2;
					messageSend.rtr = false;
					messageSend.data[0] = Obraz_DIN;
					messageSend.data[1] = Obraz_DO;

					if (locadresa == CANadresa)
					{
						myTimer.timeToSendCANReply = 1; // odosli okamzite
					}
					else if (locadresa == 0) // odosli 2ms* CANadresa lebo odpovedaju vsetky na broadcast
					{
						myTimer.timeToSendCANReply = (2 * CANadresa);
					}
				}
			}

			if ((message.rtr))
			{
				log_i("Message ma RTR");
			}
			else
			{
				log_i("Message NEma RTR");
			}

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
		if (0) // semafor.Task_test_inProces == true)
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