#include "main.h"
#include "key_handler.h"
#include "mcc_generated_files/mcc.h"
#include "mcc_generated_files/include/port.h"

#define debounce 10
#define hold_len  150 
#define hold_len2 100 
#define hold_len3 150

extern volatile KEYstateBITS KeyStatus;
extern volatile KEYstateControl Key;
extern uint8_t go_tx;
extern uint8_t tx_pipe;
extern uint8_t get_resp;
extern uint8_t active_device;

extern uint8_t front_cmd;
extern uint8_t rear_cmd;
extern uint8_t ga_cmd;
extern uint8_t dim_cmd;
extern uint8_t work_cmd;
extern uint8_t last_press;

extern ButtonState function;
extern Multiple dev_ctl;

uint16_t mask;

void ServiceKeyPressInt(void)
{

    Key._kb = PORTB_get_port_level();
    Key._kb = Key._kb & 0x3f; // mask off buttons
    if(!KeyStatus._scan_st)
    {
        KeyStatus._interrupt = 1; // flag this interrupt happened
        Key._bounce = Key._kb;
        KeyStatus._scan_st = 1;
        KeyStatus._scan_end = 0;
        Key._DB_cnt = debounce; 
        KeyStatus._pressed = 1;
        Key._hold_cnt = 0;
        KeyStatus._hold_req = 0;
        KeyStatus._hold_ack = 0;
        KeyStatus._hold_req2 = 0;
        KeyStatus._hold_ack2 = 0;
        
        if(Key._kb == 0x3f)
        {
            KeyStatus._pressed = 0;
        }
    }
}

void ServiceKeyPress(void)
{

    Key._kb = PORTB_get_port_level();
    Key._kb = Key._kb & 0x3f; // mask off buttons
    if (Key._kb == Key._bounce)
    {
        if (Key._DB_cnt != 0)
        {
            Key._DB_cnt--;
            if (Key._DB_cnt == 0)
            {
                KeyStatus._scan_end = 1;
                KeyStatus._scan_st = 0;
                if (Key._cmd != Key._bounce)
                {
                    Key._cmd = Key._bounce;
                    KeyStatus._new_cmd = 1;
                    if(Key._kb == 0x3f)
                    {
                        Key._hold_cnt = 0;
                        KeyStatus._pressed = 0;
                    }
                    else
                    {
                        Key._hold_cnt = hold_len;
                        KeyStatus._pressed = 1;                        
                    }
                }
            }
        }
        else if(KeyStatus._scan_end && !KeyStatus._scan_st && KeyStatus._pressed && !KeyStatus._hold_req)
        {
            if(Key._hold_cnt != 0)
            {
                Key._hold_cnt--;
                if(Key._hold_cnt == 0)
                {
                    KeyStatus._hold_req = 1;
                    KeyStatus._hold_ack = 0;
                    KeyStatus._hold_req2 = 0;
                    KeyStatus._hold_ack2 = 0;
                    KeyStatus._hold_req3 = 0;
                    KeyStatus._hold_ack3 = 0;
                    Key._hold_cnt = hold_len2;
                }
            }
        }
        else if(KeyStatus._scan_end && !KeyStatus._scan_st && KeyStatus._pressed && KeyStatus._hold_req && !KeyStatus._hold_req2)
        {
            if(Key._hold_cnt != 0)
            {
                Key._hold_cnt--;
                if(Key._hold_cnt == 0)
                {
                    KeyStatus._hold_req2 = 1;
                    KeyStatus._hold_ack2 = 0;
                    Key._hold_cnt = hold_len3;                    
                }
            }
        }
        else if(KeyStatus._scan_end && !KeyStatus._scan_st && KeyStatus._pressed && KeyStatus._hold_req && KeyStatus._hold_req2 && !KeyStatus._hold_req3)
        {
            if(Key._hold_cnt != 0)
            {
                Key._hold_cnt--;
                if(Key._hold_cnt == 0)
                {
                    KeyStatus._hold_req3 = 1;
                    KeyStatus._hold_ack3 = 0;                   
                }
            }
        }

    }
    else
    {
        Key._bounce = Key._kb;
        Key._DB_cnt = debounce; 
        Key._hold_cnt = 0;
        KeyStatus._hold_req = 0;
        KeyStatus._hold_ack = 0;
        KeyStatus._hold_req2 = 0;
        KeyStatus._hold_ack2 = 0;
    }
}    

