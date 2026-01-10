#pragma once

#include <stdbool.h>

bool TempSensor_Init(void);
bool TempSensor_ReadCelsius(float *outTempC);
