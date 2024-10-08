#include <ch32v003fun.h>
#include "funny_defs.h"

#include "i2c_dma.h"
#include "BMP280.h"
#include "vk16k33.h"
#include "simpleTimer.h"
#include "tim2Encoder.h"
#include "optiondata.h"
#include "funny_time.h"

#include "pack_settings.h"

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
#define PRESSURE_HISTORY_HOURS  12

void setupMode(void);
int8_t getPressureTrend(uint32_t* pressureHistory, uint8_t pressureHistoryHours, uint32_t currentPressure);
void pushPressure(uint32_t* pressureHistory, uint8_t pressureHistoryHours, uint32_t currentPressure);

tim2Encoder enc(AFIO_PCFR1_TIM2_REMAP_NOREMAP);
vk16k33 screen;
uint16_t screenChangeSeconds_x5;
bool firstTime = true;

uint32_t pressureHistory[PRESSURE_HISTORY_HOURS] = {0};

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

	screenChangeSeconds_x5 = getScreenChangeSeconds_x5();

	bmp280 sensor;
	uint32_t pressure, humidity;
	int32_t temperature;

	// Accroding to vk16k33 datasheet, we need to wait until it wakes up.
	Delay_Ms(1); 

	I2C_init(400000, 0);

	sensor.init_default_params();
	sensor.init();

	screen.init();
	screen.setBrightness(getBrigtness());

	displayMode_t displayMode = displayMode_t::printTemperature;

	if( (screenChangeSeconds_x5 > 720) )
		screenChangeSeconds_x5 = 1;

	simpleTimer32 screenChangeTimer((uint32_t)screenChangeSeconds_x5 * 5000UL);
	simpleTimer32 flashTimer(BRIGHTNESS_SAVE_TIMEOUT);
	simpleTimer32 pressureHistoryTimer(3600UL * 1000UL);

	sensor.force_measurement();
	while (sensor.is_measuring())
	{
	};
	sensor.read_fixed();
	pressureHistory[PRESSURE_HISTORY_HOURS - 1] = sensor.getPressureMmHg();
	sensor.sleep();

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

		if(pressureHistoryTimer.ready())
		{
			sensor.force_measurement();
			while (sensor.is_measuring())
			{
			};
			sensor.read_fixed();
			pushPressure(pressureHistory, PRESSURE_HISTORY_HOURS, sensor.getPressureMmHg());
		}
		
		if(flashTimer.ready() && ((getBrigtness() != screen.getBrightness()) || (getScreenChangeSeconds_x5() != screenChangeSeconds_x5)))
		{
			uint8_t data0, data1;
			packSettings(screen.getBrightness(), screenChangeSeconds_x5, data0, data1);
			FlashOptionData(data0, data1);
			system_initSystick();
			screenChangeTimer.setPrd(screenChangeSeconds_x5 * 5000UL);
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
					screen.digit(ASCII_TO_INT(buf[2]), 2);
					switch (getPressureTrend(pressureHistory, PRESSURE_HISTORY_HOURS, pressure))
					{
						case 1:
							screen.digit(14, 3);
						break;

						case -1:
							screen.digit(13, 3);
						break;
					
						default:
							screen.digit(12, 3);
						break;
					}
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
			uint8_t min = (screenChangeSeconds_x5 * 5) / 60;
			uint8_t sec = (screenChangeSeconds_x5 * 5) % 60;

			itoa(sec, buf, 10);
			if(sec > 0)
			{
				if(sec < 10)
				{
					screen.digit(0, 2);
					screen.digit(ASCII_TO_INT(buf[0]), 3);
				}
				else
				{
					screen.digit(ASCII_TO_INT(buf[0]), 2);
					screen.digit(ASCII_TO_INT(buf[1]), 3);
				}
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
			if(screenChangeSeconds_x5 < 720)
			{
				screenChangeSeconds_x5++;
			}
		}
		else if (delta < 0)
		{
			if(screenChangeSeconds_x5 > 1)
			{
				screenChangeSeconds_x5--;
			}
		}
	}
}