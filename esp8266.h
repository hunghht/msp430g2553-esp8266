/*
 * esp8266.h
 *
 *  Created on: Feb 24, 2015
 *      Author: Nischal
 */

#ifndef ESP8266_H_
#define ESP8266_H_

#include <msp430g2553.h>
#include <string.h>

void init_serial();
void serial_putsc(unsigned char value);
void serial_puts(char* String);
unsigned int wifiCommand_ack(char*,int,char*);
void wifiCommand_val(char*,int);
void init_esp(void);
void clear_buffer(void);
void mode1(void);
unsigned char receive_data(void);
void update_settings(short int* no_of_feeds,short int* feed_m,short int* feed_h,short int* feed_units);

#endif /* ESP8266_H_ */
