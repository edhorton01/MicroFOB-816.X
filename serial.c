#include "main.h"
#include "serial.h"
#include "si241.h"
#include "mcc_generated_files/mcc.h"

extern SerialSt SerialWk;
extern uint8_t SBuf_in[16];
extern uint8_t SBuf_inbc;
extern uint8_t SBuf_out[16];
extern uint8_t SBuf_outbc;
extern uint8_t SBuf_infe;
extern Hexcon hexc;
extern uint8_t Device_ID[10];
extern uint8_t tx_pipe;
extern uint8_t test_channel;

extern uint16_t si24_on_timer;

extern volatile uint8_t Si24_Status;
extern volatile uint8_t Serial_Cmd;

void SerialIn_Hand(void)
{
    if(!SerialWk._rx_inprog)
    {
        IO_PC0_TP4_SetHigh();
        SerialWk._flags = 0;
        SerialWk._ser_rx_sc = 0;
        SerialWk._ser_rx_bc = 0;
        SerialWk._ser_rx_int = 0;
        TCB0_WriteTimer(0);
        SerialWk._rx_inprog = 1;
        TCB0.CTRLA = 0x41;
    }
}

bool SerialOut_Start(uint8_t ser_work)
{
    bool res;
    res = false;
    if(!SerialWk._tx_inprog)
    {
        SerialWk._ser_tx_sc = 0;
        SerialWk._ser_tx_bc = 0;
        SerialWk._tx_inprog = 1;
        SerialWk._ser_out = ser_work;
        TCB0.CTRLA = 0x41;
        res = true;
    }
    return res;
}

void SendSerial_Buf(void)
{
    uint8_t by;
    bool res;
    if(SerialWk._buf_tx_inprog || SerialWk._buf_tx_req)
    {
        if(SerialWk._buf_tx_req)
        {
            SerialWk._buf_tx_req = 0;
            SerialWk._buf_tx_inprog = 1;
            SerialWk._buf_tx_ccnt = 0;
        }
        by = SBuf_out[SerialWk._buf_tx_ccnt];
        res = SerialOut_Start(by);
        if(res)
        {
            SerialWk._buf_tx_ccnt++;
            if(SerialWk._buf_tx_ccnt >= SerialWk._buf_tx_bcnt)
            {
                SerialWk._buf_tx_inprog = 0;
            }
        }
    }
}

void TimerB_Hand(void)
{
    uint8_t bv;
    
    if(SerialWk._rx_inprog)
    {
        IO_PC0_TP4_Toggle();
        if(IO_PC3_TP1_GetValue())
        {
            SerialWk._ser_rx_int++;
        }
        
        SerialWk._ser_rx_sc++;
        if(SerialWk._ser_rx_sc >= 8)
        {
            SerialWk._ser_rx_sc = 0;
            if(SerialWk._ser_rx_int >= 6) { bv = 0x80; } 
            else if(SerialWk._ser_rx_int <= 2) { bv = 0; }
            else {
                bv = 0;
                SerialWk._frame_err = 1;
            }
            SerialWk._ser_rx_int = 0;
            if(SerialWk._ser_rx_bc == 0)
            {
                if(bv)
                {
                    SerialWk._frame_err = 1;
                    SerialWk._rx_inprog = 0;
                }
                else
                {
                    SerialWk._ser_rx_bc++;
                }
            }
            else if(SerialWk._ser_rx_bc == 9)
            {
                if(bv)
                {
                    SerialWk._frame_err = 0;
                }
                else
                {
                    SerialWk._frame_err = 1;
                }
                if(SerialWk._frame_err) { SBuf_infe++; }
                SBuf_in[SBuf_inbc++] = SerialWk._ser_in;
                SerialWk._rx_inprog = 0;
                
                SerialOut_Start(SerialWk._ser_in);
                
                if(!SerialWk._tx_inprog && !SerialWk._buf_tx_req && !SerialWk._buf_tx_inprog) {TCB0.CTRLA = 0x40;}
                SBuf_inbc = SBuf_inbc & 0x0f;
                Serial_Cmd = 0x80;
            }
            else
            {
                SerialWk._ser_rx_bc++; 
                SerialWk._ser_in = (SerialWk._ser_in >> 1) + bv;
                if(SerialWk._ser_rx_bc == 9) {SerialWk._ser_rx_sc = 01;}
            }            
        }
    }
    
    else if(SerialWk._tx_inprog)
    {
        if((SerialWk._ser_tx_sc == 0) && (SerialWk._ser_tx_bc == 0))
        {
             IO_PC2_TP2_SetLow();
             SerialWk._ser_tx_sc++;
        }
        if((SerialWk._ser_tx_sc == 0) && (SerialWk._ser_tx_bc == 9))
        {
             IO_PC2_TP2_SetHigh();
             SerialWk._ser_tx_sc++;
        }
        else
        {
            SerialWk._ser_tx_sc++;
            if(SerialWk._ser_tx_sc >= 8)
            {
                SerialWk._ser_tx_sc = 0;
                SerialWk._ser_tx_bc++;
                if(SerialWk._ser_tx_bc == 10)
                {
                    SerialWk._tx_inprog = 0;
                    if(!SerialWk._rx_inprog && !SerialWk._buf_tx_req && !SerialWk._buf_tx_inprog) {TCB0.CTRLA = 0x40;}                    
                }
                else
                {                  
                    if(SerialWk._ser_out & 0x01)
                    {
                        IO_PC2_TP2_SetHigh();
                    }
                    else
                    {
                        IO_PC2_TP2_SetLow();                    
                    }
                    SerialWk._ser_out = SerialWk._ser_out >> 1;
                }
            }
        }        
    }
    SendSerial_Buf();
}

