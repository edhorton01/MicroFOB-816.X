/*
    (c) 2018 Microchip Technology Inc. and its subsidiaries. 
    
    Subject to your compliance with these terms, you may use Microchip software and any 
    derivatives exclusively with Microchip products. It is your responsibility to comply with third party 
    license terms applicable to your use of third party software (including open source software) that 
    may accompany Microchip software.
    
    THIS SOFTWARE IS SUPPLIED BY MICROCHIP "AS IS". NO WARRANTIES, WHETHER 
    EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS SOFTWARE, INCLUDING ANY 
    IMPLIED WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY, AND FITNESS 
    FOR A PARTICULAR PURPOSE.
    
    IN NO EVENT WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY KIND 
    WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF MICROCHIP 
    HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE FORESEEABLE. TO 
    THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP'S TOTAL LIABILITY ON ALL 
    CLAIMS IN ANY WAY RELATED TO THIS SOFTWARE WILL NOT EXCEED THE AMOUNT 
    OF FEES, IF ANY, THAT YOU HAVE PAID DIRECTLY TO MICROCHIP FOR THIS 
    SOFTWARE.
*/

#include "mcc_generated_files/mcc.h"
#include "main.h"
#include "key_handler.h"
#include "si241.h"
#include "serial.h"

uint16_t timer0_val;
uint16_t si24_on_timer;

volatile KEYstateBITS KeyStatus;
volatile KEYstateControl Key;
volatile INTstateBITS IntStatus;
volatile TmrDelay TimerD;
volatile uint8_t Si24_Status;
volatile uint8_t PORTA_img;

volatile uint8_t Serial_Cmd;

uint8_t work1;
uint8_t go_tx;
uint8_t tx_pipe;
uint8_t SPI_Bout[16];
uint8_t SPI_Bin[16];
uint8_t RX_Payload[16];
uint8_t TX_Payload[16];
uint8_t RX_PL_Len;
uint8_t TX_PL_Len;
uint8_t TX_Address[6];
uint8_t RX_Address[6];
uint8_t e2_buf[16];

uint8_t Device_ID[10];
uint8_t active_channel;
uint8_t active_device;
uint8_t test_channel;
uint8_t fcc_power;

uint8_t front_cmd;
uint8_t rear_cmd;
uint8_t ga_cmd;
uint8_t dim_cmd;
uint8_t work_cmd;
uint8_t top_cmd;
uint8_t last_command;
uint8_t get_resp;
uint8_t stat;
uint8_t last_press;

uint8_t rx_bc;
uint8_t rssi;

micro_id remotes[2];
ButtonState function;
Multiple dev_ctl;
SerialSt SerialWk;

uint8_t SBuf_in[16];
uint8_t SBuf_inbc;
uint8_t SBuf_out[16];
uint8_t SBuf_outbc;

uint8_t SBuf_infe;
Hexcon hexc;

