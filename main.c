#include <msp430g2553.h>
#include "esp8266.h"

#define MOTOR BIT6
#define ESP_EN BIT0

unsigned char sec = 0, min = 0, hour = 0;

unsigned char status_flags = 0x01, feed_offset_min, feed_offset_hour;

short int num_of_feeds = 0, feed_min[9] = {0,0,0,0,0,0,0,0,0}, feed_hour[9] = {0,0,0,0,0,0,0,0,0},
        num_of_units[9] = {0,0,0,0,0,0,0,0,0};

void flash_erase(int *addr)
{
    __disable_interrupt();               // Disable interrupts. This is important, otherwise,
    // a flash operation in progress while interrupt may
    // crash the system.
    while(BUSY & FCTL3);                 // Check if Flash being used
    FCTL2 = FWKEY + FSSEL_1 + FN3;       // Clk = SMCLK/4
    FCTL1 = FWKEY + ERASE;               // Set Erase bit
    FCTL3 = FWKEY;                       // Clear Lock bit
    *addr = 0;                           // Dummy write to erase Flash segment
    while(BUSY & FCTL3);                 // Check if Flash being used
    FCTL1 = FWKEY;                       // Clear WRT bit
    FCTL3 = FWKEY + LOCK;                // Set LOCK bit
    __enable_interrupt();
}

void flash_write(int *addr)
{
    int i;
    flash_erase(addr);
    //  int* addr1 = addr;
    __disable_interrupt();
    FCTL2 = FWKEY + FSSEL_1 + FN0;       // Clk = SMCLK/4
    FCTL3 = FWKEY;                       // Clear Lock bit
    FCTL1 = FWKEY + WRT;                 // Set WRT bit for write operation

    //  *addr++ = 50;         // copy value to flash
    //  *addr++ = 51;         // copy value to flash

    *addr++ = num_of_feeds;
    for(i = 0; i < 10; i++){
        *addr++ = feed_min[i];
        *addr++ = feed_hour[i];
        *addr++ = num_of_units[i];
    }

    FCTL1 = FWKEY;                        // Clear WRT bit
    FCTL3 = FWKEY + LOCK;                 // Set LOCK bit
    while(BUSY & FCTL3);
    __enable_interrupt();
}

void flash_read(int *addr)
{
    char i;
    num_of_feeds = *addr++;
    for(i = 0; i < 10; i++){
        feed_min[i] =  *addr++;
        feed_hour[i] =  *addr++;
        num_of_units[i] =  *addr++;
    }
}

void config_clock(void)
{
    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    BCSCTL3 |= LFXT1S_0 + XCAP_3;
}

void config_timer0(void)
{
    TA0CCTL2 &= ~CCIE;
    TA0CCTL0 = CCIE;
    TA0CCR0 = 32767;
    TA0CTL = TACLR + TASSEL_1 + MC_1;
}

void config_timer1(int period)
{
    TA1CCTL2 &= ~CCIE;
    TA1CCTL0 = CCIE;
    TA1CCR0 = period;
    TA1CTL = TACLR + TASSEL_1 + MC_0;
}

unsigned char update_rtc(void)
{
    unsigned char check_flag;
    //uart_txChar(sec + 0x30);
    if (sec >= 60)
    {
        sec = 0;
        ++min;
        check_flag = 1;
        if (min >= 60)
        {  min = 0;
        ++hour;
        check_flag = 1;
        if (hour >= 24)
            hour = 0;
        }
    }
    else
        check_flag = 0;
    return(check_flag);
}

void config_ports(void)
{
    P1DIR |= MOTOR;
    P1OUT &=~MOTOR;
    ADC10CTL0 = ADC10SHT_3 + ADC10ON + ADC10IE; // ADC10ON, interrupt enabled
    ADC10CTL1 = INCH_3;                       // input A3
    ADC10AE0 |= 0x08;                         // P1.3 ADC option select
}

unsigned char check_for_feed(void){
    unsigned char i;
    for(i = 0; i<num_of_feeds; i++)
    {
        if((hour == feed_hour[i]) & (min == feed_min[i]))
            return (i+1);
    }

    return 0;

}

