#if defined(__dsPIC33F__)
#include <p33Fxxxx.h>
#elif defined(__PIC24H__)
#include <p24hxxxx.h>
#endif
#include "timer.h"
#include "adc.h"
#include "sci.h"
#include "spi.h" 
#include "stdio.h"
#include "collect.h"

_FBS(0xCF);
_FSS(0xCF);
_FGS(0x07);
_FOSCSEL(0xA2);  //Select HS without PLL
_FOSC(0x46);  // HS oscilator, OSC2 is clock output
_FWDT(0xDF);
_FPOR(0xE7);
//_FICD(0xC3);
#define CLRWDT {__asm__ volatile ("CLRWDT");}




/****
四大指示灯
**/

#define COMM LATCbits.LATC14

#define WORK LATCbits.LATC13

#define STAT LATDbits.LATD0

 #define FAIL LATDbits.LATD11
 
 /**
 网口复位引脚
 
 ****/
 
#define Nrest LATFbits.LATF6
 
/****
MIAN_ID与IP地址的最后两位对应
**********/
#define BOARD_NUM 1 // Number of board
unsigned char MAIN_ID = 0x21;

int count_reset = 0; 


//#define N0 0.206572
//#define N1 0.413144
//#define N2 0.206572

//#define D1 -0.369527
//#define D2 0.195816


#define N0 1
#define N1 0
#define N2 0

#define D1 0
#define D2 0

unsigned char send_data[200];
char send_ascii[250];

char flag_ascii_or_bin = 'b';

unsigned int freq[BOARD_NUM][9],lq; 
//unsigned int temp[BOARD_NUM][9];
unsigned int temp_freq[8];	
 			  
unsigned int temp[BOARD_NUM][8]; // Temperature
 
unsigned char res[16]; 
int res_y[8][3];
int res_x[8][3]; 
int zero[8] = {0,0,0,0,0,0,0,0};
 
unsigned char work_enable = 0;//采集模块工作使能位
unsigned char send_enable = 0;
unsigned char uart2_enable = 0;//串口使能位
unsigned char uart1_enable = 0;//网口使能位

unsigned char speed = 'l';
unsigned char flag_tozero = 0;

void DELAY(unsigned int t)//t=1 6us
{
	unsigned int i,j;
  	for(i=0;i<20;i++)
  	{
	  	for(j=0;j<t;j++)
   		{
	   		asm("nop");	
    	}
   }	
} 

void UART2_Send(unsigned char str[], int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		U2TXREG = str[i];
		while((U2STA&0x0100)==0x0000){asm("nop");}
	}
}

void UART1_Send(unsigned char str[], int len)
{
	int i;
	for(i=0;i<len;i++)
	{
		U1TXREG = str[i];
		while((U1STA&0x0100)==0x0000){asm("nop");}
	}
}

int tick=0;
int Tick_60S=0;
int count_5 = 0;
int count_10 = 0;
int count_50 = 0;
int count_100 = 0;

void __attribute__((interrupt,no_auto_psv)) _T6Interrupt(void)  // 1ms interrupt
{
	IFS2bits.T6IF = 0;
    count_5 ++;

	if(speed == 'h')
	{
		work_enable = 1;
    }
	
    if(count_5 >= 5)
	{
    	if(speed == 'h')
		{
			send_enable = 1;
    	}
		count_5 = 0;
        count_10 ++;
    }

	if(count_10 >= 2)
	{
		if(speed == 'l')
		{
			work_enable = 1;
    	}
		count_10 = 0;
		count_50 ++;
	}

    if(count_50 >= 5)
	{
		if(speed == 'l')
		{
			send_enable = 1;
    	}   
		count_50 = 0;
	    Tick_60S++;
    }

    if(Tick_60S >= 1280) //60S
    {
        count_reset ++;
        Tick_60S = 0;
	}
	if(count_reset >= 15)
	{	
		Nrest=0;
		DELAY(5000);
		Nrest=1;
		DELAY(5000);
		count_reset = 0;
		UART2_Send("reset",5);
	}	
}

/******
网口中断


**********/

void __attribute__((interrupt,no_auto_psv)) _U1RXInterrupt(void)
{
	IFS0bits.U1RXIF = 0;
	unsigned char data[17];
	unsigned int UART_Timeout = 0;
	unsigned char i = 0;
	int nian,yue,ri,shi,fen,miao;
	//unsigned int num;
	data[0] = U1RXREG;
			 
	while(data[i]!='E')
	{
		i++;
		while((0==(U1STA&0x0001))&&(UART_Timeout<50000))
		{UART_Timeout++;}
		if(UART_Timeout>=50000)
		{
			data[i]='E'; // 如果接收超时，退出中断
		}
		else
		{
			data[i] = U1RXREG;
		}
		UART_Timeout = 0;
	}
	
	if( (i==16)&&(data[2]==0X01)&&(data[3]==0X02)&&(data[0]=='S') )
	{	
		STAT = 1;
		speed = 'l';
		flag_ascii_or_bin = 'b';
	    uart1_enable =1;
		count_reset = 0;	
	}//if(i==16)

	else if( (i==16)&&(data[2]==0X03)&&(data[3]==0X04)&&(data[0]=='S') )
	{	
		STAT = 1;
		speed = 'h';
		flag_ascii_or_bin = 'b';
	    uart1_enable =1;
		count_reset = 0;	
	}//if(i==16)	
	else if( (i==16)&&(data[2]==0X11)&&(data[3]==0X12)&&(data[0]=='S') )
	{	
		STAT = 0;
		speed = 'l';
		flag_ascii_or_bin = 'b';
	    uart1_enable =0;	
	}//if(i==16)

	else if( (i==16)&&(data[2]==0X13)&&(data[3]==0X14)&&(data[0]=='S') )
	{	
		STAT = 0;
		speed = 'h';
		flag_ascii_or_bin = 'b';
	    uart1_enable =0;	
	}//if(i==16)	
	else if( (i==16)&&(data[2]==0X05)&&(data[3]==0X06)&&(data[0]=='S') )
	{	
		flag_tozero = 1;	
	}//if(i==16)

	return;	
}//





