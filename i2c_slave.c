/****
*
*	esp8266 i2c slave code - stable at 35khz at 160mhz processor clock
*	seems to be stable at ~60khz if wifi and other stuff is not in use
*
****/


#include <esp8266.h>
#include <esp/interrupts.h>
#include <espressif/esp_misc.h>
#include <FreeRTOS.h>
#include <task.h>
#include <string.h>
#include "i2c_slave.h"

static uint8_t _scl_pin, _sda_pin, sda = 1, scl = 1, *buf, *sendbuf, *tempbuf, data, len, idx, opr, sbuflen, ict, I2C_SLAVE_ADDRESS;
static bool pause_sda = false, wrt = false;
static I2C_SLAVE_STATE state;
static READCALL cb;

static IRAM inline bool read_scl(void) 		{ gpio_enable(_scl_pin, GPIO_INPUT); return gpio_read(_scl_pin); }
static IRAM inline bool read_sda(void) 		{ gpio_enable(_sda_pin, GPIO_INPUT); return gpio_read(_sda_pin); }
static IRAM inline void clear_scl(void) 	{ scl = 0; gpio_enable(_scl_pin, GPIO_OUT_OPEN_DRAIN); gpio_write(_scl_pin, 0); }
static IRAM inline void clear_sda(void)		{ sda = 0; pause_sda = 1; gpio_enable(_sda_pin, GPIO_OUT_OPEN_DRAIN); gpio_write(_sda_pin, 0); }
static IRAM inline void release_scl(void) 	{ gpio_enable(_scl_pin, GPIO_OUT_OPEN_DRAIN); gpio_write(_scl_pin, 1); }
static IRAM inline void release_sda(void)	{ pause_sda = 0; gpio_enable(_sda_pin, GPIO_OUT_OPEN_DRAIN); gpio_write(_sda_pin, 1); }
static IRAM inline void make_sda(bool on)	{ sda = on; pause_sda = 1; gpio_enable(_sda_pin, GPIO_OUT_OPEN_DRAIN); gpio_write(_sda_pin, on); }

void IRAM scl_isr(uint8_t gpio_num){
	scl = read_scl();

	//rising edge SCL - data stable
	if(scl == 1){
		switch(state){
			case ADDR:
				data = (data << 1) | sda;
				len++;
				if(len < 7) break;
				if(data != I2C_SLAVE_ADDRESS) state = IDLE; //address mismatch
				else state = OP;
			break;
			case OP:
				data = sda;
				state = IACK1;
			break;
			case IACK1:
				state = IACK2;
			break;
			case WRITEDATA:
				data = (data << 1) | sda;
				len++;
				if(len < 8) break;
				len = 0;
				buf[idx++] = data;
				data = 0;
				state = WACK1;
			break;
			case WACK1:
				state = WACK2;
			break;
			case READDATA:
				if(len < 8) break;
				//byte done
				idx++;
				len = 0;
				state = RACK1;
			break;
			case RACK:
				if(sda == 1) state = IDLE; //error/done?
				else state = READDATA; //more data follows
			break;
			default:
			break;
		}
	} else {
		//SCL low - time to edit
		switch(state){
			case IACK1:
				clear_sda();
			break;
			case IACK2:
				release_sda();
				opr = data;
				if(data == 0) {
					len = 0; idx = 0;
					state = WRITEDATA;
					wrt = true;
				}
				else {
					state = READDATA;
					wrt = false;
					idx = 0;
					len = 1;
					make_sda(((tempbuf[idx]) & 0x80) > 0);
					tempbuf[idx] <<= 1;
				}
			break;
			case WACK1:
				clear_sda();
			break;
			case WACK2:
				release_sda();
				state = WRITEDATA;
			break;
			case READDATA:
				make_sda(((tempbuf[idx]) & 0x80) > 0);
				len++;
				tempbuf[idx] <<= 1;
			break;
			case RACK:
				release_sda();
			break;
			default:
			break;
		}
	}
}

void IRAM sda_isr(uint8_t gpio_num){
	if(pause_sda) return;
	sda = read_sda();

	if(scl == 1) {
		if(sda == 1) {
			//STOP
			//state = STOP;
			state = IDLE;
			if(wrt) cb(buf, idx);
			else memcpy(tempbuf, sendbuf, sbuflen);
			len = 0; idx = 0; data = 0;
		}
		else {
			state = ADDR;
			len = 0; idx = 0; data = 0;
		}
	}
}

void i2c_slave_set_buffer(uint8_t *data, uint8_t length){
	memcpy(sendbuf, data, length);
	memcpy(tempbuf, data, length);
	sbuflen = length;
}

void i2c_slave_init(uint8_t addr, uint8_t scl_pin, uint8_t sda_pin, READCALL call){
	I2C_SLAVE_ADDRESS = addr;
	_scl_pin = scl_pin;
	_sda_pin = sda_pin;
	cb = call;
	state = IDLE;

	gpio_enable(_scl_pin, GPIO_INPUT);
	gpio_enable(_sda_pin, GPIO_INPUT);
	
	buf = (uint8_t*) malloc(256);
	sendbuf = (uint8_t*) malloc(256);
	tempbuf = (uint8_t*) malloc(256);
	data = 0;
	len = 0;
	ict = 0;

	gpio_set_interrupt(scl_pin, GPIO_INTTYPE_EDGE_ANY, scl_isr);
	gpio_set_interrupt(sda_pin, GPIO_INTTYPE_EDGE_ANY, sda_isr);
}
