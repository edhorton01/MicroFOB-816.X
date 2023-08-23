
#include "mcc_generated_files/utils/compiler.h"

#define KAYAK_REM    0x01
//#define NORMAL_REM  0x01
//#define FCC_MODE 0x01

typedef union {
    struct {
        uint16_t _flags;
    };
    struct {
        unsigned _scan_st:1;
        unsigned _scan_end:1;
        unsigned _pressed:1;        
        unsigned _new_cmd:1;
        unsigned _interrupt:1;
        unsigned _startCycle:1;
        unsigned _hold_req:1;
        unsigned _hold_ack:1;        
        unsigned _hold_req2:1;
        unsigned _hold_ack2:1;        
        unsigned _hold_req3:1;
        unsigned _hold_ack3:1;        

    };
} KEYstateBITS;

typedef struct {
    uint8_t _DB_cnt;
    uint8_t _bounce;
    uint8_t _cmd;
    uint8_t _kb;
    uint16_t _hold_cnt;    
} KEYstateControl;

typedef union {
    struct {
        uint8_t _flags;
    };
    struct {
        unsigned _tc0:1;
        unsigned _spi_tx:1;
        unsigned _spi_rx:1;        
    };
} INTstateBITS;

typedef struct {
    uint8_t _delaycnt;    
    uint8_t _delaytarget;
    union
    {
        struct {
            uint8_t _flags;
        };
        struct {
            unsigned _active:1;
            unsigned _finished:1;        
        };
    };
} TmrDelay;

typedef struct {
    uint8_t _chan;
    uint8_t _address[5];
} micro_id;

typedef union {
    struct {
        uint16_t _flags;
    };
    struct {
        uint8_t _char[2];
    };
    struct {
        unsigned _front:1;
        unsigned _rear:1;
        unsigned _top:1;
        unsigned _dim:1;
        unsigned _dimhold:1;
        unsigned _ga:1;
        unsigned _gahold:1;
        unsigned _work:1;
        unsigned _workhold:1;
        unsigned _fronthold:1;        
        unsigned _rearhold:1;        
        unsigned _gahold2:1;        
        unsigned _spare4:1;        
        unsigned _spare5:1;        
        unsigned _spare6:1;        
        unsigned _state:1;        
    };
} ButtonState;

typedef union {
    struct {
        uint8_t _flags;
    };
    struct {
        unsigned _both_devices:1;
        unsigned _both_devices_go:1;
        unsigned _last_both_active:1;
        unsigned _last_used:1;
        unsigned _invert:1;         
        unsigned _next_used:1;
        unsigned _update_used:1;
        unsigned _resend:1;        
    };
} Multiple;

typedef struct {
    uint8_t _ser_in;    
    uint8_t _ser_rx_bc;
    uint8_t _ser_rx_sc;    
    uint8_t _ser_rx_int;
    uint8_t _ser_out;
    uint8_t _ser_tx_bc;
    uint8_t _ser_tx_sc;    
    uint8_t _buf_tx_bcnt;    
    uint8_t _buf_tx_ccnt;    

    union
    {
        struct {
            uint8_t _flags;
        };
        struct {
            unsigned _rx_inprog:1;
            unsigned _tx_inprog:1;        
            unsigned _frame_err:1;        
            unsigned _buf_tx_req:1;        
            unsigned _buf_tx_inprog:1;        
            unsigned _tx_on:1;
        };
    };
} SerialSt;

typedef union {
    struct {
        uint16_t _word;
    };
    struct {
        uint8_t _ms;
        uint8_t _ls;
    };    
} Hexcon;


enum function_map {
    FRONT_M, 
    REAR_M, 
    TOP_M,
    DIM_M,
    DIMHOLD_M,
    GA_M,
    GAHOLD_M,
    WORK_M,
    WORKHOLD_M,
    FRONTHOLD_M,
    REARHOLD_M,
    GAHOLD2_M,    
};