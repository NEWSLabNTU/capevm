#include "debug.h"
#include "types.h"
#include "panic.h"
#include "heap.h"
#include "global_id.h"
#include "execution.h"
#include "execution_instructions.h"
#include "rtc.h"

// TMPRTC
extern int16_t *intStack;
extern ref_t *refStack;

void RTC_INVOKEVIRTUAL_OR_INTERFACE(dj_local_id localId, uint8_t nr_ref_args) {
    AVRORATRACE_DISABLE();
	DEBUG_LOG(DBG_RTC, "RTC_INVOKEVIRTUAL_OR_INTERFACE %d %d %d\n", localId.infusion_id, localId.entity_id, nr_ref_args);
	DEBUG_LOG(DBG_RTC, "RTC_INVOKEVIRTUAL_OR_INTERFACE %p %p\n", intStack, refStack);

	DO_INVOKEVIRTUAL(localId, nr_ref_args);

    // If we called a JVM method, run until it's done.
    while (!dj_exec_currentMethodIsRTCCompiled())
        dj_exec_run(RUNSIZE);
    AVRORATRACE_ENABLE();
}

void RTC_INVOKESPECIAL(dj_local_id localId) {
    AVRORATRACE_DISABLE();
	DEBUG_LOG(DBG_RTC, "RTC_INVOKESPECIAL %d %d\n", localId.infusion_id, localId.entity_id);
	DEBUG_LOG(DBG_RTC, "RTC_INVOKESPECIAL %p %p\n", intStack, refStack);
	dj_global_id globalId = dj_global_id_resolve(dj_exec_getCurrentInfusion(),  localId);
	callMethod(globalId, true); // call to callMethod in execution.c

    // If we called a JVM method, run until it's done.
    while (!dj_exec_currentMethodIsRTCCompiled())
        dj_exec_run(RUNSIZE);
    AVRORATRACE_ENABLE();
}

void RTC_INVOKESTATIC(dj_local_id localId) {
    AVRORATRACE_DISABLE();
    DEBUG_LOG(DBG_RTC, "RTC_INVOKESTATIC %d %d\n", localId.infusion_id, localId.entity_id);
    DEBUG_LOG(DBG_RTC, "RTC_INVOKESTATIC %p %p\n", intStack, refStack);
    dj_global_id globalId = dj_global_id_resolve(dj_exec_getCurrentInfusion(),  localId);
    callMethod(globalId, false); // call to callMethod in execution.c

    // If we called a JVM method, run until it's done.
    while (!dj_exec_currentMethodIsRTCCompiled())
        dj_exec_run(RUNSIZE);
    AVRORATRACE_ENABLE();
}


ref_t RTC_NEW(dj_local_id localId) {
    AVRORATRACE_DISABLE();
    DEBUG_LOG(DBG_RTC, "RTC_NEW %d %d free: %d\n", localId.infusion_id, localId.entity_id, dj_mem_getFree());
    ref_t rv = DO_NEW(localId);
    AVRORATRACE_ENABLE();
    return rv;
}

ref_t RTC_LDS(dj_local_id localId) {
    AVRORATRACE_DISABLE();
    DEBUG_LOG(DBG_RTC, "RTC_LDS %d %d\n", localId.infusion_id, localId.entity_id);
    ref_t rv = DO_LDS(localId);
    AVRORATRACE_ENABLE();
    return rv;
}

ref_t RTC_ANEWARRAY(dj_local_id localId, uint16_t size) {
    AVRORATRACE_DISABLE();
    DEBUG_LOG(DBG_RTC, "RTC_NEWARRAY %d %d %d\n", localId.infusion_id, localId.entity_id, size);
    ref_t rv = DO_ANEWARRAY(localId, size);
    AVRORATRACE_ENABLE();
    return rv;
}

int16_t RTC_INSTANCEOF(dj_local_id localId, ref_t ref) {
    AVRORATRACE_DISABLE();
    DEBUG_LOG(DBG_RTC, "RTC_INSTANCEOF %d %d %d\n", localId.infusion_id, localId.entity_id, ref);
    int16_t rv = DO_INSTANCEOF(localId, ref);
    AVRORATRACE_ENABLE();
    return rv;
}

void RTC_CHECKCAST(dj_local_id localId, ref_t ref) {
    AVRORATRACE_DISABLE();
    DEBUG_LOG(DBG_RTC, "RTC_CHECKCAST %d %d %d\n", localId.infusion_id, localId.entity_id, ref);
    if (DO_INSTANCEOF(localId, ref) == 0)
        dj_panic(DJ_PANIC_CHECKCAST_FAILED);
    AVRORATRACE_ENABLE();
}
