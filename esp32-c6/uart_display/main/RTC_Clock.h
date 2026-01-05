#pragma once

#include <stdbool.h>
#include <stddef.h>

bool RTC_Clock_Init(void);
void RTC_Clock_GetTime(char *timeStr, size_t len);
void RTC_Clock_GetDate(char *dateStr, size_t len);
