#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <SN32F260.h>
#include "ch.h"
#include "hal.h"

#include "color.h"
#include "wait.h"
#include "util.h"
#include "matrix.h"
#include "debounce.h"
#include "quantum.h"

#include "SPI.h"

#ifndef GMMK_SPI

#define I2C_SCL A4
#define I2C_SDA A5

#define I2C_SCL_IN readPin(I2C_SCL)
#define I2C_SDA_IN readPin(I2C_SDA)

#define I2C_SCL_HI setPinInputHigh(I2C_SCL)
#define I2C_SCL_LO do { setPinOutput(I2C_SCL); writePinLow(I2C_SCL); } while (0)
#define I2C_SCL_HIZ setPinInputHigh(I2C_SCL)

#define I2C_SDA_HI setPinInputHigh(I2C_SDA)
#define I2C_SDA_LO do { setPinOutput(I2C_SDA); writePinLow(I2C_SDA); } while (0)
#define I2C_SDA_HIZ setPinInputHigh(I2C_SDA)

#define I2C_DELAY   for (int32_t i = 0; i < 8; i++);

static uint8_t  i2c_byte_ct = 0;
static uint8_t  i2c_addr_rw = 0;
static uint8_t* i2c_data_ptr;
static uint8_t  i2c_data_byte[2] = {0};
static uint8_t  i2c_tx_byte = 0;

void i2c_init(void)
{
    I2C_SCL_HIZ;
    I2C_SDA_HIZ;
}

static inline void i2c_process_bit(void)
{
    if (i2c_tx_byte & 0x80)
    {
        I2C_SDA_HIZ;
    }
    else
    {
        I2C_SDA_LO;
    }

    i2c_tx_byte = i2c_tx_byte << 1;
    I2C_DELAY;

    I2C_SCL_HIZ;

    I2C_DELAY;
    
    I2C_SCL_LO;    

    I2C_DELAY;    
}

static inline void i2c_transaction(void)
{
    I2C_SDA_LO;
    
    I2C_DELAY;
    
    I2C_SCL_LO;

    I2C_DELAY;

    i2c_tx_byte = i2c_addr_rw;

    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();

    I2C_DELAY;

    goto I2C_STATE_READ_ACK;

I2C_STATE_WRITE_BYTE:
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();
    i2c_process_bit();

    I2C_DELAY;

    i2c_byte_ct = i2c_byte_ct - 1;
    i2c_data_ptr++;

I2C_STATE_READ_ACK:
    I2C_SDA_HIZ;
    I2C_DELAY;
    I2C_SCL_HIZ;
    I2C_DELAY;


    if (!I2C_SDA_IN)
    {
        // Slave did ACK, move on to data
        if (i2c_byte_ct > 0)
        {
            i2c_tx_byte = *i2c_data_ptr;

            I2C_SDA_LO;
            I2C_DELAY;
            I2C_SCL_LO;
            I2C_DELAY;

            goto I2C_STATE_WRITE_BYTE;
        }
    }
    
    I2C_DELAY;

    I2C_SDA_LO;

    I2C_DELAY;
    
    I2C_SCL_LO;

    I2C_DELAY;

    I2C_SCL_HIZ;

    I2C_DELAY;
    I2C_SDA_HIZ;

    I2C_DELAY;    
}

static inline void i2c_write_buf(uint8_t devid, uint8_t* data, uint8_t len)
{   
    i2c_addr_rw  = devid;

    i2c_data_ptr = data;
    i2c_byte_ct  = len;

    i2c_transaction();
}

static inline void i2c_write_reg(uint8_t devid, uint8_t reg, uint8_t data)
{
    i2c_addr_rw      = devid;
    i2c_data_byte[0] = reg;
    i2c_data_byte[1] = data;

    i2c_data_ptr     = i2c_data_byte;
    i2c_byte_ct      = 2;

    i2c_transaction();
}

