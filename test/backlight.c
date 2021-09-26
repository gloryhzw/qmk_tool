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

/* 
 * LED index to RGB address
 * >=100 means it belongs to EE dev
 */
// 3, 6, 9 notworking
// 4, 5 flickering
// 1, 2 ok
static const uint8_t g_led_pos[DRIVER_LED_TOTAL] = {
/* 0*/ 0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9, 0xCA,0xC0,0xC1,0xC2,0xC3,0xC4,
/*16*/  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,  110, 111, 112, 113,  21,  22,  23,  24,   25,   26,  27,   
/*37*/  116, 117, 118, 119, 120, 121, 122, 123, 124, 125,  126, 127, 128, 129,  32,  33,  34,  35,   36,   37,  38, 
/*58*/  132, 133, 134, 135, 136, 137, 138, 139, 140, 141,  142, 143, 145,  42,  43,  44,    
/*74*/  148, 150, 151, 152, 153, 154, 155, 156, 157,  158, 159, 161, 49,  51,  52,  53,  54,
/*91*/  114, 115, 130, 131, 146, 147, 162, 163,  55,  56,   57,  59,  60
};

// 1: 0xEE, 0: 0xE8
static const uint8_t g_led_chip[DRIVER_LED_TOTAL] = {
/* 0*/    1,   1,   1,   1,   1,   1,   1,   1,   1,   1,    1,   0,   0,  0,   0,    0,
/*16*/  100, 101, 102, 103, 104, 105, 106, 107, 108, 109,  110, 111, 112, 113,  21,  22,  23,  24,   25,   26,  27,   
/*37*/  116, 117, 118, 119, 120, 121, 122, 123, 124, 125,  126, 127, 128, 129,  32,  33,  34,  35,   36,   37,  38, 
/*58*/  132, 133, 134, 135, 136, 137, 138, 139, 140, 141,  142, 143, 145,  42,  43,  44,    
/*74*/  148, 150, 151, 152, 153, 154, 155, 156, 157,  158, 159, 161, 49,  51,  52,  53,  54,
/*91*/  114, 115, 130, 131, 146, 147, 162, 163,  55,  56,   57,  59,  60    
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
    int l = 0; 

    // only first row 16 keys
    if (index <= 15) {
        if (g_led_chip[index])
            dev = 0xEE;
        else
            dev = 0xE8;
        l = g_led_pos[index];        
    }
    else
    {
        return;
    }
        
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