#ifndef DHT11_H
#define DHT11_H

#include "stm32f4xx_hal.h"

#define DHT11_PORT GPIOE
#define DHT11_PIN GPIO_PIN_5

extern uint8_t RHI, TCI;


void microDelay(uint16_t delay);
uint8_t DHT11_Start(void);
uint8_t DHT11_Read(void);

#endif