static void reset_rgb(int devid)
{
    i2c_write_reg(devid, 0xFD, 0x0B);
    i2c_write_reg(devid, 0x0A, 0x00);
    i2c_write_reg(devid, 0x00, 0x00);
    i2c_write_reg(devid, 0x01, 0x10);
    i2c_write_reg(devid, 0x05, 0x00);
    i2c_write_reg(devid, 0x06, 0x00);
    i2c_write_reg(devid, 0x08, 0x00);
    i2c_write_reg(devid, 0x09, 0x00);
    i2c_write_reg(devid, 0x0B, 0x00);
    // i2c_write_reg(devid, 0x0D, 0x0F);
    i2c_write_reg(devid, 0x0E, 0x01);
    i2c_write_reg(devid, 0x14, 68);
    i2c_write_reg(devid, 0x15, 128);
    i2c_write_reg(devid, 0x0F, 153);

    i2c_write_reg(devid, 0xFD, 0);
    for (int32_t i = 0; i < 0x10; i++)
        i2c_write_reg(devid, i, 0xFF);
    /* skip blink control 0x10~0x1F as reset 0 */
    for (int32_t i = 0x20; i < 0xA0; i++)
        i2c_write_reg(devid, i, 0xFF);
    
    i2c_write_reg(devid, 0xFD, 1);
    for (int32_t i = 0; i < 0x10; i++)
        i2c_write_reg(devid, i, 0xFF);
    /* skip blink control 0x10~0x1F as reset 0 */
    for (int32_t i = 0x20; i < 0xA0; i++)
        i2c_write_reg(devid, i, 0xFF);

    i2c_write_reg(devid, 0xFD, 0xB);
    i2c_write_reg(devid, 0x0A, 1);    
}

#if 0

        KC_ESC,     KC_F1,      KC_F2,      KC_F3,      KC_F4,      KC_F5,      KC_F6,      KC_F7,      KC_F8,      KC_F9,          KC_F10,         KC_F11,     KC_F12, 
        KC_GRV,     KC_1,       KC_2,       KC_3,       KC_4,       KC_5,       KC_6,       KC_7,       KC_8,       KC_9,           KC_0,           KC_MINS,    KC_EQL,    KC_NUMLOCK, KC_KP_SLASH,         KC_KP_ASTERISK,
        KC_TAB,     KC_Q,       KC_W,       KC_E,       KC_R,       KC_T,       KC_Y,       KC_U,       KC_I,       KC_O,           KC_P,           KC_LBRC,    KC_RBRC,   KC_KP_7,    KC_KP_8,             KC_KP_9, 
        KC_CAPS,    KC_A,       KC_S,       KC_D,       KC_F,       KC_G,       KC_H,       KC_J,       KC_K,       KC_L,           KC_SCLN,        KC_QUOT,    KC_BSLS,   KC_KP_4,    KC_KP_5,             KC_KP_6, 
        KC_LSFT,    KC_Z,       KC_X,       KC_C,       KC_V,       KC_B,       KC_N,       KC_M,       KC_COMM,    KC_DOT,         KC_SLSH,        KC_LSFT,    KC_ENT,    KC_KP_1,    KC_KP_2,             KC_KP_3, 
        KC_LCTL,    KC_LGUI,    KC_LALT,    KC_SPC,     KC_RALT,    MO(_FN),    KC_APP,     KC_RCTL,    KC_LEFT,    KC_DOWN,        KC_UP,          KC_RGHT,    KC_BSPC,   KC_KP_0,    KC_KP_DOT,           KC_KP_ENTER,
        KC_PSCR,    KC_SLCK,    KC_PAUS,    KC_INS,     KC_HOME,    KC_PGUP,    KC_DEL,     KC_END,     KC_PGDN,    KC_KP_MINUS,    KC_KP_PLUS), 