uint8_t testm;
uint8_t testm_tx;
/*
    Main application
*/
int main(void)
{
    /* Initializes MCU, drivers and middleware */
    SYSTEM_Initialize();
    TimerD._flags = 0;
    go_tx = 0;
    tx_pipe = 1;
    active_channel = 0x40;
    active_device = 0;
    function._flags = 0;

    front_cmd = 0;
    rear_cmd = 0;
    
    Device_ID[0] = (SIGROW.SERNUM5 & 0x7f);
    Device_ID[1] = SIGROW.SERNUM6;
    Device_ID[2] = SIGROW.SERNUM7;
    Device_ID[3] = SIGROW.SERNUM8;
    Device_ID[4] = SIGROW.SERNUM9;
    
    TCA0_ClearOverflowInterruptFlag();
    TCA0_EnableInterrupt();
    
    SPI0_Disable();                     // Disable SPI at startup
    SPI0.INTCTRL = 0x00;                // disable SPI Interrupts
    IO_PA5_PWR_SetLow();                // Make sure voltage regulator to RF chip is off
    PORTC_set_pin_level(0, false);      // TP4
    PORTA.DIR = 0xEE;                   // Set MISO as output when SPI0 is disabled
    PORTMUX.CTRLB = 0x00;               // Un-map SPI standard mappings to PORTA functions 
    
//    TCB0.CTRLA = 0x40;
    TCB0_SetCaptIsrCallback(*TimerB_Hand);
    PORTC_IO_PC3_TP1_SetInterruptHandler(*SerialIn_Hand);
    SBuf_inbc = 0;
    SBuf_infe = 0;
    IO_PC2_TP2_SetHigh();    
    testm = 0;
        
    KeyStatus._flags = 0;
    KeyStatus._scan_end = 1;
    Key._bounce = 0x3f;
    Key._cmd = Key._bounce;
    IntStatus._flags = 0;
//    PORTC_set_pin_level(1, false);      // TP3
    get_resp = 0;
    
//    remotes[0] = SI241_ReadRxAddress(0);    
//    remotes[1] = SI241_ReadRxAddress(0x20);    
    sei();
    
    tx_pipe = 1;
    SI241_SetupTx();
    
//    asm ("nop");
//    asm ("nop");
//    asm ("nop");   
    SI241_SetTx();
    dev_ctl._last_both_active = 1;
        
    while (1)
    {
#ifndef FCC_MODE        
        if(!IO_PC1_TP3_GetValue())
        {
            if(testm == 0)
            {
                testm = 1;
                testm_tx = 1;
                PORTC.PIN3CTRL = 0x09;                
            }
        }
        else
        {
            if(testm != 0)
            {
                testm = 0;
                testm_tx = 0;
                PORTC.PIN3CTRL = 0x08;
                SerialWk._flags = 0;
            }
        }
#endif
//        asm ("nop");
//        asm ("nop");
//        asm ("nop");
        if(IntStatus._tc0)
        {
            IntStatus._tc0 = 0;
            if(Si24_Status == 0x80)
            {
                PORTA_img = PORTA.IN;
                if(!(PORTA_img & 0x10)  && !SerialWk._tx_on)
                {
                    stat = SI241_Status();
//                    asm ("nop");
//                    asm ("nop");
//                    asm ("nop");
                    SI241_SetStandby();
                    if(get_resp != 0)
                    {
//                        asm ("nop");
//                        asm ("nop");
//                        asm ("nop");
                        get_resp = 0;
                        si24_on_timer = 200;            // set to 200 * 0.01 = 2 Seconds
                        SI241_SetupRxResp();
                        SI241_SetRxResp();
                    }
                    else if(!dev_ctl._both_devices && dev_ctl._update_used && !testm)
                    {
                        dev_ctl._update_used = 0;
                        dev_ctl._last_used = 0;
                        active_device = 0;
                        if(!dev_ctl._last_both_active)
                        {
                            if(dev_ctl._next_used)
                            {
                                active_device = 0x80;
                                dev_ctl._last_used = 1;
                            }
                        }
                        else
                        {
                            dev_ctl._both_devices = 1;
                        }
                        Key._cmd = 0xc0;
                        function._char[1] = function._char[1] | 0x80;
                        dev_ctl._both_devices_go = 1;
                    }
                    else if(dev_ctl._both_devices)
                    {
                        if(!active_device && !testm)
                        {
                            active_device = 0x80;
                            dev_ctl._both_devices = 0;
                            dev_ctl._both_devices_go = 1;
                        }
                    }
                }
                else
                {
                    asm ("nop");
                    asm ("nop");
                    asm ("nop");
                    if(si24_on_timer != 0)
                    {
                        if(!SerialWk._tx_on)
                        {
                            si24_on_timer--;
                        }
                        else
                        {
                            IO_PC0_TP4_Toggle();
                        }
                        if(si24_on_timer == 0)
                        {
                            Si24_Status = 0;
                            SI241_PwrOff();
                        }
                    }                                            
                }
            }
            
            else if(Si24_Status == 0x40)
            {
                PORTA_img = PORTA.IN;
                if(!(PORTA_img & 0x10))
                {
//                    asm ("nop");
//                    asm ("nop");
//                    asm ("nop");
                    rx_bc = SI241_RX0_BC();
//                    asm ("nop");
//                    asm ("nop");
//                    asm ("nop");
                    SI241_RX0_Payload(rx_bc);
                    rssi = SI241_RSSI();
//                    asm ("nop");
//                    asm ("nop");
//                    asm ("nop");                    
                    SI241_RX0_ClearInt();                                        
                    SI241_SetStandby();
//                    asm ("nop");
//                    asm ("nop");
//                    asm ("nop");                    
                    SI241_SaveRxAddress();
//                    asm ("nop");
//                    asm ("nop");
//                    asm ("nop");                                
//                    SI241_ReadRxAddress(0);
//                    asm ("nop");
//                    asm ("nop");
//                    asm ("nop");                                
                    IntStatus._tc0 = 1;
                    si24_on_timer = 500;
                }
                else
                {
                    if(si24_on_timer != 0)
                    {
                        si24_on_timer--;
                        if(si24_on_timer == 0)
                        {
                            if(dev_ctl._resend)
                            {
                                SI241_PwrOff();                              
                                dev_ctl._resend = 0;
                                dev_ctl._last_both_active = 0x01;
                                dev_ctl._both_devices = 0x01;
                                active_device = 0;  
                                dev_ctl._both_devices_go = 0x01;
                                Key._cmd = 0xc0;
                                function._char[1] = function._char[1] | 0x80;
                                IntStatus._tc0 = 1;
                                dev_ctl._invert = 0;
                                tx_pipe = 1;
                                function._ga = 0;
                                function._gahold = 0;
                            }
                            else
                            {
                                Si24_Status = 0;
                                SI241_PwrOff();
                            }
                        }
                    }                        

                }
            }   

            else if(Si24_Status == 0x20)
            {                
                work1 = SI241_Status();
                if(work1 & 0x20)
                {
//                    asm ("nop");
//                    asm ("nop");
//                    asm ("nop");                
                }                    

                if(si24_on_timer != 0)
                {
                    si24_on_timer--;
                    if(si24_on_timer == 0)
                    {
                        SI241_PwrOff();
                    }
                }
            }
            
            if(!KeyStatus._scan_end)
            {
                KeyStatus._interrupt = 0;
                ServiceKeyPress();
            }
            else if(KeyStatus._pressed)
            {
                ServiceKeyPress();
            }
        }
        
        if(Serial_Cmd == 0x80)
        {
            Serial_Cmd = 0;
            ParseRx(SerialWk._ser_in);
        }

        if(KeyStatus._new_cmd || dev_ctl._both_devices_go || (KeyStatus._hold_req && !KeyStatus._hold_ack) || (KeyStatus._hold_req2 && !KeyStatus._hold_ack2) || (KeyStatus._hold_req3 && !KeyStatus._hold_ack3))
        {
            ServiceCmd();
            if(go_tx || dev_ctl._both_devices_go)
            {
                dev_ctl._both_devices_go = 0;
                SI241_SetupTx();
                go_tx = 0;
//                asm ("nop");
//                asm ("nop");
//                asm ("nop");   
                SI241_SetTx();
            }            
        }
        else if((Si24_Status == 0) && !KeyStatus._pressed && !KeyStatus._scan_st && !SerialWk._rx_inprog && !SerialWk._tx_inprog && !SerialWk._buf_tx_req && !SerialWk._buf_tx_inprog && !SerialWk._tx_on)
        {
//            asm ("nop");
//            asm ("nop");
//            asm ("nop");
            SLPCTRL.CTRLA = 0x05;
                 asm ("SLEEP"); 
        }
        else if((Si24_Status == 0x20) || SerialWk._rx_inprog || SerialWk._tx_inprog || SerialWk._buf_tx_req || SerialWk._buf_tx_inprog || SerialWk._tx_on)
        {
//            asm ("nop");
//            asm ("nop");
//            asm ("nop");
            SLPCTRL.CTRLA = 0x01;

            asm ("SLEEP"); 
        }
//        timer0_val = TCA0_ReadTimer();
    }
}
/**
    End of File
*/