#include"esp8266.h"

char* x;
int val=0;
unsigned int i=0;
char string1[200]={0};
unsigned char rx_flag=0;

void init_serial()
{
    P1SEL = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
    P1SEL2 = BIT1 + BIT2 ;                     // P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 104;                            // 1MHz 9600
    UCA0BR1 = 0;                              // 1MHz 9600
    UCA0MCTL = UCBRS0;                        // Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    IE2 |= UCA0RXIE;
}

void serial_putsc(unsigned char value)
{
    while (!(IFG2&UCA0TXIFG));                 // USCI_A0 TX buffer ready?
    UCA0TXBUF = value;                     // TX -> RXed character
}

void serial_puts(char* String)
{
    while(*String != '\n')
    {
        serial_putsc(*String);
        String++;
    }
    serial_putsc(*String);
}

void clear_buffer(void)
{
    for(i=199;i>0;i--)
    {
        string1[i] = 0;
    }
    string1[0] = 0;
    i=0;
    rx_flag=0;
}

unsigned int wifiCommand_ack(char* sCmd,int waitTm,char* sTerm)
{
//    char lp = 0;
//    while(lp < waitTm)
//    {
        serial_puts(sCmd);
        _BIS_SR(LPM0_bits + GIE);
        while(waitTm--)
            _delay_cycles(1000);
        if(strstr(string1,sTerm) != 0)
        {
            for(;i>0;i--)
                string1[i] = 0;
            string1[0] = 0;
            i=0;
            rx_flag=0;
            return 1;
        }
//        _delay_cycles(1000000);
//        lp++;
//    }
    for(;i>0;i--)
        string1[i] = 0;
    string1[0] = 0;
    i=0;
    rx_flag=0;
    return 0;
}

void wifiCommand_val(char* sCmd,int waitTm)
{
    serial_puts(sCmd);
    _BIS_SR(LPM0_bits + GIE);
    while(waitTm--)
        _delay_cycles(1000);
    //strcpy(string1,ret);
}

void init_esp()
{
    P1DIR|=BIT0;
    P1OUT&=~BIT0;
    _delay_cycles(10000);
    P1OUT|=BIT0;
    _delay_cycles(3000000);
    wifiCommand_ack("AT\r\n",10,"OK");
    wifiCommand_ack("AT+CIPCLOSE\r\n",10,"OK");
    val = wifiCommand_ack("AT+RST\r\n",2000,"ready");
    wifiCommand_ack("AT+CWMODE=3\r\n",10,"OK");
//    wifiCommand_val("AT+GMR\r\n",50);
//    clear_buffer();
}

void mode1()
{
    wifiCommand_ack("AT+CIPMUX=1\r\n",10,"OK");
    wifiCommand_ack("AT+CIPSERVER=1,9998\r\n",10,"OK");
}

unsigned char receive_data()
{
    if(rx_flag==1)
    {
        _delay_cycles(100000);
        return 1;
    }
    return 0;
}

void update_settings(short int* no_of_feeds,short int* feed_m,short int* feed_h,short int* feed_units)
{
    char* temp;
    unsigned int index;
    //decode protocol
    temp = strchr(string1,'f');
    index = temp-string1+1;
    *no_of_feeds = string1[index]-48;
    for(i=0; i<*no_of_feeds; i++)
    {
        temp = strchr(string1,'a'+i);
        index = temp-string1+1;
        *(feed_h+i) = (string1[index]-48)*10+(string1[index+1]-48);
        *(feed_m+i) = (string1[index+2]-48)*10+(string1[index+3]-48);
        feed_units[i] = string1[index+4]-48;
    }
    //prtocol decoded
    clear_buffer();
    rx_flag=0;
}

#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
{
    string1[i++] = UCA0RXBUF;
    _BIC_SR_IRQ(LPM0_bits);
    rx_flag=1;
}
