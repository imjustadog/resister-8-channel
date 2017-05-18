#define ADC_CONV1 LATCbits.LATC1
#define ADC_RDn1 LATCbits.LATC2
#define ADC_BUSYn1 PORTBbits.RB5

#define ADC_CONV2 LATBbits.LATB3
#define ADC_RDn2 LATBbits.LATB2
#define ADC_BUSYn2 PORTBbits.RB1

void InitLTC1859();
void ReadWriteLTC1859_1(int);
void ReadWriteLTC1859_2(int);