Hexcon Hex2Asc(uint8_t asc_work)
{
    uint8_t work;
    Hexcon res;
    
    work = ((asc_work >> 4) & 0x0f) + 0x30;
    if (work >= 0x3a)
    {
        work = work + 7;
    }
    res._ms = work;
    work = (asc_work & 0x0f) + 0x30;
    if (work >= 0x3a)
    {
        work = work + 7;
    }
    res._ls = work;
    return res;
}

void SendDevId(void)
{
    uint8_t i, j;
    
    j = 0;
    i = 0;
    SBuf_out[j++] = 0x0d;
    while (i < 5)
    {
        hexc = Hex2Asc(Device_ID[i++]);
        SBuf_out[j++] = hexc._ms;
        SBuf_out[j++] = hexc._ls; 
    }
    SBuf_out[j++] = 0x0d;
    SBuf_out[j++] = 0x0a;

    SerialWk._buf_tx_bcnt = j;
}

void ParseRx(uint8_t last_ch)
{
    uint8_t work;
    if(last_ch == 0x0a)
    {
        asm ("nop");
        asm ("nop");
        asm ("nop");                     

        if(SBuf_inbc == 2)
        {
            SBuf_inbc--;
            work = SBuf_in[--SBuf_inbc];
            switch (work)
            {
            case 'S':
            case 's':
            {
                asm ("nop");
                asm ("nop");
                asm ("nop");   
                SendDevId();
                SerialWk._buf_tx_req = 1;
                TCB0.CTRLA = 0x41;                
                break;
            }

            case 'T':
            case 't':
            {
                asm ("nop");
                asm ("nop");
                asm ("nop");
                SBuf_out[0] = 0x0d;
                SerialWk._buf_tx_bcnt = 1;
                SerialWk._buf_tx_req = 1;
                TCB0.CTRLA = 0x41;                                

                SerialWk._tx_on = 1;
                test_channel = 0x40;
                tx_pipe = 0;
                SI241_SetupTx();
                SI241_SetTx();
                si24_on_timer = 1000;                // set to 1000 * 0.01 = 10 Seconds 
                break;
            }

            case 'F':
            case 'f':
            {
                asm ("nop");
                asm ("nop");
                asm ("nop");
                SBuf_out[0] = 0x0d;
                SerialWk._buf_tx_bcnt = 1;
                SerialWk._buf_tx_req = 1;
                TCB0.CTRLA = 0x41;                                

                SerialWk._tx_on = 0;
                SI241_PwrOff();
                si24_on_timer = 0;
                break;
            }

            }
        }    
        SBuf_inbc = 0;
    }
}