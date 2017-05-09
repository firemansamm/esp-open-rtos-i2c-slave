# ESP8266 I2C Slave
This repository contains a (very slow!) bit-bang implementation of the I2C slave protocol for the ESP8266 wifi module on esp-open-rtos, inspired by [kanflo's I2C master driver](https://github.com/kanflo/esp-open-rtos-driver-i2c).

This code is still really ugly and probably needs a lot of work :(

On its own, with the CPU clocked to 160MHz, this code achieves up to ~60KHz stable, but drops to ~35KHz when under load, which is thankfully just above the lower bound of `Wire.setClock` on an Arduino Uno.

### Potential improvements
- Using the I2S DMA
- Toggling the GPIOs through ASM to reduce clock cycles needed
- Fix bugs (?)
- Clean up the code

### Usage
````
void callback(uint8_t* data, uint8_t len) { ... } //data received in write operation

i2c_slave_init(0x4D, GPIO_ID_PIN(0), GPIO_ID_PIN(2), callback);
i2c_slave_set_buffer((uint8_t*)"hello", 5); //set buffer for read operations
````

This code is in the public domain.