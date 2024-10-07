#include <ch32v003fun.h>

void pushPressure(uint32_t* pressureHistory, uint8_t pressureHistoryHours, uint32_t currentPressure)
{
    uint8_t i;

    for (i = 0; i < (pressureHistoryHours - 1); i++)
    {
        pressureHistory[i] = pressureHistory[i + 1];
    }
    
    pressureHistory[pressureHistoryHours - 1] = currentPressure;
}