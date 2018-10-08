/*
 * File:   main.c
 * Author: Nakayama
 *
 * Created on September 28, 2018, 8:43 PM
 */

#include <xc.h>
#include <stdio.h>

#define _XTAL_FREQ 8000000
// #define USE_INNER_PULLUP

#pragma config FOSC = INTOSC, WDTE = OFF, LVP = OFF

#define LCD_ADD_7BIT      0x3e

static const unsigned char LCD_ADD_WRITE     = LCD_ADD_7BIT << 1;

void I2C_Master_init() {  
#ifdef USE_INNER_PULLUP
    /* set pull up */
    WPUB   = 0b00010010;
    WPUBbits.WPUB1 = 1;
    WPUBbits.WPUB4 = 1;
#else
    WPUB   = 0b00000000;
#endif
    
    SSP1STAT = 0b10000000; // standard mode (100kHz)
    SSP1CON1 = 0b00101000; // allow clock from slave, I2C master mode
    SSP1CON2 = 0;
    SSP1ADD = 0x13;
}

void I2C_Master_wait() {
    // wait for founding START or ACK sequence
    while ((SSP1STAT & 0x04) || (SSP1CON2 & 0x1F));
}

void I2C_Master_start() {
    SSP1CON2bits.SEN = 1;
    I2C_Master_wait();
}

void I2C_Master_stop() {
    SSP1CON2bits.PEN = 1;
    I2C_Master_wait();
}

void I2C_Master_write(unsigned char d) {
    SSP1BUF = d;
    I2C_Master_wait();
}

void ST7032_LCD_write(unsigned char cont, char data) {
    I2C_Master_start();
    I2C_Master_write(LCD_ADD_WRITE);
    I2C_Master_write(cont);
    I2C_Master_write(data);
    I2C_Master_stop();
}

void ST7032_LCD_writeString(const char* str) {
    const char* d;
    for (d = str; *d != '\0'; d++) {
        char c = *d;
        ST7032_LCD_write(0x40, c);
        __delay_us(30);
    }
}

void ST7032_LCD_init(unsigned char contrast) {    
    ST7032_LCD_write(0x00, 0x38); // Function set
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x39); // Function set
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x14); // Internal OSC frequency
    __delay_us(100);
    ST7032_LCD_write(0x00, (unsigned char)(0x70|(contrast & 0xf))); // Contrast set
    __delay_us(100);
    ST7032_LCD_write(0x00, (unsigned char)(0x54|((contrast & 0x30) >> 4))); // Power/ICON/Contrast control
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x6C); // Follower control
    __delay_ms(200);
    ST7032_LCD_write(0x00, 0x38); // Function set
    __delay_us(100);
    ST7032_LCD_write(0x00, 0x0d); // Display ON/OFF control
    __delay_ms(2);
    ST7032_LCD_write(0x00, 0x01); // Clear display    

    __delay_ms(100);
}

void sendUSART(unsigned char c) {
    while (!TXSTAbits.TRMT);
    TXREG = c;
}

volatile unsigned char recvByte = 0;

void interrupt isr(void) {
    if (PIR1bits.RCIF) {
        PIR1bits.RCIF = 0; // clear interrupting flag
        if ((RCSTAbits.OERR) || (RCSTAbits.FERR)) {
            // overrun error or Framing error occurred

            // restart USART
            RCSTA = 0;    // disable USART
            RCSTA = 0x90; // enable USART

            recvByte = 0;
        } else {
            recvByte = RCREG;
        }
    }
}

void main(void) {    
    /* set a clock frequency */
    OSCCON = 0b01110010; // 8MHz
    
    /* set port status */
    ANSELA  = 0b00000000; // all of pins are digital
    ANSELB  = 0b00000000; // all of pins are digital
    TRISA   = 0b00000000; // all of pins are output
    TRISB   = 0b00010110; // RB0(RTS), RB1(SDA),RB4(SCL) are input
    PORTA   = 0b00000000; // initialize portA
    PORTB   = 0b00000000; // initialize portB, and RTS off
    
    I2C_Master_init();

    __delay_ms(100);    
    ST7032_LCD_init(36);
    ST7032_LCD_writeString("waiting...");

    /* setup USART */
    TXSTA   = 0x24; // TXEN(TX enable), BRGH(high speed)
    RCSTA   = 0x90; // SPEN(serial port enable), CREN(continually receive USART)
    BAUDCON = 0x08; // BRG16 (for 115.2 BAUD RATE)
    SPBRG   = 16; // actual rate: 115200 (data seat p303))
    RXDTSEL = 1; // RB2 to RX (data seat p.118)
    TXCKSEL = 1; // RB5 to TX (data seat p.118)

    /* interrupt */
    PIR1bits.RCIF   = 0; // clear interrupt flag
    PIE1bits.RCIE   = 1; // allow to interrupt USART
    INTCONbits.PEIE = 1; // allow interrupting peripheral
    INTCONbits.GIE  = 1; // allow global

    while (1) {
        
        unsigned char c = recvByte; 
        if (c) {
            recvByte = 0;

            ST7032_LCD_write(0x00, 0x01); // clear display
            __delay_ms(10);
            
            ST7032_LCD_write(0x40, c); // display received character from USART
        } else {
            sendUSART('.'); // send an idle character
            __delay_ms(3000);
        }
    }
}
