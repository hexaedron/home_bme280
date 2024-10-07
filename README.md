# home_bme280

A small 3-in-one thermometer, barometer and gigrometer for indoor use.
Built with ch32v003 MCU, BME280 sensor and vk16k33 i2c 14-segment display modules.

## Usage
When powered on, the device starts cycling through 3 screens: Temperature in Celsius, Humidity and air pressure in mm Hg.
* To set the brightness, rotate the encoder.
* To set the screen change interval, press and hold the encoder for 2+ seconds, choose the desired interval from 5s to 60 min and then click it again.
* To force the immediate screen change, click the encoder.

## Assembly
Assembly is pretty straightforward. You should just note that PCB is a little bit redundant. A first my idea was to add an LDR resistor, but i've abandoned that idea later. So you can omit R_LDR and J_LDR. The firmware doesn't use them.
I've also made some extra pins availiable as J_EXT. Feel free to use them to extend functionality if you want.

## Build
You will need [ch32v00fun](https://github.com/cnlohr/ch32v003fun/) and [funny_libs](https://github.com/hexaedron/funny_libs) to build it. I prefer using official WCH toolchain. If you prefer using vanilla risc-v GCC, then remove `#define WCH_FAST_INTERRUPT_ENABLED` from `funny_defs.h`.

![photo](/Pictures/pic1.jpg)
![photo](/Pictures/pic2.jpg)
![photo](/Pictures/pic3.jpg)
![photo](/Pictures/back.jpg)
