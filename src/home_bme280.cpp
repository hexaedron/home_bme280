#include "ch32v003fun.h"
#include "cube_defs.h"

#include "i2c_dma.h"
#include "BMP280.h"
#include "vk16k33.h"

#include <stdbool.h>
#include <cstdlib>
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
	uint32_t pressure, humidity;
	int32_t temperature;

	I2C_init();

	sensor.init_default_params();
	sensor.init();

	vk16k33 screen;
	screen.init();

	while (true)
	{
		sensor.force_measurement();
		while (sensor.is_measuring())
		{
		};
		sensor.read_fixed();

		temperature = sensor.getTemp();
		humidity = sensor.getHumidity();
		pressure = sensor.getPressureMmHg();

		sensor.sleep();

		screen.clear();

		char buf[10];

		itoa(temperature, buf, 10);
		if ((temperature > 0) && (temperature >= 1000))
		{
			screen.digit(buf[0] - 48, 0);
			screen.digit(buf[1] - 48, 1, true);
			screen.digit(buf[2] - 48, 2);
			screen.digit(10, 3);
		}
		else if((temperature > 0) && (temperature < 1000))
		{

			screen.digit(12, 0);
			screen.digit(buf[0] - 48, 1, true);
			screen.digit(buf[1] - 48, 2);
			screen.digit(10, 3);
		}
		screen.refresh();
		Delay_Ms(1000);

		screen.clear();
		
		itoa(humidity, buf, 10);
		if(humidity >= 10000)
		{
			screen.digit(buf[0] - 48, 0);
			screen.digit(buf[1] - 48, 1, true);
			screen.digit(buf[2] - 48, 2);
		}
		else
		{
			screen.digit(12, 0);
			screen.digit(buf[0] - 48, 1, true);
			screen.digit(buf[1] - 48, 2);
		}
		screen.digit(11, 3);
		screen.refresh();
		Delay_Ms(1000);

	}
}
