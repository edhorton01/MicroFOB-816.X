#include "mcc_generated_files/utils/compiler.h"
    
void SerialIn_Hand(void);
bool SerialOut_Start(uint8_t ser_work);
void SendSerial_Buf(void);
void TimerB_Hand(void);
Hexcon Hex2Asc(uint8_t asc_work);
void SendDevId(void);
void ParseRx(uint8_t last_ch);