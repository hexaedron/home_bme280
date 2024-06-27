#include <ch32v003fun.h>
#include "funny_defs.h"

#include "i2c_dma.h"
#include "BMP280.h"
#include "vk16k33.h"
#include "simpleTimer.h"
#include "tim2Encoder.h"
#include "optiondata.h"
#include "funny_time.h"

#include <stdbool.h>
#include <cstdlib>
#include <cstring>

enum displayMode_t
{
	printTemperature,
	printHumidity,
	printPressure
};

#define BRIGHTNESS_SAVE_TIMEOUT 5000UL

void setupMode(void);

tim2Encoder enc(AFIO_PCFR1_TIM2_REMAP_NOREMAP);
vk16k33 screen;
uint8_t screenChangeSeconds_x15 = OB->Data1;
bool firstTime = true;

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

	// Accroding to vk16k33 datasheet, we need to wait until it wakes up.
	Delay_Ms(1); 

	I2C_init();

	sensor.init_default_params();
	sensor.init();

	screen.init();
	screen.setBrightness(OB->Data0);

	displayMode_t displayMode = displayMode_t::printTemperature;

	if( (screenChangeSeconds_x15 > 240) )
		screenChangeSeconds_x15 = 1;

	simpleTimer32 screenChangeTimer((uint32_t)screenChangeSeconds_x15 * 15000UL);
	simpleTimer32 flashTimer(BRIGHTNESS_SAVE_TIMEOUT);

	while (true)
	{
		keyTick();
		
		int delta = enc.getDelta();
		if(delta > 0)
		{
			screen.incBrightness();
			flashTimer.start_int();
		}
		if(delta < 0)
		{
			screen.decBrightness();
			flashTimer.start_int();
		}
		
		if(flashTimer.ready() && (((uint8_t)OB->Data0 != screen.getBrightness()) || ((uint8_t)OB->Data1 != screenChangeSeconds_x15)))
		{
			FlashOptionData(screen.getBrightness(), screenChangeSeconds_x15);
			system_initSystick();
			screenChangeTimer.setPrd(screenChangeSeconds_x15 * 15000UL);
		}
		
		if(btnHeld())
		{
			setupMode();
		}

		bool wasClick = btnClick();
		if((screenChangeTimer.ready() || firstTime) || wasClick)
		{
			firstTime = false;
			if(wasClick) screenChangeTimer.start_int();
			wasClick = false;
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

void setupMode(void)
{
	simpleTimer32 refreshTimer(100UL);

	while (true)
	{
		keyTick();
		
		char buf[5];

		if(btnClick())
		{
			firstTime = true;
			return;
		}

		if(refreshTimer.ready())
		{
			uint8_t min = (screenChangeSeconds_x15 * 15) / 60;
			uint8_t sec = (screenChangeSeconds_x15 * 15) % 60;

			itoa(sec, buf, 10);
			if(sec > 0)
			{
				screen.digit(ASCII_TO_INT(buf[0]), 2);
				screen.digit(ASCII_TO_INT(buf[1]), 3);
			}
			else
			{
				screen.digit(ASCII_TO_INT(buf[0]), 2);
				screen.digit(ASCII_TO_INT(buf[0]), 3);
			}

			itoa(min, buf, 10);
			if(min < 10)
			{
				screen.digit(12, 0);
				screen.digit(ASCII_TO_INT(buf[0]), 1, true);
			}
			else
			{
				screen.digit(ASCII_TO_INT(buf[0]), 0);
				screen.digit(ASCII_TO_INT(buf[1]), 1, true);
			}

			screen.refresh();
		}

		int16_t delta = enc.getDelta();
		if(delta > 0)
		{
			if(screenChangeSeconds_x15 < 240)
			{
				screenChangeSeconds_x15++;
			}
		}
		else if (delta < 0)
		{
			if(screenChangeSeconds_x15 > 1)
			{
				screenChangeSeconds_x15--;
			}
		}
	}
}