#ifndef __I2C_SLAVE_H__
#define __I2C_SLAVE_H__

#include <stdint.h>
#include <stdbool.h>

typedef enum {
	IDLE,
	START,
	STOP,
	ADDR,
	OP,
	READDATA,
	WRITEDATA,
	IACK1,
	IACK2,
	WACK1,
	WACK2,
	RACK
} I2C_SLAVE_STATE;

typedef void (*READCALL)(uint8_t *data, uint8_t length);

void i2c_slave_init(uint8_t scl_pin, uint8_t sda_pin, READCALL callback);
void i2c_slave_set_buffer(uint8_t *data, uint8_t length);

#endif /* I2C_SLAVE_H */