const led_config_t g_led_config = { 
{
/*0*/   {   0,          1,      2,          3,          4,          5,          6,          7,          8,          9,              10,             11,         12,         NO_LED,      NO_LED,     NO_LED},
/*1*/   {  16,         17,     18,          19,         20,         21,         22,         23,         24,         25,             26,             27,         28,         33,          34,         35},
/*2*/   {  37,         38,     39,          40,         41,         42,         43,         44,         45,         46,             47,             48,         49,         54,          55,         56},
/*3*/   {  58,         59,     60,          61,         62,         63,         64,         65,         66,         67,             68,             69,         50,         71,          72,         73},
/*4*/   {  74,         75,     76,          77,         78,         79,         80,         81,         82,         83,             84,             85,         70,         87,          88,         89},
/*5*/   {  91,         92,     93,          94,         95,         96,         97,         98,         99,         100,            86,             101,        29,         102,         103,         90},
/*6*/   {  13,         14,     15,          30,         31,         32,         51,         52,         53,         36,             57,             NO_LED,     NO_LED,     NO_LED,      NO_LED,         NO_LED},

}

#endif

/* 
 * LED index to RGB address
 * >=100 means it belongs to EE dev
 */
// 3, 6, 9 notworking
// 4, 5 flickering
// 1, 2 ok



static const uint8_t g_led_pos[DRIVER_LED_TOTAL] = {
/* 0*/ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9, 0xCA,0xC0,0xC1,0xC2,0xC3,0xC4,
/*16*/ 0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x9C, 0x9D,0x9E,0x9F,0xCA,0x90,0x91,0x92,   0,    0,    0,   0,   
/*37*/ 0x60,0x61,0x62,0x63,0x64,0x65,0x69,0x6A,0x6B,0x6C, 0x6D,0x6E,0x6F,   0,0x98,0x60,0x61,   0,    0,    0,   0, 
/*58*/ 0x30,0x31,0x32,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C, 0x3D,0x3E,0x3F,   0,   0,   0,    
/*74*/ 0x03,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D, 0x0E,0x0F,0x36,   0,   0,   0,   0,
/*91*/ 0x03,0x04,0x05,0x07,0x09,0x0a,0x0B,0x0D,0x0E,0x0F, 0x3B,   0,   0
};

// 1: 0xEE, 0: 0xE8
static const uint8_t g_led_chip[DRIVER_LED_TOTAL] = {
/* 0*/    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,   0,   0,  0,    0,   0,
/*16*/    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,  0,    0,   0,   0,   0,    0,    0,   0,   
/*37*/    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,  0,    0,   0,   0,   0,    0,    0,   0, 
/*58*/    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   1,  0,    0,   0,    
/*74*/    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,   1,   0,  0,    0,   0,
/*91*/    0,   0,   0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0    
};


static void set_pwm(uint8_t dev, uint8_t addr, uint8_t value)
{
    if (addr >= 0x80) {
        i2c_write_reg(dev, 0xFD, 1);
        addr -= 0x80;
    }        
    else
        i2c_write_reg(dev, 0xFD, 0);
        
    i2c_write_reg(dev, addr + 0x20, value);    
}

void _set_color(int index, uint8_t r, uint8_t g, uint8_t b)
{
    uint8_t dev;
    int l = g_led_pos[index];        ; 

    // TBD
    if (l == 0)
        return;

    // only first row 16 keys
    if (g_led_chip[index])
        dev = 0xEE;
    else
        dev = 0xE8;
        
    // r, g, b TBD
    set_pwm(dev, l, r);
    set_pwm(dev, l + 0x10, g);
    set_pwm(dev, l + 0x20, b);
}

void _read_color(int index, uint8_t *r, uint8_t *g, uint8_t *b)
{
    // TBD
    *r = 0;
    *g = 0;
    *b = 0;
}

#else /* GMMK_SPI */

#define SDB     B0

static int g_cs_pin = 0;

void spi_init(void)
{
    SPI0_Init();
    SPI0_Enable();
}

void spi_set_cspin(int pin)
{   
    if (g_cs_pin == pin)
        return;

    writePinHigh(B2); 
    setPinOutput(B2);    
    writePinHigh(B1); 
    setPinOutput(B1);

    g_cs_pin = pin;
}

extern void SPI0_Read3(unsigned char b1, unsigned char b2, unsigned char *b3);

void spi_read3(unsigned char b1, unsigned char b2, unsigned char *b3)
{    
    writePinLow(g_cs_pin);    
    SPI0_Read3(b1, b2, b3);    
    writePinHigh(g_cs_pin);
}


void spi_write(uint8_t *data_p, int len)
{    
    writePinLow(g_cs_pin);    
    SPI0_Write(data_p, len);    
    writePinHigh(g_cs_pin);
}

void spi_w3(uint8_t page, uint8_t addr, uint8_t data)
{
    uint8_t c[4];
    c[0] = page | 0x20;
    c[1] = addr;
    c[2] = data;
    spi_write(c, 3);
}

void spi_r3(uint8_t page, uint8_t addr, uint8_t *data)
{
    uint8_t c[4];
    c[0] = page | 0x20;
    c[1] = addr;    
    spi_read3(c[0], c[1], data);
}

/* 
 * LED index to RGB address
 * >100 means it belongs to pin B1 chipselected SN2735 chip, the real addr is minus by 100 
 */
static const uint8_t g_led_pos[DRIVER_LED_TOTAL] = {
/* 0*/    0,   2,   3,   4,   5,   6,   7,   8,   9,  10,   11,  12,  13,  14,  15,  16,
/*16*/  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,  110, 111, 112, 113,  21,  22,  23,  24,   25,   26,  27,   
/*37*/  116, 117, 118, 119, 120, 121, 122, 123, 124, 125,  126, 127, 128, 129,  32,  33,  34,  35,   36,   37,  38, 
/*58*/  132, 133, 134, 135, 136, 137, 138, 139, 140, 141,  142, 143, 145,  42,  43,  44,    
/*74*/  148, 150, 151, 152, 153, 154, 155, 156, 157,  158, 159, 161, 49,  51,  52,  53,  54,
/*91*/  114, 115, 130, 131, 146, 147, 162, 163,  55,  56,   57,  59,  60
};

void _set_color(int index, uint8_t r, uint8_t g, uint8_t b)
{
    int l = g_led_pos[index];

    if (l >= 100)
    {
        l -= 100;
        spi_set_cspin(B1);
    }
    else
        spi_set_cspin(B2);

    int y = l / 16;
    int a = l % 16;
            
    spi_w3(1, y * 48 + a, r);       // r
    spi_w3(1, y * 48 + a + 2*8, b); // b
    spi_w3(1, y * 48 + a + 4*8, g); // g  
}

void _read_color(int index, uint8_t *r, uint8_t *g, uint8_t *b)
{
    int l = g_led_pos[index];

    if (l >= 100)
    {
        l -= 100;
        spi_set_cspin(B1);
    }
    else
        spi_set_cspin(B2);

    int y = l / 16;
    int a = l % 16;
            
    spi_r3(1, y * 48 + a, r);       // r
    spi_r3(1, y * 48 + a + 2*8, b); // b
    spi_r3(1, y * 48 + a + 4*8, g); // g  
}

void reset_rgb(int pin)
{
    spi_set_cspin(pin);
    
    spi_w3(3, 0, 0);
    spi_w3(3, 0x13, 0xAA);
    spi_w3(3, 0x14, 0);
    // spi_w3(3, 0x15, 4);
    spi_w3(3, 0x15, 0);
    spi_w3(3, 0x16, 0xC0);
    spi_w3(3, 0x1A, 0);
    
    // set curent
    for (int i = 0; i < 12; i++)
    {
        spi_w3(4, i, 0x80);
    }               
    
    // led all on
    for (int i = 0; i < 192/8; i++)
    {
        spi_w3(0, i, 0xFF);
    }

    // turn off pwm
    for (int i = 0; i < 192; i++)
    {
        spi_w3(1, i, 0);
    }
                
    // normal mode
    spi_w3(3, 0, 1);
}

#endif  /* GMMK_SPI */

void process_backlight(uint8_t devid, volatile LED_TYPE* states)
{    
    static unsigned char state = 0;       

    switch (state)
    {
        case 0: /* init RGB chips */
        #ifdef GMMK_SPI            
            spi_init();
            
            writePinHigh(SDB);
            setPinOutput(SDB);            
            
            reset_rgb(B1);                        
            reset_rgb(B2);
        #else
            i2c_init();
            reset_rgb(0xE8);
            reset_rgb(0xEE);            
        #endif
        
            state = 1;
            break;
    }
}