#ifndef __DEFINE_H
#define __DEFINE_H


#define HI(ii) ( ii >> 8)
#define LO(ii) ( ii & 0xff)
#define sbi(adr,bitt) (adr|=(((uint64_t)1)<<bitt))
#define cbi(adr,bitt) (adr&=~(((uint64_t)1)<<bitt))
#define isbit(adr,bitt) (adr &(((uint64_t)1)<<bitt))

#define  TX_RX_MAX_BUF_SIZE 4096
#define TCP_10001_socket 3
#define FirstInit false
#define Re_Init true


//Typy zaznamov zaznamov
#define IDzaznamu_SCT_prud 1
#define IDzaznamu_IN1 2
#define IDzaznamu_IN2 3
#define IDzaznamu_IN3 4
#define IDzaznamu_IN4 5
#define IDzaznamu_SCT_test 6



 

#define input1 0
#define input2 1
#define input3 2
#define input4 3
#define input5 4
#define input6 5
#define input7 6
#define input8 7
#define output1 0
#define output2 1
#define output3 2
#define output4 3
#define output5 4
#define output6 5
#define output7 6
#define output8 7


#define pocetDIN 8
#define pocetDO 7
#define filterTime_DI 3  //10ms loop
#define pocetADR 7
#define WDT_TIMEOUT 5
#define firmware "ver202207021_1"
#define CAN_Rx_timeout 5000; //ms


//EEPROM adrese
#define EE_IPadresa 00				//16bytes
#define EE_SUBNET 16				//16bytes
#define EE_Brana 32					//16bytes
#define EE_NazovSiete 48			//16bytes
#define EE_Heslosiete 64			//20bytes
#define EE_citacZapisuDoEEPORM 84   //2bytes
#define EE_citac2_ZapisuDoEEPORM 86   //2bytes
#define EE_dalsi 88
#define EE_zacateKaret_1 200
#define EE_zacateKaret_2 1300   //EE_zacateKaret + 100*11tj 1300


#endif 