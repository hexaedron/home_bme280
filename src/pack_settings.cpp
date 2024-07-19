#include <ch32v003fun.h>
#include <cstdint>

uint8_t getBrigtness(void)
{
    uint8_t brigthness = ((uint8_t)OB->Data0) >> 4;

    if(brigthness > 16)
        brigthness = 16;

    return brigthness;
}

uint16_t getScreenChangeSeconds_x5(void)
{
    uint16_t screenChangeSeconds_x5 = 0;
    screenChangeSeconds_x5  = (uint8_t)OB->Data0;
    screenChangeSeconds_x5  = screenChangeSeconds_x5 << 8;
    screenChangeSeconds_x5 |= (uint8_t)OB->Data1;

    if((screenChangeSeconds_x5 > 720) || (screenChangeSeconds_x5 == 0))
        screenChangeSeconds_x5 = 1;

    return screenChangeSeconds_x5;
}

void packSettings(uint8_t brigtnness, uint16_t screenChangeSeconds_x5, uint8_t& data0, uint8_t& data1)
{
    data0 = 0;
    data1 = (uint8_t)screenChangeSeconds_x5;

    data0  = (uint8_t)(screenChangeSeconds_x5 >> 8);
    data0 |= (brigtnness << 4);
}