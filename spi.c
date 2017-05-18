#if defined(__dsPIC33F__)
#include <p33Fxxxx.h>
#elif defined(__PIC24H__)
#include <p24hxxxx.h>
#endif

#include "spi.h"

extern unsigned char res[16]; 


void delay_cvt()
{
	unsigned int i;
  	for(i=0;i<25;i++)
  	{
	   	asm("nop");	   		
    }	
 }

void InitLTC1859()
{
	TRISGbits.TRISG6 = 0;
	TRISGbits.TRISG7 = 1;
	TRISGbits.TRISG8 = 0;
	
    TRISCbits.TRISC1 = 0;
	TRISCbits.TRISC2 = 0;
	TRISBbits.TRISB5 = 1;

    TRISBbits.TRISB3 = 0;
	TRISBbits.TRISB2 = 0;
	TRISBbits.TRISB1 = 1;

	IFS2bits.SPI2IF = 0; // Clear the Interrupt Flag
	IEC2bits.SPI2IE = 0; // Disable the Interrupt
	// SPI1CON1 Register Settings
	SPI2CON1bits.PPRE = 2; // 4:1 primary prescale
	SPI2CON1bits.SPRE = 7; // 1:1 secondary prescale
	SPI2CON1bits.DISSCK = 0; // Internal Serial Clock is Enabled
	SPI2CON1bits.DISSDO = 0; // SDOx pin is controlled by the module
	SPI2CON1bits.MODE16 = 0; // Communication is byte-wide (8 bits)
	SPI2CON1bits.SMP = 0; // sample at the middle of clock
	SPI2CON1bits.CKE = 1; // sampling at rising edge,change at falling edge 
	SPI2CON1bits.CKP = 0; // Idle state for clock is a low level; 
	// active state is a high level
	SPI2CON1bits.MSTEN = 1; // Master mode Enabled
    SPI2CON2bits.FRMEN = 0; // frame disabled
    SPI2STATbits.SPIROV = 0; //clear overflow
	SPI2STATbits.SPIEN = 1; // Enable SPI module 
	// Interrupt Controller Settings
/*	
	SPI1STATbits.SPIEN = 0; 
	TRISFbits.TRISF2 = 0;
	
	SPI1CON1bits.DISSCK = 1; // Internal Serial Clock is Enabled
	SPI1CON1bits.DISSDO = 1; // SDOx pin is controlled by the module
*/
    ADC_RDn1 = 1;
    ADC_RDn2 = 1;
	ADC_CONV1 = 0;
	ADC_CONV2 = 0;
}

void ReadWriteLTC1859_1(int num)
{
    int index;
    if(num == 0)
       index = 6;
    else index = (num - 1) * 2;

    ADC_RDn1 = 0;
    ADC_CONV1 = 1;
	asm("nop");
    asm("nop");
    ADC_CONV1 = 0;
    //while(!ADC_BUSYn1){asm("nop");}
    delay_cvt();
	while(SPI2STATbits.SPITBF){asm("nop");}
	SPI2BUF = 0x10 * num;
	while(!SPI2STATbits.SPIRBF){asm("nop");}
	res[index] = SPI2BUF;
	while(SPI2STATbits.SPITBF){asm("nop");}
	SPI2BUF = 0x00;
	while(!SPI2STATbits.SPIRBF){asm("nop");}
	res[index + 1] = SPI2BUF;
	asm("nop");
	
    ADC_RDn1 = 1;
}

void ReadWriteLTC1859_2(int num)
{
    int index;
    if(num == 0)
       index = 14;
    else index = (num - 1) * 2 + 8;

    ADC_RDn2 = 0;
    ADC_CONV2 = 1;
	asm("nop");
    asm("nop");
    ADC_CONV2 = 0;
    //while(!ADC_BUSYn2){asm("nop");}
    delay_cvt();
	while(SPI2STATbits.SPITBF){asm("nop");}
	SPI2BUF = 0x10 * num;
	while(!SPI2STATbits.SPIRBF){asm("nop");}
	res[index] = SPI2BUF;
	while(SPI2STATbits.SPITBF){asm("nop");}
	SPI2BUF = 0x00;
	while(!SPI2STATbits.SPIRBF){asm("nop");}
	res[index + 1] = SPI2BUF;
	asm("nop");
	
    ADC_RDn2 = 1;
}