/******
外部串口中断


**********/
int start_judge = 0;

void __attribute__((interrupt,no_auto_psv)) _U2RXInterrupt(void)
{
	unsigned int UART_Timeout = 0;
	unsigned char i = 0;
	unsigned char dat;
	unsigned char receive_buf[60];
	
	IFS1bits.U2RXIF = 0;
	dat = U2RXREG;
	
		if((start_judge == 0) && (dat == 'S'))
	    {
			start_judge = 1;
			receive_buf[0] = dat;
	    }
		else if((start_judge == 0) && (dat == 'C'))
	    {
			start_judge = 3;
	    }
	    else if(start_judge == 1)
		{
			while(receive_buf[i]!='E')
			{
				if(i == 0)
				{
					i = 1;
					receive_buf[i] = dat;
				}	
				i++;
				while((0==(U2STA&0x0001))&&(UART_Timeout<50000))
				{UART_Timeout++;}
				if(UART_Timeout>=50000)
				{
					receive_buf[i]='E'; // 如果接收超时，退出中断
				}
				else
				{
					receive_buf[i] = U2RXREG;
				}
				UART_Timeout = 0;
			}	
			start_judge = 0; 
			if((i==16)&&(receive_buf[2]==0X01)&&(receive_buf[3]==0X02)) 
			{	
				speed = 'l';
				uart2_enable =1;
				flag_ascii_or_bin = 'b';	
			}
			else if((i==16)&&(receive_buf[2]==0X03)&&(receive_buf[3]==0X04)) 
			{	
				speed = 'h';
				uart2_enable =1;
				flag_ascii_or_bin = 'b';	
			}
			else if((i==16)&&(receive_buf[2]==0X11)&&(receive_buf[3]==0X12)) 
			{	
				speed = 'l';
				uart2_enable =0;
				flag_ascii_or_bin = 'b';	
			}
			else if((i==16)&&(receive_buf[2]==0X13)&&(receive_buf[3]==0X14)) 
			{	
				speed = 'h';
				uart2_enable =0;
				flag_ascii_or_bin = 'b';	
			}
			else if((i==16)&&(receive_buf[2]==0X05)&&(receive_buf[3]==0X06)) 
			{	
				flag_tozero = 1;	
			}

		}
		
		else if((start_judge == 0) && (dat == '5'))
	    {
			start_judge = 2;
			receive_buf[0] = dat;
	    }
	    else if(start_judge == 2)
		{
			while((receive_buf[i] != '5') || (i == 0))
			{
				if(i == 0)
				{
					i = 1;
					receive_buf[i] = dat;
				}
				i++;
				while((0==(U2STA&0x0001))&&(UART_Timeout<50000))
				{UART_Timeout++;}
				if(UART_Timeout>=50000)
				{
					receive_buf[i]='5'; // 如果接收超时，退出中断
				}
				else
				{
					receive_buf[i] = U2RXREG;
				}
				UART_Timeout = 0;
			}	
			start_judge = 0; 
			if((i==49)&&(receive_buf[7]=='1')&&(receive_buf[10]=='2')) 
			{	
				uart2_enable =1;
				flag_ascii_or_bin = 'a';	
			}
		}
		else if(start_judge == 3)
		{
			start_judge = 0;
			if(dat == 'Q')
			{
				IEC1bits.U2RXIE = 0; // Enable UART2 RX interrupt
	            IEC1bits.U2TXIE = 0;
				IEC0bits.U1RXIE = 0; //  Enable UART1 RX interrupt
				IEC0bits.U1TXIE = 0;
				IEC2bits.T6IE = 0;
				IEC2bits.C1IE=0;
				C1INTEbits.RBIE=0;
				while(1); //饿狗，让狗来重启
			}
		}
		
	return;	
}

