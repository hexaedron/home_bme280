#include <ch32v003fun.h>

#define PRESSURE_DELTA 128UL // ~0.5 mmHg

int8_t getPressureTrend(uint32_t* pressureHistory, uint8_t pressureHistoryHours, uint32_t currentPressure)
{
    uint8_t i;
    uint32_t min = UINT32_MAX, max = 0;

    for(i = 0; i < pressureHistoryHours; i++)
    {
        if(max < pressureHistory[i])
            max = pressureHistory[i];

        if((min > pressureHistory[i]) && (pressureHistory[i] != 0))
            min = pressureHistory[i];
    }

    if(currentPressure > (max + PRESSURE_DELTA))
        return 1;

    if(currentPressure < (min - PRESSURE_DELTA))
        return -1;

    return 0;
}