void ServiceCmd(void)
{
    enum function_map shifter;
    
    go_tx = 0;
    if (KeyStatus._new_cmd)
    {
        KeyStatus._new_cmd = 0;
        KeyStatus._scan_st = 0;        
        tx_pipe = 1;
        dev_ctl._both_devices = 0;
        shifter = 0;

        switch (Key._cmd)
        {
            case 0x3e:
            {
                last_press = Key._cmd;
                shifter = TOP_M;
                function._dimhold = 0;                
                if(dev_ctl._last_both_active)
                {
                    dev_ctl._both_devices = 0x01;
                    active_device = 0;                                   
                }
                else
                {
                    if(dev_ctl._last_used)
                    {
                        active_device = 0x80; 
                    }
                    else
                    {
                        active_device = 0x0;                        
                    }
                }               
                go_tx = 1; 
                break;
            }

            case 0x3d:
            {
                shifter = REAR_M;
                function._dimhold = 0;                
#ifdef KAYAK_REM 
                dev_ctl._last_both_active = 0x0;
                dev_ctl._last_used = 0x0;
                if((last_press == 0x3b) && (active_device == 0x80))
                {
                    active_device = 0;
                }
                else
                {
                    active_device = 0;
                    dim_cmd = 2;
                    function._dim = 1;                    
                    go_tx = 1; 
                }
                last_press = Key._cmd;
#elif NORMAL_REM
                if(dev_ctl._last_both_active)
                {
                    dev_ctl._both_devices = 0x01;
                    active_device = 0;                                   
                }
                else
                {
                    if(dev_ctl._last_used)
                    {
                        active_device = 0x80; 
                    }
                    else
                    {
                        active_device = 0x0;                        
                    }
                }
                go_tx = 1; 
#endif    
                break;
            }

            case 0x3b:
            {
                last_press = Key._cmd;
                shifter = DIM_M;
                go_tx = 1;
                if(dev_ctl._last_both_active)
                {
                    dev_ctl._both_devices = 0x01;
                    active_device = 0;                                   
                }
                else
                {
                    if(dev_ctl._last_used)
                    {
                        active_device = 0x80; 
                    }
                    else
                    {
                        active_device = 0x0;                        
                    }
                }                                 
                break;
            }

            case 0x37:
            {
                last_press = Key._cmd;
                shifter = GA_M;
#ifdef KAYAK_REM                
                dev_ctl._both_devices = 0x01;
                dev_ctl._last_both_active = 0x01;
                active_device = 0;
                asm ("nop");
                asm ("nop");
                asm ("nop");                  
                function._front = 0;
                function._rear = 0;
                function._top = 0;
                function._dimhold = 0;
                dim_cmd = 2;
                function._dim = 1;
#elif NORMAL_REM
                if(dev_ctl._last_both_active)
                {
                    dev_ctl._both_devices = 0x01;
                    active_device = 0;                                   
                }
                else
                {
                    if(dev_ctl._last_used)
                    {
                        active_device = 0x80; 
                    }
                    else
                    {
                        active_device = 0x0;                        
                    }
                }                               
#endif                
                go_tx = 1; 
                break;
            }

            case 0x2f:
            {
                last_press = Key._cmd;
                shifter = WORK_M;
                if(dev_ctl._last_both_active)
                {
                    dev_ctl._both_devices = 0x01;
                    active_device = 0;                                   
                }
                else
                {
                    if(dev_ctl._last_used)
                    {
                        active_device = 0x80; 
                    }
                    else
                    {
                        active_device = 0x0;                        
                    }
                }               
                go_tx = 1;                 
                break;
            }

            case 0x1f:
            {
                shifter = FRONT_M;
                function._dimhold = 0;                
#ifdef KAYAK_REM                
                dev_ctl._last_both_active = 0x0;
                dev_ctl._last_used = 0x1;
                if((last_press == 0x3b) && (active_device == 0))
                {
                    active_device = 0x80;
                }
                else
                {
                    active_device = 0x80;
                    dim_cmd = 2;
                    function._dim = 1;                    
                    go_tx = 1; 
                }
                last_press = Key._cmd;
#elif NORMAL_REM
                if(dev_ctl._last_both_active)
                {
                    dev_ctl._both_devices = 0x01;
                    active_device = 0;                                   
                }
                else
                {
                    if(dev_ctl._last_used)
                    {
                        active_device = 0x80; 
                    }
                    else
                    {
                        active_device = 0x0;                        
                    }
                }               
                go_tx = 1; 
#endif                
                break;
            }                        
        }
        
        if(go_tx)
        {
            dev_ctl._invert = 0;
            mask = (0x01 << shifter);
#ifdef KAYAK_REM
            if(shifter == FRONT_M)
            {
                if((function._flags & mask) || function._fronthold)                   
                {
                    function._front = 0;
                    function._fronthold = 0;
                }
                else
                {
                    function._flags = function._flags ^ mask;
                }
            }
            else if(shifter == REAR_M)
            {
                if((function._flags & mask) || function._rearhold)                   
                {
                    function._rear = 0;
                    function._rearhold = 0;
                }
                else
                {
                    function._flags = function._flags ^ mask;
                }                
            }
 
            else if(shifter == DIM_M)
            {
                if(!(function._flags & mask))                   
                {
                    function._flags = function._flags | mask;
                    dim_cmd = 0;
                    function._state = 1;
                }
                else
                {
                    dim_cmd++;
                    if(dim_cmd >= 3)
                    {
                        function._flags = function._flags ^ mask;
                        function._state = 0;
                    }
                    else
                    {
                        if(dim_cmd == 1)
                        {
                            Key._cmd = 0xac;
                        }
                        else if(dim_cmd == 2)
                        {
                            Key._cmd = 0xad;
                        }
                    }
                }
            } 
            
            else
            {
                function._flags = function._flags ^ mask;                
            }
         
            if(function._flags & mask)
            {
                function._state = 1;
                if(dev_ctl._both_devices && (shifter == GA_M))
                {
                    function._front = 1;
                    function._rear = 1;
                }
                else
                {
                    if((shifter == FRONT_M) && (function._rear == 1))
                    {
                        function._ga = 1;
                    }
                    else if((shifter == REAR_M) && (function._front == 1))
                    {
                        function._ga = 1;
                    }
                }
            }
            else
            {
                function._state = 0;                    
                if(dev_ctl._both_devices && (shifter == GA_M))
                {
                    function._front = 0;
                    function._rear = 0;
                }
                else
                {
                    if((shifter == FRONT_M) && (function._rear == 0))
                    {
                        function._ga = 0;
                    }
                    else if((shifter == REAR_M) && (function._front == 0))
                    {
                        function._ga = 0;
                    }
                }
            }
            
#elif NORMAL_REM
            if(shifter == FRONT_M)
            {
                if(!(function._flags & mask))                   
                {
                    function._flags = function._flags | mask;
                    front_cmd = 0;
                    function._state = 1;
                }
                else
                {
                    front_cmd++;
                    if(front_cmd > 2)
                    {
                        function._flags = function._flags ^ mask;
                        function._state = 0;
                        function._dimhold = 0;
                    }
                    else
                    {
                        function._state = 1;
                        Key._cmd = Key._cmd + front_cmd;
                        function._dimhold = 0;
                    }
                }
            }
            else if(shifter == REAR_M)
            {
                if(!(function._flags & mask))                   
                {
                    function._flags = function._flags | mask;
                    rear_cmd = 0;
                    function._state = 1;
                }
                else
                {
                    rear_cmd++;
                    if(rear_cmd > 2)
                    {
                        function._flags = function._flags ^ mask;
                        function._state = 0;
                        function._dimhold = 0;
                    }
                    else
                    {
                        function._state = 1;
                        Key._cmd = (Key._cmd & 0xf4) + rear_cmd;
                        function._dimhold = 0;
                    }
                }
            }            
            else if(shifter == DIM_M)
            {
                if(!(function._flags & mask))                   
                {
                    function._flags = function._flags | mask;
                    dim_cmd = 0;
                    function._state = 1;
                }
                else
                {
                    dim_cmd++;
                    if(dim_cmd >= 3)
                    {
                        function._flags = function._flags ^ mask;
                        function._state = 0;
                    }
                    else
                    {
                        if(dim_cmd == 1)
                        {
                            Key._cmd = 0xac;
                        }
                        else if(dim_cmd == 2)
                        {
                            Key._cmd = 0xad;
                        }
                    }
                }
            }
            
            else if(shifter == GA_M)
            {
                rear_cmd = 0;
                front_cmd = 0;
                function._work = 0;
                function._front = 0;
                function._rear = 0;
                if(!(function._flags & mask))                   
                {
                    function._flags = function._flags | mask;
                    ga_cmd = 0;
                    function._state = 1;
                }
                else
                {
                        function._flags = function._flags ^ mask;
                        function._state = 0;
                        function._dimhold = 0;
                }
            }
            
            else if(shifter == WORK_M)
            {
                rear_cmd = 0;
                front_cmd = 0;
                ga_cmd = 0;
                function._front = 0;
                function._rear = 0;
                function._ga = 0;
                function._flags = function._flags ^ mask;
                function._dimhold = 0;
                if(function._flags & mask)
                {
                    function._state = 1;                
                }
                else
                {
                     function._state = 0;                     
                }
            }  
            else if(shifter == TOP_M)
            {  
                function._flags = function._flags ^ mask;
                if(function._flags & mask)
                {
                    function._state = 1;                
                }
                else
                {
                     function._state = 0;                     
                }
            }
#endif               
        }
    }
    
    else if(KeyStatus._hold_req && !KeyStatus._hold_ack)
    {
        KeyStatus._hold_ack = 1;
        shifter = 0;
        asm ("nop");
        asm ("nop");
        asm ("nop");
        tx_pipe = 1;
        get_resp = 0;
        
        switch (Key._bounce)
        {
            case 0x3b:
            {
                Key._cmd = 0x3b;
                shifter = DIMHOLD_M;
                go_tx = 1;
                if(dev_ctl._last_both_active)
                {
                    dev_ctl._both_devices = 0x01;
                    active_device = 0;                                   
                }
                else
                {
                    if(dev_ctl._last_used)
                    {
                        active_device = 0x80; 
                    }
                    else
                    {
                        active_device = 0x0;                        
                    }
                }               
                break;
            }

            case 0x2f:
            {
                shifter = WORKHOLD_M;
                go_tx = 1;
#ifdef NORMAL_REM
                if(dev_ctl._last_both_active)
                {
                    dev_ctl._both_devices = 0x01;
                    active_device = 0;                                   
                }
                else
                {
                    if(dev_ctl._last_used)
                    {
                        active_device = 0x80; 
                    }
                    else
                    {
                        active_device = 0x0;                        
                    }
                }               
#endif                    
                break;
            }
            
#ifdef NORMAL_REM                        
            case 0x3d:
            {
                shifter = REARHOLD_M;
                dev_ctl._next_used = 0;
                go_tx = 1;
                Key._cmd = 0x37;
                function._char[1] = function._char[1] & 0x7f;
                dev_ctl._last_both_active = 0;
                dev_ctl._both_devices = 0x01;
                dev_ctl._update_used = 0x01;
                function._rearhold = 0;
                active_device = 0;                                   
                break;
            }

            case 0x1f:
            {
                shifter = FRONTHOLD_M;
                dev_ctl._next_used = 1;
                go_tx = 1;
                Key._cmd = 0x37;
                function._char[1] = function._char[1] & 0x7f;
                dev_ctl._last_both_active = 0;
                dev_ctl._both_devices = 0x01;
                dev_ctl._update_used = 0x01;
                function._fronthold = 0;
                active_device = 0;                                   
                break;
            }
#else
            case 0x3d:
            {
                shifter = REARHOLD_M;
                dev_ctl._next_used = 0;
                go_tx = 1;
                Key._cmd = 0xc8;
                function._char[1] = function._char[1] & 0x7f;
                dev_ctl._last_both_active = 0;
                dev_ctl._both_devices = 0;
                function._rearhold = 1;
                active_device = 0;                                   
                break;
            }

            case 0x1f:
            {
                shifter = FRONTHOLD_M;
                dev_ctl._next_used = 0;
                go_tx = 1;
                Key._cmd = 0xc8;
                function._char[1] = function._char[1] & 0x7f;
                dev_ctl._last_both_active = 0;
                dev_ctl._both_devices = 0;
                function._fronthold = 1;
                active_device = 0x80;                                   
                break;
            }            
#endif

            case 0x37:
            {
#ifdef NORMAL_REM
                shifter = GAHOLD_M;
                Key._cmd = 0x38;
                if(dev_ctl._last_both_active)
                {
                    dev_ctl._both_devices = 0x01;
                    active_device = 0;                                   
                }
                else
                {
                    if(dev_ctl._last_used)
                    {
                        active_device = 0x80; 
                    }
                    else
                    {
                        active_device = 0x0;                        
                    }
                }                               
                function._ga = 1;
                go_tx = 1;
#else
                shifter = GAHOLD_M;
                Key._cmd = 0xc8;
                dev_ctl._last_both_active = 0x01;
                dev_ctl._both_devices = 0x01;
                active_device = 0;
                function._ga = 1;
                go_tx = 1;                
#endif                
                break;
            }                        
        }
        asm ("nop");
        asm ("nop");
        asm ("nop");                
        if(go_tx)
        {
            asm ("nop");
            asm ("nop");
            asm ("nop");                            
            dev_ctl._invert = 1;
            mask = (0x01 << shifter);
            if(shifter == DIMHOLD_M)
            {
                function._flags = function._flags ^ mask;
                front_cmd = 0;
                rear_cmd = 0;
            }
            else if(shifter == WORKHOLD_M)
            {
                function._work = 1;
                front_cmd = 0;
                rear_cmd = 0;
            }
            else if(shifter == REARHOLD_M)
            {
                front_cmd = 0;
                rear_cmd = 0;
                dev_ctl._invert = 0;
            }
            else if(shifter == FRONTHOLD_M)
            {
                front_cmd = 0;
                rear_cmd = 0;
                dev_ctl._invert = 0;
            }
            else if(shifter == GAHOLD_M)
            {
                front_cmd = 0;
                rear_cmd = 0;
                dev_ctl._invert = 0;
                function._flags = function._flags | mask;
            }
            
            if(function._flags & mask)
            {
                function._state = 1;
            }
            else
            {
                 function._state = 0;               
            }
        }
    }
    
    else if(KeyStatus._hold_req2 && !KeyStatus._hold_ack2)
    {
        KeyStatus._hold_ack2 = 1;
        shifter = 0;
        asm ("nop");
        asm ("nop");
        asm ("nop");
        tx_pipe = 1;
        get_resp = 0;
        
        switch (Key._bounce)
        { 
#ifdef NORMAL_REM
            case 0x37:
            {
                shifter = GAHOLD2_M;
                dev_ctl._next_used = 0;
                go_tx = 1;
                Key._cmd = 0x37;
                function._char[1] = function._char[1] & 0x7f;
                dev_ctl._last_both_active = 0x01;
                dev_ctl._both_devices = 0x01;
                dev_ctl._update_used = 0x01;
                function._gahold = 0;
                function._ga = 0;
                active_device = 0;                                   
                break;
            }            
#else            
            case 0x37:
            {
                go_tx = 1;
                tx_pipe = 0;
                get_resp = 1;             
                break;
            }
#endif            
        }
        asm ("nop");
        asm ("nop");
        asm ("nop");                
        if(go_tx)
        {
#ifdef NORMAL_REM
            asm ("nop");
            asm ("nop");
            asm ("nop");                            
            dev_ctl._invert = 0;           
#else            
            asm ("nop");
            asm ("nop");
            asm ("nop");                            
            dev_ctl._invert = 1;
            dev_ctl._both_devices = 0;
            active_device = 0;
#endif            
        }
    }

    else if(KeyStatus._hold_req3 && !KeyStatus._hold_ack3)
    {
        KeyStatus._hold_ack3 = 1;
        shifter = 0;
        asm ("nop");
        asm ("nop");
        asm ("nop");
        tx_pipe = 1;
        get_resp = 0;
        
        switch (Key._bounce)
        { 
#ifdef NORMAL_REM
            case 0x37:
            {
                function._ga = 0;
                function._gahold = 0;
                function._gahold2 = 0;
                go_tx = 1;
                tx_pipe = 0;
                get_resp = 1;             
                break;
            }
#else            
            case 0x37:
            {
                go_tx = 1;
                tx_pipe = 0;
                get_resp = 1;             
                break;
            }
#endif            
        }
        asm ("nop");
        asm ("nop");
        asm ("nop");                
        if(go_tx)
        {
            asm ("nop");
            asm ("nop");
            asm ("nop");                            
            dev_ctl._invert = 0;           
        }
    }

    
}