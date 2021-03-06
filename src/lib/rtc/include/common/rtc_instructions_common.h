#ifndef RTC_INSTRUCTIONS_COMMON_H
#define RTC_INSTRUCTIONS_COMMON_H

#include <stdint.h>
#include "rtc.h"

void rtc_common_translate_invoke(rtc_translationstate *ts, uint8_t opcode, uint8_t jvm_operand_byte0, uint8_t jvm_operand_byte1);
void rtc_common_translate_invokelight(uint8_t jvm_operand_byte0, uint8_t jvm_operand_byte1);
void rtc_common_translate_inc(uint8_t opcode, uint8_t jvm_operand_byte0, uint8_t jvm_operand_byte1, uint8_t jvm_operand_byte2);
void rtc_common_push_returnvalue_from_R22_if_necessary(uint8_t rettype);

#endif // RTC_INSTRUCTIONS_COMMON_H