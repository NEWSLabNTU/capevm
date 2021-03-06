#ifndef RTC_OPTIMISER_H
#define RTC_OPTIMISER_H

#include "types.h"

#if defined (AOT_STRATEGY_BASELINE)  || defined (AOT_STRATEGY_IMPROVEDPEEPHOLE)
void rtc_optimise(uint16_t *buffer, uint16_t **code_end);
#else
#define rtc_optimise(buffer, code_end)
#endif

#endif // RTC_OPTIMISER_H