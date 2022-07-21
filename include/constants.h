#ifndef INC_CONSTANST_H
#define INC_CONSTANST_H

#include <Arduino.h>
#include "define.h"
#include <ESPAsyncWebServer.h>

static const char deviceName[31] = {"Zapsi Eng FW 1                "};
static const char MCU_type[9] = {"F412RE"};

static const char path_DI_Lock_txt[] = {"log/di_lock.txt"};
static const char path_digital_txt[] = {"log/digital.txt"};
static const char path_Digital_txt[] = {"LOG/digital.txt"};
static const char path_analog_txt[] = {"log/analog.txt"};
static const char path_Analog_txt[] = {"LOG/analog.txt"};
static const char path_rs232_txt[] = {"log/rs232.txt"};
static const char path_power_txt[] = {"log/UI_value.txt"};
static const char path_Power_txt[] = {"Log/UI_value.txt"};

typedef int32_t i32;
typedef int16_t i16;
typedef int8_t i8;

typedef const int32_t sc32; /*!< Read Only */
typedef const int16_t sc16; /*!< Read Only */
typedef const int8_t sc8;	 /*!< Read Only */

typedef volatile int32_t vs32;
typedef volatile int16_t vs16;
typedef volatile int8_t vs8;

typedef volatile const int32_t vsc32; /*!< Read Only */
typedef volatile const int16_t vsc16; /*!< Read Only */
typedef volatile const int8_t vsc8;	  /*!< Read Only */

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;

typedef const uint32_t uc32; /*!< Read Only */
typedef const uint16_t uc16; /*!< Read Only */
typedef const uint8_t uc8;	  /*!< Read Only */

typedef volatile uint32_t vu32;
typedef volatile uint16_t vu16;
typedef volatile uint8_t vu8;

typedef volatile const uint32_t vuc32; /*!< Read Only */
typedef volatile const uint16_t vuc16; /*!< Read Only */
typedef volatile const uint8_t vuc8;	/*!< Read Only */

//definovani ID  sprav
#define MsgID_Ping 1
#define MsgID_AllDataSpace 2
#define MsgID_SingleNode 3
#define MsgID_NameraneHodnoty 4

#define MsgID_PublicNazvyMistnosti 0xff00
#define MsgID_PublicHodnoty 0xff01
#define MsgID_GoToBoot 0xff02
#define MsgID_WriteToFlashAdress 0xff03
#define MsgID_ExitFromBoot 0xff04

#define broadcast 0xFFFF

#define Preamble1 0x02
#define Frame 0x43

#define CMD_bit_RW 7
#define CMD_read 0
#define CMD_write 0x80

#define CMD_Ack_OK 0
#define CMD_Ack_Busy 1
#define CMD_Ack_NOK 2
#define CMD_Reply 0x20
#define CMD_Event 0x40
#define CMD_RW 0x80

//ACK_NOK
#define NOKcode_countOverflow 1 //vela dat v pakete
#define NOKcode_sumaError 2	  //nesedi suma
#define NOKcode_missingData 3	  //chynaju data v pakete
#define NOKcode_naznamyMSGID 4  //

typedef struct
{
	u8 typVstupu; //klasika,citac,ci ma automaticky padat fo nule
	u16 pin;
	bool input;
	u8 input_prew;
	u16 filter;
	u32 counter;
	u16 conter_filter;
	bool zmena;
} VSTUP_t;

typedef struct
{
	uint16_t pin;
	bool output;
	uint8_t output_prew;
	uint16_t timeout_ON;
	uint16_t timeout_OFF;
}VYSTUP_t;
typedef struct //
{  
	bool posti_Wifi;
	bool Wifi_zapnuta;
	bool posielajDataIOpageNaZmenuIO;
} FLAGS_t;

typedef struct //
{
	bool Task_test_inProces;
	
} SEMAFOR_t;

typedef struct //
{
	u16 Wifi_ON_timeout;
	u8  Wifi_zapsi_za_X_sekund;
	u16 timeToSendCANReply;
	u16 CAN_prima_data;
	u8 CAN_reinitDriver;
}TIMERS_t;

typedef struct //
{  
	u8 pinLedky;
	u16 time_ON;
	u16 time_OFF;
	u8 pocetBkliknuti; //cycles;
	i16 pauzaMedziOpakovaniami;
	u16 pocetOpakovani;
	void(*PTRnaFunkciu());
}LED_INDICATOR_t;

extern SEMAFOR_t semafor;
extern FLAGS_t flg;
extern TIMERS_t myTimer;

#endif