int main()
{
	int p,z;
	int i = 1000; // 扫频频率 473Hz~7.042kHz
	int k,j;
	int n,m;
	int s=0;
	int temp;
	unsigned char year,month,day,hour,minute,second;
    unsigned int humi_val_i,temp_val_i;
	unsigned char error,checksum;
    unsigned char read_temp;

    CLRWDT
	OSCCON = 0x2200;
 
 	TRISCbits.TRISC13 = 0;//LIGNT
 	TRISCbits.TRISC14 = 0; 
	TRISDbits.TRISD11 = 0; 
	TRISDbits.TRISD0  = 0; 
 
 	TRISFbits.TRISF6 = 0; //interet

	s_connectionreset();
    /*temp hr   */

    WORK=1;//COMMM1
    STAT=0;
    COMM=0;//COMMM1
    FAIL=0;
    Nrest=1;//
	InitTimer6();  //// Timer6 提供0.8s中断定时
    StartTimer6();
	InitSCI();
	InitLTC1859();

	CLRWDT

    ReadWriteLTC1859_1(0);
    DELAY(1);  // 600~9ms
    ReadWriteLTC1859_2(0);
    DELAY(1);

	while(1)
	{		
		CLRWDT
		
		if((U1STA & 0x000E) != 0x0000)
		{
			read_temp = U1RXREG;
			U1STAbits.OERR = 0;
        }
		
		if((U2STA & 0x000E) != 0x0000)
		{
			read_temp = U2RXREG;
			U2STAbits.OERR = 0;
        }
		
		if(work_enable ==1) 
		{	
	        CLRWDT

			for(n=0;n<3;n++)
			{
                ReadWriteLTC1859_1(n + 1);
				
				res_x[n][2] = res_x[n][1];
				res_x[n][1] = res_x[n][0];
				res_x[n][0] = (res[n * 2] << 8) + res[n * 2 + 1];

				res_y[n][2] = res_y[n][1];
				res_y[n][1] = res_y[n][0];
				res_y[n][0] = (N0 * res_x[n][0]) + (N1 * res_x[n][1]) + (N2 * res_x[n][2]) - (D1 * res_y[n][1]) - (D2 * res_y[n][2]);
				
                DELAY(1);				
			}
            ReadWriteLTC1859_1(0);
			
			res_x[n][2] = res_x[n][1];
			res_x[n][1] = res_x[n][0];
			res_x[n][0] = (res[n * 2] << 8) + res[n * 2 + 1];

			res_y[n][2] = res_y[n][1];
			res_y[n][1] = res_y[n][0];
			res_y[n][0] = (N0 * res_x[n][0]) + (N1 * res_x[n][1]) + (N2 * res_x[n][2]) - (D1 * res_y[n][1]) - (D2 * res_y[n][2]);
            
			DELAY(1);

			for(n=4;n<7;n++)
			{
                ReadWriteLTC1859_2(n + 1 - 4);

				res_x[n][2] = res_x[n][1];
				res_x[n][1] = res_x[n][0];
				res_x[n][0] = (res[n * 2] << 8) + res[n * 2 + 1];

				res_y[n][2] = res_y[n][1];
				res_y[n][1] = res_y[n][0];
				res_y[n][0] = (N0 * res_x[n][0]) + (N1 * res_x[n][1]) + (N2 * res_x[n][2]) - (D1 * res_y[n][1]) - (D2 * res_y[n][2]);
                
				DELAY(1);			
			}
            ReadWriteLTC1859_2(0);
			
			res_x[n][2] = res_x[n][1];
			res_x[n][1] = res_x[n][0];
			res_x[n][0] = (res[n * 2] << 8) + res[n * 2 + 1];

			res_y[n][2] = res_y[n][1];
			res_y[n][1] = res_y[n][0];
			res_y[n][0] = (N0 * res_x[n][0]) + (N1 * res_x[n][1]) + (N2 * res_x[n][2]) - (D1 * res_y[n][1]) - (D2 * res_y[n][2]);
            
            DELAY(1);
			work_enable = 0;
		}

		if(flag_tozero == 1) 
		{	
	        CLRWDT
			for(z = 0;z < 8;z ++)
			{
				zero[z] = 0.117582 * res_y[z][0];
			}
			flag_tozero = 0;
		}

		if(send_enable == 1)
		{
	        /*error=0;
	     	error+=s_measure((unsigned char*) &humi_val_i,&checksum,HUMI); 
	     	error+=s_measure((unsigned char*) &temp_val_i,&checksum,TEMP); 
	
			if(error!=0)
	            s_connectionreset(); 
			else
		        error=0;*/


			send_data[0]='S';
			send_data[1] = MAIN_ID;
	        s=2;

			for(n = 0;n < 8;n ++)
			{
				temp = 0.117582 * res_y[n][0] - zero[n];
				send_data[s] = temp >> 8;s++;
				send_data[s] = temp & 0x00FF ;s++;
			}

			send_data[s] = humi_val_i>>8;s++;
			send_data[s] = humi_val_i;s++;
			
			send_data[s] = temp_val_i>>8;s++;
			send_data[s] =temp_val_i;s++;
			
			send_data[s]='E';s++;

			if(uart2_enable ==1)
			{
				UART2_Send(send_data,s);
			}
			if(uart1_enable ==1)
   	        {
   	        	UART1_Send(send_data,s);
   	        }	
	
			send_enable = 0;
		}		
	}
	
	return 0;
}