unsigned char process_ir(void){
    unsigned char i;//, char1, char2, char3, char4;
    unsigned int sum = 0, avg = 0, smallest_ir_val = 0, highest_ir_val = 1024;
    for(i = 0; i<10; i++)
    {
        ADC10CTL0 |= ENC + ADC10SC;             // Sampling and conversion start
        __bis_SR_register(CPUOFF + GIE);        // LPM0, ADC10_ISR will force exit

        if(i == 0) {
            smallest_ir_val = ADC10MEM;
            highest_ir_val = ADC10MEM;
        }
        else if (i == 1){
            if(ADC10MEM < smallest_ir_val)
                smallest_ir_val = ADC10MEM;
            else if(ADC10MEM > highest_ir_val)
                highest_ir_val = ADC10MEM;
        }
        else {
            if((ADC10MEM > smallest_ir_val)&(ADC10MEM < highest_ir_val))
                sum += ADC10MEM;
            else if(ADC10MEM > highest_ir_val)
            {
                sum += highest_ir_val;
                highest_ir_val = ADC10MEM;
            }
            else if(ADC10MEM < smallest_ir_val)
            {
                sum += smallest_ir_val;
                smallest_ir_val = ADC10MEM;
            }
        }
    }
    avg = sum/8;

//    char1 = avg/1000;
//    char2 = (avg - char1*1000)/100;
//    char3 = (avg - (char1*1000 + char2*100))/10;
//    char4 = (avg - (char1*1000 + char2*100 + char3*10));

//    uart_txChar(char1 + 0x30);
//    uart_txChar(char2 + 0x30);
//    uart_txChar(char3 + 0x30);
//    uart_txChar(char4 + 0x30);
//    uart_txChar(0x0A);
//    uart_txChar(0x0D);

    if(avg >= 600 & avg <= 650) {
        return 1;
    }

    else if(avg >= 580){
        return 2;
    }

    else if(avg < 580 & avg >= 550){
        return 3;
    }

    else if(avg < 550 & avg != 0){
        return 4;
    }

    else if(avg > 650 & avg != 1024){
        return 5;
    }

    else if(avg == 0){
        return 6;
    }

    return 7;

}

void start_timer1(int period)
{       TA1R = 0;
config_timer1(period);
TA1CTL = TACLR + TASSEL_1 + MC_1;
}

void delay_seconds(unsigned char seconds){
    unsigned char i;
    for(i = 0; i< seconds; i++)
        start_timer1(32767);
}

void run_motor(unsigned char operation_mode){

    P1OUT |=MOTOR;
    start_timer1(20000);
    _BIS_SR(LPM3_bits + GIE);

    switch(operation_mode){
    case 1: P1OUT |=MOTOR;
    start_timer1(20000);
    break;
    case 2: P1OUT |=MOTOR;
    start_timer1(25000);
    break;
    case 3: P1OUT |=MOTOR;
    start_timer1(30000);
    break;
    case 4:
        break;
    case 5:
        break;
    case 6:
        break;
    case 7:
        break;

    }

}

void main(void) {

    unsigned char operation_mode,j,x,index = 0;

    int *addr = (int *)0x01000 ; // Address of the flash memory segment starting

    WDTCTL = WDTPW | WDTHOLD;   // Stop watchdog timer
    config_clock();
    IE1 |= OFIE;
    config_timer0();
    config_ports();

    //flash_write(addr);
    flash_read(addr);

    init_serial();
    init_esp();
    mode1();

    while(1)
    {
        x = receive_data();
        if(x==1)
        {
            update_settings(&num_of_feeds,&feed_min[0],&feed_hour[0],&num_of_units[0]);
            flash_write(addr);
        }
        if(status_flags &  0x01){ // RUNNING mode
            if((x = update_rtc()) == 1){
                if((index = check_for_feed()) != 0)
                {
                    status_flags |= 0x08;
                    for(j = 0;j < num_of_units[index-1]; j++){

                        //uart_txString("Turn\r\n");
                        operation_mode = process_ir();
                        run_motor(operation_mode);
//                      start_timer1(32767);
                        delay_seconds(3);
                        _BIS_SR(LPM3_bits + GIE);
//                      __delay_cycles(10000);
                    }
                }
               // uart_txChar(num_of_units[index-1] + 0x30);
               // uart_txChar(index + 0x30);
               // uart_txChar(0x0A);
               // uart_txChar(0x0D);
            }
        }

        else if(status_flags & 0x02){ // SETTING mode

        }

        else if(status_flags & 0x04){ // COMMUNICATION mode
        }

        _BIS_SR(LPM3_bits + GIE);

    }
}

#pragma vector=NMI_VECTOR
__interrupt void nmi_ (void)
{
    while(IFG1 & OFIFG)  // wait for OSCFault to clear
    {
        IFG1 &= ~OFIFG;
        __delay_cycles(100);
    }
    IE1 |= OFIE;                              // Enable Osc Fault
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void timer(void)
{
    CCR0 += 32768;
    ++sec;
    _BIC_SR_IRQ(LPM3_bits);
}

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
  __bic_SR_register_on_exit(CPUOFF);        // Clear CPUOFF bit from 0(SR)
}

#pragma vector=TIMER1_A0_VECTOR
__interrupt void timer_motor(void)
{
    P1OUT&=~MOTOR;
    TA1CTL = MC_0;
    __bic_SR_register_on_exit(CPUOFF);
}
