#include "ch32v003fun.h"
#include "cube_defs.h"

#include "i2c_dma.h"
#include "BMP280.h"
#include "vk16k33.h"

#include <stdbool.h>
#include <cstdio>
#include <cstring>

// from system.cpp
void system_initSystick();
void system_initEXTI(uint32_t pin, bool risingEdge = true, bool fallingEdge = false);


int main()
{
	SystemInit();
	#ifdef WCH_FAST_INTERRUPT_ENABLED
		__set_INTSYSCR(0x3); // [Experimental] enable fast interrupt feature	
	#endif
	system_initSystick();

	// We use button on PC6, so we need to init it and turn on interrupt.	
	funGpioInitAll();
	funPinMode(PC6, GPIO_Speed_In | GPIO_CNF_IN_FLOATING);
	
	bmp280 sensor;
	volatile uint32_t  pressure,	humidity;	
	volatile int32_t temperature;

	I2C_init();

	sensor.init_default_params();
	sensor.init();

	vk16k33 screen;
	screen.init();

	uint8_t i = 0;
	while (true)
	{		
		sensor.force_measurement();
		while(sensor.is_measuring()) {};	 
			sensor.read_fixed();

		temperature = sensor.getTemp();
		humidity = sensor.getHumidity();
		pressure = sensor.getPressureMmHg();	
		
		sensor.sleep();	

		screen.clear();
        screen.digit( i, 0);
		screen.digit( i, 1);
		screen.digit( i, 2, true);
		screen.digit( i++, 3);
		screen.refresh();
		if(i>=12) i = 0;

		Delay_Ms(1000);
	}
}
