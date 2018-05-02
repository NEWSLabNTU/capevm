
/*
 * config.h
 * 
 * Copyright (c) 2008-2010 CSIRO, Delft University of Technology.
 * 
 * This file is part of Darjeeling.
 * 
 * Darjeeling is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Darjeeling is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with Darjeeling.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#ifndef __config_h
#define __config_h

#define AVRORA
#include <stdint.h>
#include "types.h"
#include "djtimer.h"
#include "/Users/nielsreijers/src/rtc/avrora/src/avrora/monitors/AvroraTrace.h"
#include "/Users/nielsreijers/src/rtc/avrora/src/avrora/monitors/AvroraPrint.h"
#include "/Users/nielsreijers/src/rtc/avrora/src/avrora/monitors/AvroraTimer.h"
#include "/Users/nielsreijers/src/rtc/avrora/src/avrora/monitors/AvroraRTC.h"
#include "/Users/nielsreijers/src/rtc/avrora/src/avrora/monitors/AvroraProfiler.h"

void javax_rtc_RTC_void_startBenchmarkMeasurement_Native();
void javax_rtc_RTC_void_startBenchmarkMeasurement_AOT();
void javax_rtc_RTC_void_stopBenchmarkMeasurement();

#define HEAPSIZE 512

// 'Time slices' are 128 instructions
#define RUNSIZE 128

#define PACK_STRUCTS
#define ALIGN_16

// ---- AOT RELATED SWITCHES ----

// #define ARRAYINDEX_32BIT
#if defined(AOT_OPTIMISE_CONSTANT_SHIFTS_BY1) || defined(AOT_OPTIMISE_CONSTANT_SHIFTS_ALL_ONLY_SHIFT) || defined(AOT_OPTIMISE_CONSTANT_SHIFTS_ALL_MOVE_AND_SHIFT) || defined(AOT_OPTIMISE_CONSTANT_SHIFTS_GCC_LIKE)
#define AOT_OPTIMISE_CONSTANT_SHIFTS
#endif
// ---- AOT RELATED SWITCHES ----

#define avroraCallMethodTimerMark(x) avroraTimerMark(x)
// #define avroraCallMethodTimerMark(x) 

#define EXECUTION_DISABLEINTERPRETER_COMPLETELY 1
#define EXECUTION_FRAME_ON_STACK 1

// #define EXECUTION_PRINT_CALLS_AND_RETURNS 1
// #define EXECUTION_PRINT_WHEN_GC_RUNS
// #define EXECUTION_PRINT_GC_DETAILS 1
// #define EXECUTION_BREAK_IF_IN_INTERPRETER 1
#define DBG_PRINT_GC(x) // avroraPrintHex32(x)

/* Please see common/debug.h */
// #define DARJEELING_DEBUG
// #define DARJEELING_DEBUG_FRAME
// #define DARJEELING_DEBUG_MEM_TRACE
// #define DARJEELING_DEBUG_TRACE
// #define DARJEELING_DEBUG_CHECK_HEAP_SANITY
// #define DARJEELING_DEBUG_PERFILE
// #define DBG_DARJEELING true
// #define DBG_DARJEELING_GC true
// #define DBG_WKPF true
// #define DBG_WKPFGC true
// #define DBG_WKPFUPDATE true
// #define DBG_WKCOMM true
// #define DBG_WKREPROG true
// #define DBG_ZWAVETRACE true
// #define DBG_RTC true
// #define DBG_RTCTRACE true

void avr_serialPrintf(char * format, ...);
#define DARJEELING_PRINTF avr_serialPrintf

#define DARJEELING_PGMSPACE_MACRO

// Routing: choose 1
#define ROUTING_USE_NONE
// #define ROUTING_USE_DSDV
// #define ROUTING_USE_WUKONG

// Radios: choose as many as the routing protocol allows (max 1 for routing_none)
// #define RADIO_USE_ZWAVE
// #define RADIO_USE_XBEE

#endif
