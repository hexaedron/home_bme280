#include "ch32v003fun.h"
#include "funny_defs.h"

#include "i2c_dma.h"
#include "BMP280.h"
#include "vk16k33.h"
#include "simpleTimer.h"
#include "tim2Encoder.h"

#include <stdbool.h>
#include <cstdlib>
#include <cstring>

enum displayMode_t
{
	printTemperature,
	printHumidity,
	printPressure,
	idle
};

#define SCREEN_CHANGE_INTERVAL 2000UL

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

	tim2Encoder enc(AFIO_PCFR1_TIM2_REMAP_NOREMAP);

	I2C_init();

	sensor.init_default_params();
	sensor.init();

	vk16k33 screen;
	screen.init();

	displayMode_t displayMode = displayMode_t::printTemperature;

	simpleTimer myTimer(SCREEN_CHANGE_INTERVAL);
	bool firstTime = true;

	while (true)
	{
		int delta = enc.getDelta();
		if(delta > 0)
			screen.incBrightness();
		if(delta < 0)
			screen.decBrightness();
		
		if(myTimer.ready() || firstTime)
		{
			firstTime = false;
			sensor.force_measurement();
			while (sensor.is_measuring())
			{
			};
			sensor.read_fixed();

			temperature = sensor.getTemp();
			humidity = sensor.getHumidity();
			pressure = sensor.getPressureMmHg();

			sensor.sleep();

			char buf[10];

			switch (displayMode)
			{
				case displayMode_t::printTemperature :
					itoa(temperature, buf, 10);
					if ((temperature > 0) && (temperature >= 1000))
					{
						screen.digit(ASCII_TO_INT(buf[0]), 0);
						screen.digit(ASCII_TO_INT(buf[1]), 1, true);
						screen.digit(ASCII_TO_INT(buf[2]), 2);
						screen.digit(10, 3);
					}
					else if((temperature > 0) && (temperature < 1000))
					{

						screen.digit(12, 0);
						screen.digit(ASCII_TO_INT(buf[0]), 1, true);
						screen.digit(ASCII_TO_INT(buf[1]), 2);
						screen.digit(10, 3);
					}
					screen.refresh();
					displayMode = displayMode_t::printHumidity;
				break;

				case displayMode_t::printHumidity :
					itoa(humidity, buf, 10);
					if(humidity >= 10000)
					{
						screen.digit(ASCII_TO_INT(buf[0]), 0);
						screen.digit(ASCII_TO_INT(buf[1]), 1, true);
						screen.digit(ASCII_TO_INT(buf[2]), 2);
					}
					else
					{
						screen.digit(12, 0);
						screen.digit(ASCII_TO_INT(buf[0]), 1, true);
						screen.digit(ASCII_TO_INT(buf[1]), 2);
					}
					screen.digit(11, 3);
					screen.refresh();
					displayMode = displayMode_t::printPressure;
				break;

				case  displayMode_t::printPressure :
					itoa(pressure / 256, buf, 10);
					screen.digit(ASCII_TO_INT(buf[0]), 0);
					screen.digit(ASCII_TO_INT(buf[1]), 1);
					screen.digit(ASCII_TO_INT(buf[2]), 2, true);
					itoa(pressure % 256, buf, 10);
					screen.digit(ASCII_TO_INT(buf[0]), 3);
					screen.refresh();
					displayMode = displayMode_t::printTemperature;
				break;
			
			default:
				break;
			}
		}
	}
}
