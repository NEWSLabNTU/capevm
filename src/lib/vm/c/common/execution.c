/*
 * execution.c
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
 

/**
 * \defgroup execution Execution
 * \ingroup execution
 * @{
 * \page execution_description
 *
 * The execution module is responsible for executing instructions, task switching, and exception handling. The internal state of the execution module can be
 * accessed through methods.
 *
 * Before execution can begin the execution engine must be passed a dj_vm object. This object contains information about loaded infusions, running threads and
 * the like. Then one of the threads from the dj_vm object has to be enabled, after which execution can start.
 *
 * Methods that start with dj_exec are accessible from other modules and expose the execution module's internal state to the outside world. The dj_exec_stack
 * methods are used to control the state of the operand stack. Native methods use these to pop arguments off the stack, and optionally push a result
 * onto the stack.
 *
 * When the dj_exec_run(nrOpcodes) method is called the execution engine will execute at most [nrOpcodes] instructions. Execution can terminate prematurely
 * for instance when a thread terminates, when an uncaught exception is thrown, or when a thread is put to sleep with Thread.sleep().
 *
 * Because Darjeeling uses a double-ended stack, there are separate stack manipulation methods for shorts, integers and references. Note that integer arithmetic
 * using a 16-bit stack width is not fully implemented at this time.
 *
 */

#include <alloca.h>
#include <string.h>

#include "execution.h"
#include "execution_instructions.h"
#include "types.h"
#include "parse_infusion.h"
#include "infusion.h"
#include "object.h"
#include "array.h"
#include "heap.h"
#include "vm.h"
#include "vm_gc.h"
#include "global_id.h"
#include "debug.h"
#include "panic.h"
#include "hooks.h"
#include "core.h"

// platform-specific configuration
#include "config.h"

// generated at infusion time
#include "jlib_base.h"

// platform specific
#include "pointerwidth.h"

#include "opcodes.h"
#include "opcodes.c"

#include "rtc_safetychecks_core_part.h"

// currently selected Virtual Machine context
dj_vm *currentVm;
dj_thread *dj_currentThread;

// global variables for quick access
//static dj_thread *currentThread;

// execution state
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static int16_t *localIntegerVariables;
static uint16_t pc;
static dj_di_pointer code;
#endif

int16_t *intStack;
ref_t *refStack;
ref_t *localReferenceVariables;


bool dj_exec_use_rtc = true;

static int nrOpcodesLeft;
#ifdef DARJEELING_DEBUG
static uint32_t totalNrOpcodes;
static uint16_t oldPc;
char name[16];
#endif
#ifdef DARJEELING_DEBUG_TRACE
static int callDepth = 0;
#endif

#ifdef AVRORA
#define AVRORATRACE_DISABLE()    avroraTraceDisable();
#define AVRORATRACE_ENABLE()     avroraTraceEnable();
#else
#define AVRORATRACE_DISABLE()
#define AVRORATRACE_ENABLE()
#endif


//if it is tossim we need a bunch of getter setters,
//because tossim considers global variables in all nodes to be shared
/**
 * Tells the execution engine which VM is currently running. In principle this should be called once in the main.
 * @param _vm the virtual machine to set as the executing VM
 */

void dj_exec_setVM(dj_vm *_vm)
{
#ifdef DARJEELING_DEBUG
	totalNrOpcodes=0;
#endif
	currentVm = _vm;
}

/**
 * Tells the execution loop to stop after it has finished interpreting the current instruction. The next instruction
 * will not be fetched.
 */
inline void dj_exec_breakExecution() {
	nrOpcodesLeft = -1;
}


/**
 * Saves execution state (stack pointer, pc) in the given frame struct. This method
 * is used in context switching and method invocations/returns.
 * @param frame a dj_frame struct in which to save the current state
 */
static inline void dj_exec_saveLocalState(dj_frame *frame) {
	if (frame != NULL) {
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
		frame->pc = pc;
#endif
		frame->saved_intStack = intStack;
		frame->saved_refStack = refStack;
	}
}


void verifyAllFrames() {
	dj_frame *frame = dj_exec_getCurrentThread()->frameStack;

	while (frame != NULL && frame > (dj_frame *)0x0b00) {
		avroraPrintHex32(0xfae00000);
		avroraPrintPtr(frame);
		ref_t *localReferenceVariables = dj_frame_getLocalReferenceVariables(frame);
		dj_di_pointer methodImpl = dj_global_id_getMethodImplementation(frame->method);
		uint16_t numberOfRefVariables = dj_di_methodImplementation_getReferenceLocalVariableCount(methodImpl);

		for (int i=0; i<numberOfRefVariables; i++) {
			avroraPrintHex32(0x10ca1000);
			if (localReferenceVariables[i] != NULL) {
				dj_mem_guardValidHeapPointerPointer(&(localReferenceVariables[i]));
			}
		}

		ref_t *refStackFinger = dj_frame_getReferenceStackBase(frame, methodImpl);
		ref_t *end_of_refStack = frame == dj_exec_getCurrentThread()->frameStack ? refStack : frame->saved_refStack;
		while (refStackFinger < end_of_refStack) {
			avroraPrintHex32(0x5ac5ac00);
			if (*refStackFinger != NULL) {
				dj_mem_guardValidHeapPointerPointer(refStackFinger);
			}
			refStackFinger++;
		}

		frame = frame->parent;
	}
}

/**
 * Loads execution state (stack pointer, pc, code pointer, local variables) from a dj_frame struct. This method
 * is used in context switching and method invocations/returns.
 * @param frame the frame to load
 */
#ifdef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static inline void dj_exec_loadLocalState(dj_frame *frame) {
// avroraCallMethodTimerMark(50);
	intStack = frame->saved_intStack;
	refStack = frame->saved_refStack;

// avroraCallMethodTimerMark(51);
	localReferenceVariables = dj_frame_getLocalReferenceVariables(frame);
// avroraCallMethodTimerMark(52);
}
#else
static inline void dj_exec_loadLocalState(dj_frame *frame) {
	dj_di_pointer methodImpl = dj_global_id_getMethodImplementation(frame->method);

	code = dj_di_methodImplementation_getData(methodImpl);
	pc = frame->pc;

	intStack = frame->saved_intStack;
	refStack = frame->saved_refStack;

	localReferenceVariables = dj_frame_getLocalReferenceVariables(frame);
	localIntegerVariables = dj_frame_getLocalIntegerVariables(frame, methodImpl);
}
#endif

dj_frame *dj_exec_getCurrentFrame() {
	dj_frame *ret = NULL;
	dj_thread *thread = dj_exec_getCurrentThread();
	if( thread==NULL ) {
		DEBUG_LOG(DBG_DARJEELING, "Thread NULL. No frame.\n");
	} else {
		ret = thread->frameStack;
	}
	return ret;
}


#ifdef DARJEELING_DEBUG_FRAME
static void dj_exec_debugInt16( const char *desc, int16_t *data, uint8_t num, int increment ) {
	int idx;

	DARJEELING_PRINTF("%s: %p, %d values", desc, data, num);
	if( (data!=NULL) && (num>0) ) {
		DARJEELING_PRINTF(" (");
		for( idx=0; idx<num; idx++ ) {
			if( idx>0 ) {
				DARJEELING_PRINTF(", ");
			}
			DARJEELING_PRINTF("%#x", data[idx*increment] );
		}
		DARJEELING_PRINTF(")");
	}
	DARJEELING_PRINTF(".\n");
}

static void dj_exec_debugRef( const char *desc, ref_t *data, uint8_t num, int increment ) {
	int idx;

	DARJEELING_PRINTF("%s: %p, %d values", desc, data, num);
	if( (data!=NULL) && (num>0) ) {
		DARJEELING_PRINTF(" (");
		for( idx=0; idx<num; idx++ ) {
			if( idx>0 ) {
				DARJEELING_PRINTF(", ");
			}
			DARJEELING_PRINTF("%p", data[idx*increment] );
		}
		DARJEELING_PRINTF(")");
	}
	DARJEELING_PRINTF(".\n");
}

void dj_exec_dumpFrame( dj_frame *frame ) {
	int numLocalInts, numLocalRefs, numIntParams, numRefParams;
	int16_t *intParams;
	ref_t *refParams;

	DARJEELING_PRINTF("Frame = %p.\n", frame);
	if( frame!=NULL ) {
		DARJEELING_PRINTF(" PC=%#x, saved_IntStack=%p, saved_RefStack=%p, Parent=%p\n",
					frame->pc,
					frame->saved_intStack, frame->saved_refStack, frame->parent );
		dj_infusion_getName(frame->method.infusion, name, 16);
		DARJEELING_PRINTF(" Method.infusion=%s, Method.id=%d\n", name, frame->method.entity_id);

		dj_di_pointer methodImpl = dj_global_id_getMethodImplementation(frame->method);

		// calculate the size of the frame to create
		numLocalInts = dj_di_methodImplementation_getIntegerLocalVariableCount(methodImpl);
		numLocalRefs = dj_di_methodImplementation_getReferenceLocalVariableCount(methodImpl);
		int localVariablesSize = (numLocalRefs * sizeof(ref_t)) + (numLocalInts * sizeof(int16_t));

		int size =
			sizeof(dj_frame) +
			(dj_di_methodImplementation_getMaxStack(methodImpl) * sizeof(int16_t)) +
			localVariablesSize;
		DARJEELING_PRINTF(" Size of struct: %lu, max.stack: %d, local var's: %d bytes.\n",
				sizeof(dj_frame),
				(dj_di_methodImplementation_getMaxStack(methodImpl) * sizeof(int16_t)),
				localVariablesSize );
		DARJEELING_PRINTF(" Frame total size is %d bytes, so frame ends at %p.\n", size, ( ((void *)frame)+size) );

		dj_exec_debugInt16(" local int variables ",
							dj_frame_getLocalIntegerVariables(frame, methodImpl),
							numLocalInts, -1);
		dj_exec_debugRef(  " local ref. variables",
							dj_frame_getLocalReferenceVariables(frame),
							numLocalRefs, 1);

		numIntParams
				= dj_di_methodImplementation_getIntegerArgumentCount(methodImpl);
		numRefParams
				= dj_di_methodImplementation_getReferenceArgumentCount(methodImpl)
						+ ((dj_di_methodImplementation_getFlags(methodImpl)
								& FLAGS_STATIC) ? 0 : 1);
		if (frame->parent != NULL) {
			intParams = dj_frame_getIntegerStack(frame->parent) + numIntParams;
			refParams = dj_frame_getReferenceStack(frame->parent) - numRefParams;
		} else {
			intParams = NULL;
			refParams = NULL;
		}
		dj_exec_debugInt16(" integer   parameters", intParams, numIntParams, -1);
		dj_exec_debugRef(  " reference parameters", refParams, numRefParams, 1);

		DARJEELING_PRINTF(" local int stack base: %p.\n", dj_frame_getStackEnd(frame));
		dj_exec_debugInt16(" local integer stack ", dj_frame_getIntegerStackBase(frame), dj_frame_getIntegerStackBase(frame) - frame->saved_intStack, -1);

		DARJEELING_PRINTF(" local ref stack base: %p.\n", dj_frame_getStackStart(frame));
		dj_exec_debugRef(  " local reference stack", dj_frame_getReferenceStackBase(frame), frame->saved_refStack - dj_frame_getReferenceStackBase(frame), 1);
	}
}

void dj_exec_dumpFrameTrace( dj_frame *frame ) {
	dj_exec_dumpFrame(frame);
	if( frame!=NULL ) {
		DARJEELING_PRINTF("--- Parent frame follows ---\n");
		dj_exec_dumpFrameTrace(frame->parent);
	}
}

dj_frame *dj_exec_dumpExecutionState() {
	int16_t *orgIntStack;
	ref_t *orgRefStack;
	dj_frame *frame = dj_exec_getCurrentFrame();

	DARJEELING_PRINTF("--- Current execution state follows ---\n");
	if( frame!=NULL ) {
		orgIntStack = dj_frame_getIntegerStackBase(frame);
		DARJEELING_PRINTF(" current int stack base: %p.\n", orgIntStack);
		dj_exec_debugInt16(" current integer stack ", intStack, (orgIntStack - intStack), 1);

		orgRefStack = dj_frame_getReferenceStackBase(frame);
		DARJEELING_PRINTF(" current ref stack base: %p.\n", orgRefStack);
		dj_exec_debugRef(  " current reference stack", refStack, (refStack - orgRefStack), -1);
	} else {
		DARJEELING_PRINTF(" current integer stack : %p\n", intStack);
		DARJEELING_PRINTF(" current reference stack: %p\n", refStack);
		DARJEELING_PRINTF(" Couldn't determine stack depths, because frame==NULL.\n");
	}
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
	DARJEELING_PRINTF(" local integer   variables: %p\n", localIntegerVariables);
#endif
	DARJEELING_PRINTF(" local reference variables: %p\n", localReferenceVariables);
	DARJEELING_PRINTF(" PC:%#x  code:%p  this:%p  #OP-codes left:%d\n", pc, code, this, nrOpcodesLeft);

	DARJEELING_PRINTF("--- Current execution state ends ---\n");
	return frame;
}

void dj_exec_debugCurrentFrame() {
	dj_frame *frame = dj_exec_dumpExecutionState();
	DARJEELING_PRINTF("---- Current frame follows ----\n");
	dj_exec_dumpFrame( frame );
	DARJEELING_PRINTF("----  Current frame ends   ----\n\n");
}

void dj_exec_debugFrameTrace() {
	dj_frame *frame = dj_exec_dumpExecutionState();
	DARJEELING_PRINTF("---- Frame trace starts ----\n");
	dj_exec_dumpFrameTrace( frame );
	DARJEELING_PRINTF("---- End of frame trace ----\n\n");
}
#endif // DARJEELING_DEBUG_FRAME

/**
 * Deactivates a thread by saving the execution state (program counter, stack, etc) into the top execution frame.
 * This method is used in context switching.
 * @see dj_exec_saveLocalState()
 * @param thread the thread to deactivate
 */
void dj_exec_deactivateThread(dj_thread *thread) {
	// store program counter and stack pointers
	if (thread == NULL)
		return;
#if defined(DARJEELING_DEBUG_FRAME) && 0
	DARJEELING_PRINTF("%s:%d\n", __FILE__, __LINE__ );
	DARJEELING_PRINTF("---- Frame before 'save' follows ----\n");
	dj_exec_dumpFrame( dj_exec_getCurrentFrame() );
	DARJEELING_PRINTF("---- Frame before 'save' ends    ----\n");
#endif

	if (thread->frameStack != NULL)
		dj_exec_saveLocalState(thread->frameStack);

#if defined(DARJEELING_DEBUG_FRAME) && 0
	dj_exec_debugCurrentFrame();
#endif
}

/**
 * Activates a thread by loading the execution state (program counter, stack, etc) from the top execution frame.
 * This method is used in context switching.
 * @see dj_exec_loadLocalState()
 * @param thread the thread to deactivate
 */
void dj_exec_activate_thread(dj_thread *thread) {

	if (thread == NULL)
		return;

	dj_exec_setCurrentThread(thread);

	if (thread->frameStack != NULL) {
		dj_exec_loadLocalState(thread->frameStack);
	} else {
		DEBUG_LOG(DBG_DARJEELING, "Thread frame NULL, cannot activate\n");
		dj_panic(DJ_PANIC_ILLEGAL_INTERNAL_STATE_THREAD_FRAME_NULL);
	}
#if defined(DARJEELING_DEBUG_FRAME) && 0
	DARJEELING_PRINTF("%s:%d\n", __FILE__, __LINE__ );
	dj_exec_debugCurrentFrame();
#endif
}

void dj_exec_updatePointers() {
	dj_exec_setVM(dj_mem_getUpdatedPointer(dj_exec_getVM()));
	dj_exec_setCurrentThread(dj_mem_getUpdatedPointer(dj_exec_getCurrentThread()));
}

/**
 * Returns the current infusion which is the infusion containing the method that's currently executing. This infusion
 * serves as a context for instructions, as any local ID's contained in them are relative to it.
 * @return the infusion containing the method that's currently executing
 */
dj_infusion * dj_exec_getCurrentInfusion() {
#ifndef DARJEELING_DEBUG_FRAME
	DEBUG_LOG(DBG_DARJEELING, "\tCur. infusion read from %p->%p\n", dj_exec_getCurrentThread(), dj_exec_getCurrentThread()->frameStack);
#endif
	return dj_exec_getCurrentThread()->frameStack->method.infusion;
}

#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
/**
 * Fetches a byte from the code pointer. Increases the PC by 1.
 * TODO make quicker using code++
 */
static inline uint8_t fetch() {
	uint8_t bytecode = dj_di_getU8(code + pc);
	pc++;
	return bytecode;
}

/**
 * Peeks a 16-bit value from the code area, doesn't increase the PC.
 */
static inline int16_t peek16() {
	return (dj_di_getU8(code + pc) << 8) | dj_di_getU8(code + pc + 1);
}

/**
 * Peeks a 16-bit value from the code area at PC+n, doesn't increase the PC.
 * @param n the offset in bytes from the current PC at which to peek
 */
static inline int16_t peekn16(int n) {
	return (dj_di_getU8(code + pc + n) << 8) | dj_di_getU8(code + pc + 1 + n);
}

/**
 * Fetches a 16-bit value from the code area and increases the PC by 2.
 */
static inline int16_t fetch16() {
	// TODO speed up using getU16
	uint16_t ret = (dj_di_getU8(code + pc) << 8) | dj_di_getU8(code + pc + 1);
	pc += 2;
	return ret;
}

/**
 * Fetches a 32-bit value from the code area and increases the PC by 4.
 */
static inline int32_t fetch32() {
	int32_t ret = ((uint32_t) dj_di_getU8(code + pc) << 24)
			| ((uint32_t) dj_di_getU8(code + pc + 1) << 16)
			| ((uint32_t) dj_di_getU8(code + pc + 2) << 8)
			| (uint32_t) dj_di_getU8(code + pc + 3);

	pc += 4;
	return ret;
}

/**
 * Fetches a 64-bit value from the code area and increases the PC by 8.
 */
static inline int64_t fetch64() {
	int64_t ret = ((uint64_t) dj_di_getU8(code + pc) << 56)
			| ((uint64_t) dj_di_getU8(code + pc + 1) << 48)
			| ((uint64_t) dj_di_getU8(code + pc + 2) << 40)
			| ((uint64_t) dj_di_getU8(code + pc + 3) << 32)
			| ((uint64_t) dj_di_getU8(code + pc + 4) << 24)
			| ((uint64_t) dj_di_getU8(code + pc + 5) << 16)
			| ((uint64_t) dj_di_getU8(code + pc + 6) << 8)
			| (uint64_t) dj_di_getU8(code + pc + 7);

	pc += 8;
	return ret;
}

/**
 * Peeks a 32-bit value from the code area at PC+n, doesn't increase the PC.
 * @param n the offset in bytes from the current PC at which to peek
 */
static inline uint32_t peekn32(int n) {
	return ((uint32_t) dj_di_getU8(code + pc + n) << 24)
			| ((uint32_t) dj_di_getU8(code + pc + n + 1) << 16)
			| ((uint32_t) dj_di_getU8(code + pc + n + 2) << 8)
			| (uint32_t) dj_di_getU8(code + pc + n + 3);
}

/**
 * Fetches a dj_local_id value from the code area and increases the PC by 2.
 */
static inline dj_local_id dj_fetchLocalId() {
	dj_local_id ret;
	ret.infusion_id = fetch();
	ret.entity_id = fetch();
	return ret;
}
#endif // EXECUTION_DISABLEINTERPRETER_COMPLETELY

/**
 * Pushes a short (16 bit) onto the runtime stack
 */
static inline void pushShort(int16_t value) {
	intStack--;	
	*(intStack+1) = value;
}

/**
 * Pushes an int (32 bit) onto the runtime stack
 */
static inline void pushInt(int32_t value) {
	intStack -= 2;
	*((int32_t*) (intStack+1)) = value;
}

/**
 * Pushes a long (64 bit) onto the runtime stack
 */
static inline void pushLong(int64_t value) {
	intStack -= 4;
	*((int64_t*) (intStack+1)) = value;
}

/**
 * Pushes a reference onto the runtime stack
 */
static inline void pushRef(ref_t value) {
	refStack++;
	*(refStack-1) = value;
}

/**
 * Pops a short (16 bit) from the runtime stack
 */
static inline int16_t popShort()
{
	intStack++;
	int16_t ret = *intStack;
	return ret;
}

/**
 * Pops an int (32 bit) from the runtime stack
 */
static inline int32_t popInt() {
	intStack++;
	int32_t ret = *(int32_t*) intStack;
	intStack += 1; // Popping an Int frees up an additional 16bit slot
	return ret;
}

/**
 * Pops a long (64 bit) from the runtime stack
 */
static inline int64_t popLong() {
	intStack++;
	int64_t ret = *(int64_t*) intStack;
	intStack += 3; // Popping an Int frees up three additional 16bit slots
	return ret;
}

/**
 * Pops a reference from the runtime stack
 */
static inline ref_t popRef() {
	refStack--;
	ref_t ret = *refStack;
	return ret;
}

/**
 * Returns the topmost 16 bit short element on the integer stack, does not change the stackpointer.
 */
static inline int16_t peekShort() {
	return *(intStack+1);
}

/**
 * Returns the topmost 32 bit integer element on the integer stack, does not change the stackpointer.
 */
static inline int32_t peekInt() {
	return *(int32_t*) (intStack+1);
}

/**
 * Returns the topmost 64 bit long element on the integer stack, does not change the stackpointer.
 */
// 20140410 NR: Comment unused function
//static inline int64_t peekLong() {
//	return *(int64_t*) (intStack+1);
//}


/**
 * Returns the topmost element on the reference stack, does not change the stackpointer.
 */
static inline ref_t peekRef() {
	return *(refStack-1);
}

/**
 * Peeks the reference stack at a specified depth. For example, if the stack equals
 * [a, b, c, d, e, f], peekDeepRef(0) would return 'a' (same as peek()), and peekDeepRef(3) would return 'c'.
 * Does not change the stackpointer
 */
static inline ref_t peekDeepRef(int depth) {
	return *(refStack - depth - 1);
}

uint16_t dj_exec_getNumberOfObjectsOnReferenceStack(dj_frame * frame) {
	return frame->saved_refStack - dj_frame_getReferenceStackBase(frame, dj_global_id_getMethodImplementation(frame->method));	
}

/**
 * Pushes a short onto the runtime stack. Can be used by functions outside the execution module to interact with the
 * running program.
 * @param value the integer value to push.
 */
void dj_exec_stackPushShort(int16_t value)
{
	pushShort(value);
}

/**
 * Pushes an int onto the runtime stack. Can be used by functions outside the execution module to interact with the
 * running program.
 * @param value the integer value to push.
 */
void dj_exec_stackPushInt(int32_t value)
{
	pushInt(value);
}

/**
 * Pushes a long onto the runtime stack. Can be used by functions outside the execution module to interact with the
 * running program.
 * @param value the integer value to push.
 */
void dj_exec_stackPushLong(int64_t value)
{
	pushLong(value);
}

/**
 * Pushes a reference onto the runtime stack. Can be used by functions outside the execution module to interact with the
 * running program. Corruping the stack can lead to crashes, use with care!
 * @param value the reference value to push.
 */
void dj_exec_stackPushRef(ref_t value)
{
	pushRef(value);
}

/**
 * Pops a short from the runtime stack. Can be used by functions outside the execution module to interact with the
 * running program. Corruping the stack can lead to crashes, use with care!
 * @return the top value of the runtime stack.
 */
int16_t dj_exec_stackPopShort()
{
	return popShort();
}

/**
 * Pops an int from the runtime stack. Can be used by functions outside the execution module to interact with the
 * running program. Corruping the stack can lead to crashes, use with care!
 * @return the top value of the runtime stack.
 */
int32_t dj_exec_stackPopInt()
{
	return popInt();
}

/**
 * Pops a long from the runtime stack. Can be used by functions outside the execution module to interact with the
 * running program. Corruping the stack can lead to crashes, use with care!
 * @return the top value of the runtime stack.
 */
int64_t dj_exec_stackPopLong()
{
	return popLong();
}

/**
 * Pops a reference from the runtime stack. Can be used by functions outside the execution module to interact with the
 * running program. Corruping the stack can lead to crashes, use with care!
 * @return the top value of the runtime stack.
 */
ref_t dj_exec_stackPopRef() {
	return popRef();
}

/**
 * Returns the top of the stack as a short value, but does not change the stackpointer. Can be used by functions outside
 * the execution module to interact with the running program.
 * @return the top value of the runtime stack.
 */
int16_t dj_exec_stackPeekShort() {
	return peekShort();
}

/**
 * Returns the top of the stack as an integer value, but does not change the stackpointer. Can be used by functions outside
 * the execution module to interact with the running program.
 * @return the top value of the runtime stack.
 */
int32_t dj_exec_stackPeekInt() {
	return peekInt();
}

/**
 * Returns the top of the stack as a long value, but does not change the stackpointer. Can be used by functions outside
 * the execution module to interact with the running program.
 * @return the top value of the runtime stack.
 * /
int64_t dj_exec_stackPeekLong() {
	return peekLong();
}*/

/**
 * Returns the top of the stack as a reference value, but does not change the stackpointer. Can be used by functions outside
 * the execution module to interact with the running program.
 * @return the top value of the runtime stack.
 */
ref_t dj_exec_stackPeekRef() {
	return peekRef();
}


/**
 * Returns the n's element of the stack as a reference value, but does not change the stackpointer. Can be used by functions outside
 * the execution module to interact with the running program.
 * @return the top value of the runtime stack.
 */
ref_t dj_exec_stackPeekDeepRef(int depth) {
	return peekDeepRef(depth);
}

/**
 * Set local reference variable at index
 * @param index local variable slot number
 * @param value 16 bit short value
 */
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static inline void setLocalShort(int index, int16_t value) {
	// local ints grow down (so we don't need to know the types of the int arguments and worry about flipping the endianness)
  	localIntegerVariables[-index] = value;
}
#endif

/**
 * Returns 16 bit short local variable at index
 * @param index local variable slot number
 */
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static inline int16_t getLocalShort(int index) {
	// local ints grow down (so we don't need to know the types of the int arguments and worry about flipping the endianness)
  	return localIntegerVariables[-index];
}
#endif

/**
 * Set local reference variable at index
 * @param index local variable slot number
 * @param value 32 bit integer value
 */
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static inline void setLocalInt(int index, int32_t value) {
	// local ints grow down (so we don't need to know the types of the int arguments and worry about flipping the endianness)
	// substract -1 to find the start of the 32 bit int.
	*(int32_t*) (localIntegerVariables - index - 1) = value;
}
#endif

/**
 * Returns 32 bit integer local variable at index
 * @param index local variable slot number
 */
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static inline int32_t getLocalInt(int index) {
	// local ints grow down (so we don't need to know the types of the int arguments and worry about flipping the endianness)
	// substract -1 to find the start of the 32 bit int.
	return *(int32_t*) (localIntegerVariables - index - 1);
}
#endif

/**
 * Set local reference variable at index
 * @param index local variable slot number
 * @param value 64-bit long value
 */
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static inline void setLocalLong(int index, int64_t value) {
	// local ints grow down (so we don't need to know the types of the int arguments and worry about flipping the endianness)
	// substract -3 to find the start of the 64 bit int.
	*(int64_t*) (localIntegerVariables - index - 3) = value;
}
#endif

/**
 * Returns 64-bit long local variable at index
 * @param index local variable slot number
 */
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static inline int64_t getLocalLong(int index) {
	// local ints grow down (so we don't need to know the types of the int arguments and worry about flipping the endianness)
	// substract -3 to find the start of the 64 bit int.
	return *(int64_t*) (localIntegerVariables - index - 3);
}
#endif

/**
 * Set local reference variable at index
 * @param index local variable slot number
 * @param value reference value
 */
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static inline void setLocalRef(int index, ref_t value) {
	localReferenceVariables[index] = value;
}
#endif

/**
 * Returns local reference variable at index
 * @param index local variable slot number
 */
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
static inline ref_t getLocalRef(int index) {
	return localReferenceVariables[index];
}

/**
 * Branches to PC + offset.
 */
static inline void branch(int16_t offset) {
	pc += offset;
}
#endif


/**
 * Copies the parameters from the caller's stack to
 * the callee's local variables.
 * @param frame the callee's frame
 * @param methodImplId the callee's methodimpl entity_id
 */
static inline void dj_exec_passParameters(dj_frame *frame, dj_di_pointer methodImpl, uint8_t numberOfIntArguments, uint8_t numberOfRefArguments) {
	int16_t *newFrameLocalIntegerVariables = dj_frame_getLocalIntegerVariables(frame, methodImpl);
	ref_t *newFrameLocalReferenceVariables = dj_frame_getLocalReferenceVariables(frame);

	DEBUG_LOG(DBG_DARJEELING, " refs %d ints %d\n", numberOfRefArguments, numberOfIntArguments);
	DEBUG_LOG(DBG_DARJEELING, " intStack %p, refStack %p\n", intStack, refStack);
	for (int i=0; i<numberOfIntArguments; i++) {
		// int stack grows down
		uint16_t intValue = intStack[ +numberOfIntArguments - i ];
		DEBUG_LOG(DBG_DARJEELING, " copy int %d\n", intValue);
		newFrameLocalIntegerVariables[-i] = intValue; // local ints grow down (so we don't need to know the types of the int arguments and worry about flipping the endianness)
	}
	for (int i=0; i<numberOfRefArguments; i++) {
		// ref stack grows up
		ref_t refValue = refStack[ -numberOfRefArguments + i ];
		DEBUG_LOG(DBG_DARJEELING, " copy ref %d\n",refValue);
		newFrameLocalReferenceVariables[i] = refValue;
	}
}

/**
 * Returns from a method. The current execution frame is popped off the thread's frame stack. If there are no other
 * frames to execute, the thread ends. Otherwise control is switched to the underlying caller frame.
 */
// This only contains retval so we can avoid callMethod from storing the return value in a local var first, then calling returnFromMethod, then returing that local var.
// This takes up space in the stack frame while this allows it to stay in registers.
static inline uint32_t returnFromMethod(uint32_t retval) {
// avroraCallMethodTimerMark(20);
	AVRORATRACE_DISABLE();
	avroraRTCRuntimeMethodCallReturn();

#ifdef EXECUTION_FRAME_ON_STACK
	dj_thread_popFrame();
#else // EXECUTION_FRAME_ON_STACK
	// pop frame from frame stack and dealloc it
	dj_frame_destroy(dj_thread_popFrame());
#endif // EXECUTION_FRAME_ON_STACK

	// check if there are elements on the call stack
	if (dj_exec_getCurrentThread()->frameStack == NULL) {
		// done executing (exited last element on the call stack)
		dj_exec_getCurrentThread()->status = THREADSTATUS_FINISHED;
		dj_exec_breakExecution();
	} else {
		// perform context switch.
		// dj_exec_activate_thread(dj_exec_getCurrentThread()); This just ends up setting vm->currentThread to the value it already has, and then calling dj_exec_loadLocalState... Just call it directly.
		dj_exec_loadLocalState(dj_exec_getCurrentThread()->frameStack);
#ifdef EXECUTION_VERIFY_FRAMES_ON_JVM_RETURN
		verifyAllFrames();
#endif
	}
// avroraCallMethodTimerMark(21);
	return retval;
}

/**
 * Returns true if the current frame belongs to a RTC compiled method.
 * Used by dj_exec_run to return control to RTC code after running a JVM method called from RTC code.
 */
bool dj_exec_currentMethodIsRTCCompiled() {
	return !dj_mem_isHeapPointer(intStack);
}

/**
 * Enters a method. The method may be either Java or native. If the method is
 * native, the native handler of the method's containing infusion will be
 * called.
 * If the method is not native, a new frame is created and a context switch is
 * performed.
 * @param methodImplId a global id pointing to the method to be executed
 * @param virtualCall indicates if the call is a virtual or static call. In the
 * case of a virtual call the object the method belongs to is on the stack and
 * should be handled as an additional parameter. Should be either 1 or 0.
 */
void callMethod(dj_global_id methodImplId, bool virtualCall)
{
	// get a pointer in program space to the method implementation block
	// from the method's global id
	dj_di_pointer methodImpl = dj_global_id_getMethodImplementation(methodImplId);
	uint8_t flags = dj_di_methodImplementation_getFlags(methodImpl);

	callMethodFast(methodImplId, methodImpl, flags, virtualCall);
}

void callMethodFast(dj_global_id methodImplId, dj_di_pointer methodImpl, uint8_t flags, bool virtualCall)
{
	// check if the method is a native methods
	if ((flags & FLAGS_NATIVE) != 0)
	{
		callNativeMethod(methodImplId, methodImpl, virtualCall);
	} else {
		dj_global_id_with_flags methodImplIdWithFlags;
		methodImplIdWithFlags.infusion = methodImplId.infusion;
		methodImplIdWithFlags.entity_id = methodImplId.entity_id;
		methodImplIdWithFlags.flags = flags;
		uint32_t retval = callJavaMethod(methodImplIdWithFlags, methodImpl, dj_frame_size(methodImpl));
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
		const native_method_function_t *handlers = methodImplId.infusion->native_handlers;
		native_method_function_t handler = (handlers != NULL ? methodImplId.infusion->native_handlers[methodImplId.entity_id] : NULL);
		if (handler != NULL && dj_exec_use_rtc)
#endif
		{
			uint8_t rettype = dj_di_methodImplementation_getReturnType(methodImpl);

			switch (rettype) {
				case JTID_VOID:
					break;
				case JTID_BOOLEAN:
				case JTID_CHAR:
				case JTID_BYTE:
				case JTID_SHORT:
					pushShort(retval);
					break;
				case JTID_INT:
					pushInt(retval);
					break;
				case JTID_REF:
					pushRef((void*)(uint16_t)retval);
					break;
				default:
					dj_panic(DJ_PANIC_UNIMPLEMENTED_FEATURE);
			}
		}
	}	
}

typedef int32_t  (*aot_compiled_method_handler)(uint16_t rtc_frame_locals_start, uint16_t rtc_ref_stack_start);
uint32_t callJavaMethod_setup(dj_global_id_with_flags methodImplId, dj_di_pointer methodImpl, dj_frame *frame) {
	// Java method. May or may not be RTC compiled
// avroraCallMethodTimerMark(30);

	// not enough space on the heap to allocate the frame
	if (frame == NULL) {
		dj_exec_createAndThrow(STACKOVERFLOW_ERROR);
		return 0;
	}

// avroraCallMethodTimerMark(31);

	// create new frame for the function
#ifdef EXECUTION_FRAME_ON_STACK
	// Note that integer variables 'grow' down in the stack frame, so dj_di_methodImplementation_getOffsetToLocalIntegerVariables is also the size of the frame, -2 because the address of the 'first' int variable is 2 lower than the size of the frame (since slots are 16-bit).

	// init the frame
	frame->method.infusion = methodImplId.infusion;
	frame->method.entity_id = methodImplId.entity_id;
	frame->parent = NULL;
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
	// This isn't necessary when the interpreter is disabled, since the start of refStack will be set when calling the AOT compiled handler. saved_refStack is only necessary when this method will call another, at which point saveLocalState will set saved_refStack.
	frame->saved_refStack = dj_frame_getReferenceStackBase(frame, methodImpl); // initial saved_refStack (nothing on the stack)
#endif
	// set local variables to 0/null
	// Java requires locals to be explicitly initialised, so it may seem this is not necessary.
	// But if we don't initialise references to null, a GC run may mistake it for a real reference, thus corrupting memory.
	void * start = ((void*)frame) + sizeof(dj_frame);
	void * end = start + dj_di_methodImplementation_getReferenceLocalVariableCount(methodImpl)*2;
	memset(start, 0, end-start);
#endif // EXECUTION_FRAME_ON_STACK

// avroraCallMethodTimerMark(32);
	uint8_t numberOfIntArguments = dj_di_methodImplementation_getIntegerArgumentCount(methodImpl);
	uint8_t numberOfRefArguments = dj_di_methodImplementation_getReferenceArgumentCount(methodImpl)
									+ ((methodImplId.flags & FLAGS_STATIC) ? 0 : 1);
	if(dj_exec_getCurrentThread()->frameStack != NULL) {
		// For the main(String args[]) method, we don't pass anything but just leave the args NULL.
		dj_exec_passParameters(frame, methodImpl, numberOfIntArguments, numberOfRefArguments);
	}
	// pop arguments off the stack
	refStack -= numberOfRefArguments;
	intStack += numberOfIntArguments;

// avroraCallMethodTimerMark(33);

	// save current state of the frame
	dj_exec_saveLocalState(dj_exec_getCurrentThread()->frameStack);

// avroraCallMethodTimerMark(34);

	// push the new frame on the frame stack
	dj_thread_pushFrame(frame);

// avroraCallMethodTimerMark(35);

	// switch in newly created frame
	dj_exec_loadLocalState(frame);

// avroraCallMethodTimerMark(36);

	const native_method_function_t *handlers = methodImplId.infusion->native_handlers;
	native_method_function_t handler = (handlers != NULL ? methodImplId.infusion->native_handlers[methodImplId.entity_id] : NULL);
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
	if (handler != NULL && dj_exec_use_rtc)
	{
#endif
// avroraCallMethodTimerMark(37);
		// RTC compiled method: execute it directly
		uint16_t rtc_frame_locals_start = (uint16_t)dj_frame_getLocalReferenceVariables(frame); // Will be stored in Y by the function prologue
		uint16_t rtc_ref_stack_start = (uint16_t)dj_frame_getReferenceStackBase(frame, methodImpl); // Will be stored in X by the function prologue
// avroraCallMethodTimerMark(38);

		DEBUG_LOG(DBG_RTC, "[rtc] starting rtc compiled method %i at %p with return type %i\n", methodImplId.entity_id, handler, rettype);

		AVRORATRACE_ENABLE();
		// This doesn't always return int32. We don't follow the real avr-gcc ABI here, which requires shorts to be returned in R24:25 and ints in R22:25.
		// Instead we return shorts in R22:23 and ints in R22:25, which means we can simply cast a short return value when we push it on the stack and
		// ignore the higher bytes. For voids we just ignore the (garbage) return value completely.
// avroraCallMethodTimerMark(39);
		return ((aot_compiled_method_handler)handler)(rtc_frame_locals_start, rtc_ref_stack_start);
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
	}
	return 0;
#endif
}


uint32_t callJavaMethod(dj_global_id_with_flags methodImplId, dj_di_pointer methodImpl, uint16_t frame_size) {
	avroraRTCRuntimeMethodCall(methodImplId.infusion, methodImplId.entity_id);

// avroraCallMethodTimerMark(10);
#ifdef EXECUTION_FRAME_ON_STACK
	dj_frame *frame = alloca(frame_size);
#else // EXECUTION_FRAME_ON_STACK
	dj_frame *frame = dj_frame_create_fast(methodImplId, methodImpl);
#endif // EXECUTION_FRAME_ON_STACK
// avroraCallMethodTimerMark(11);

#ifdef AOT_SAFETY_CHECKS
	// When allocating a new java stack frame we need to make sure there is
	// enough space between the frame and the heap for the Java int stack to
	// grow, and for the native frames to start the AOT compiled code
	// (returnFromMethod and callJavaMethod_setup).
	// For native methods that may be called we could either reserve enough
	// space to guarantee they can execute, or check on each call, or a
	// combination of both (only check for expensive methods)

	if (((uint16_t)frame) - dj_di_methodImplementation_getMaxIntStack(methodImpl) - RTC_SAFETY_MIN_GAP_BETWEEN_STACK_AND_HEAP < (uint16_t)right_pointer) {
		rtc_safety_abort_with_error(RTC_SAFETY_RUNTIMECHECK_NATIVE_STACK_OVERFLOW);
	}
#endif

	// This will create the frame, pass parameters, etc. For AOT compiled methods it will also call the method and return the return value.
	// It always returns a 32 bit int, but for short/refs the high two bytes are garbage. For voids the whole return value should be ignored.
	return returnFromMethod(callJavaMethod_setup(methodImplId, methodImpl, frame));
}

void callNativeMethod(dj_global_id methodImplId, dj_di_pointer methodImpl, bool virtualCall) {
	avroraRTCRuntimeMethodCall(methodImplId.infusion, methodImplId.entity_id);

	bool isReturnReference=false;
	int oldNumRefStack, numRefStack;
	int diffRefArgs;

	const native_method_function_t *handlers = methodImplId.infusion->native_handlers;
	native_method_function_t handler = (handlers != NULL ? methodImplId.infusion->native_handlers[methodImplId.entity_id] : NULL);

#ifndef DARJEELING_DEBUG_FRAME
		DEBUG_LOG(DBG_DARJEELING, "Invoking native.\n");
#endif

		// the method is native, check if we have a native handler
		// for the infusion the method is in
		if (handler != NULL)
		{
#ifdef EXECUTION_PRINT_NAT_JAVA_OR_AOT
			avroraPrintStr("call nat");
#endif
			// Observe the number of reference elements on the ref. stack:
			dj_frame *frame = dj_exec_getCurrentFrame();
			oldNumRefStack = refStack - dj_frame_getReferenceStackBase(frame, methodImpl);

			// execute the method by calling the infusion's native handler
			handler();

			// The reference stack needs right treatment now, so remember the
			// contract for parameters and return values of native methods:
			// 1. All parameters MUST be popped off the stack, regardless of
			// whether they are actually used or not.
			// 2. Non-static native methods MUST NOT (are not allowed to) pop
			// off the object reference (last item). Use a peek instead.
			// 3. Return values MUST be pushed back to the stack.
			// For non-static methods, this means that the object reference is
			// to be removed from the stack now afterwards.

			// Again, compute number of reference elements on the ref. stack:
			frame = dj_exec_getCurrentFrame();
			numRefStack = refStack - dj_frame_getReferenceStackBase(frame, methodImpl);
			diffRefArgs = dj_di_methodImplementation_getReferenceArgumentCount(methodImpl) - (oldNumRefStack - numRefStack);

			if(dj_di_methodImplementation_getReturnType(methodImpl)==JTID_REF) {
				diffRefArgs--;
				isReturnReference = true;
			}

			// Popped too few arguments or more references than arguments
			if( diffRefArgs!=0 ) {
#ifdef DARJEELING_DEBUG_FRAME
				DARJEELING_PRINTF("Native method violates reference stack.\n");
				DARJEELING_PRINTF("#Ref.Arguments: %d, return type: %d, ",
						dj_di_methodImplementation_getReferenceArgumentCount(
																	methodImpl),
						dj_di_methodImplementation_getReturnType(methodImpl) );
				DARJEELING_PRINTF("isReturnReference=%s\n",
						(isReturnReference?"true":"false") );
				DARJEELING_PRINTF("#before: %d, #after: %d, diff=%d.\n",
						oldNumRefStack, numRefStack, diffRefArgs);
				dj_exec_debugCurrentFrame();
#endif
				dj_exec_createAndThrow(VIRTUALMACHINE_ERROR);
				return;
			}

			// If the method is non-static (virtual), pop the object reference
			if (virtualCall) {
				ref_t refData = popRef();
				// If the method returns a reference, we have to peel it off
				if(isReturnReference) {
					// ... now, pop-off the object reference ...
					popRef();
					// ... and push the result back afterwards.
					pushRef(refData);
				}
			}
			
			avroraRTCRuntimeMethodCallReturn();
		}
		else
		{
			DEBUG_LOG(DBG_DARJEELING, "No native handler! \n");
			// there is no native handler for this method's infusion.
			// Throw an exception
			dj_exec_createAndThrow(NATIVEMETHODNOTIMPLEMENTED_ERROR);
		}
}

void createThreadAndRunMethodToFinish(dj_global_id methodImplId) {
	int threadId;
	dj_thread * thread;

	// create a new thread object to run the <CLINIT> methods in
	thread = dj_thread_create();

	if (thread == NULL)
	{
		DEBUG_LOG(DBG_DARJEELING, "No memory to create thread\n");
		dj_panic(DJ_PANIC_OUT_OF_MEMORY);
	}

	dj_vm_addThread(dj_exec_getVM(), thread);
	threadId = thread->id;
    thread->status = THREADSTATUS_RUNNING;
	dj_exec_setCurrentThread(thread);

	callMethod(methodImplId, 0);

	// execute the method
	while (dj_vm_getThreadById(dj_exec_getVM(), threadId)->status!=THREADSTATUS_FINISHED)
	{
		// running the CLINIT method may trigger garbage collection
		rtc_run_interpreter_if_not_aot_compiled();
	}

	// clean up the thread
	thread = dj_vm_getThreadById(dj_exec_getVM(), threadId);
	dj_vm_removeThread(dj_exec_getVM(), thread);
	dj_thread_destroy(thread);
	dj_exec_setCurrentThread(NULL);
}

/**
 * Convenience method for throwing system library exceptions. It creates an exception object and throws it at the current PC.
 * For throwing exceptions that are not in the System class, create the object manually and throw it with dj_exec_throwHere.
 * @param exceptionId the class entity ID of the exception to throw. The exception class is assumed to be in the system library.
 */
void dj_exec_createAndThrow(int16_t exceptionType)
{
	avroraTerminateOnException(exceptionType);

	// DEAD CODE BELOW

	dj_object *obj = dj_vm_createSysLibObject(dj_exec_getVM(), BASE_CDEF_java_lang_Exception);
	((BASE_STRUCT_java_lang_Exception *)obj)->type = exceptionType;

	// if we can't allocate the exception, we're really out of memory :(
	// throw the last resort panic exception object we pre-allocated
	if (obj == NULL)
		obj = vm_mem_getPanicExceptionObject();

	// in case we didn't preallocate an exception object for this corner case, simply panic
	if (obj == NULL) {
		DEBUG_LOG(DBG_DARJEELING, "No memory to create exception\n");
		dj_panic(DJ_PANIC_OUT_OF_MEMORY);
	}

#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
	dj_exec_throwHere(obj);
#endif
}

#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY

/**
 * Throws an exception at the current PC. Ideally this function should be called only from outside the execution module.
 * @see dj_exe_throw()
 * @param obj the object to throw
 */
void dj_exec_throwHere(dj_object *obj)
{
	dj_exec_throw(obj, pc);
}

/**
 * Throws an exception at the given PC.
 * @see dj_exe_throw_here()
 * @param obj the object to throw
 * @param throw_pc the address at which the exception ocurred.
 */

void dj_exec_throw(dj_object *obj, uint16_t throw_pc)
{
	uint8_t i;
	dj_di_pointer method;
	char caught = 0, type_applies;
	dj_local_id catch_type_local_id;
	dj_global_id catch_type;

	// get runtime class
	runtime_id_t classRuntimeId = dj_mem_getChunkId(obj);
	dj_global_id classGlobalId = dj_vm_getRuntimeClass(classRuntimeId);

#ifdef DARJEELING_DEBUG
	dj_infusion_getName(classGlobalId.infusion, name, 16);

	DEBUG_LOG(DBG_DARJEELING, "Throwing ex., infusion=%s, entity_id=%d\n", name, classGlobalId.entity_id);

#endif

	throw_pc = pc;
	while (!caught && dj_exec_getCurrentThread()->frameStack != NULL)
	{
		method = dj_global_id_getMethodImplementation(
				dj_exec_getCurrentThread()->frameStack->method);

		// loop through the exception handlers to try and find an appropriate one
		uint8_t nr_handlers =
				dj_di_methodImplementation_getNrExceptionHandlers(method);
		for (i = 0; (i < nr_handlers) && (caught == 0); i++)
		{
			catch_type_local_id = dj_di_methodImplementation_getExceptionHandlerType(method, i);

			// If the infusion_id of the catch type equals 255, the catch block applies for every type.
			// This special case implements the finally block, as per JVM spec version 2, section 7.13.
			if (catch_type_local_id.infusion_id == 255)
			{
				type_applies = 1;
			} else {
				catch_type = dj_global_id_resolve(dj_exec_getCurrentInfusion(),
						catch_type_local_id);
				type_applies = dj_global_id_testClassType(classGlobalId,
						catch_type);
			}

			// check if this handler applies
			if ((type_applies)
					&& (throw_pc >= dj_di_methodImplementation_getExceptionHandlerStartPC(method, i))
					&& (throw_pc <= dj_di_methodImplementation_getExceptionHandlerEndPC(method, i)))
			{
				// handler found, jump to catch adress
				pc = dj_di_methodImplementation_getExceptionHandlerCatchPC(method, i);

				// TODO is this correct?
				// pop all operands from the integer and reference stacks
				intStack = (int16_t*) dj_frame_getIntegerStackBase(dj_exec_getCurrentThread()->frameStack, dj_global_id_getMethodImplementation(dj_exec_getCurrentThread()->frameStack->method));
				refStack = (ref_t*) dj_frame_getReferenceStackBase(dj_exec_getCurrentThread()->frameStack, dj_global_id_getMethodImplementation(dj_exec_getCurrentThread()->frameStack->method));

				pushRef(VOIDP_TO_REF(obj));
				caught = 1;
			}
		}

		// if not caught, destroy the current frame and go into the next on the stack
		if (!caught)
		{
#ifdef EXECUTION_FRAME_ON_STACK
			dj_thread_popFrame();
#else
			dj_frame_destroy(dj_thread_popFrame());
#endif
			// perform context switch
			if (dj_exec_getCurrentThread()->frameStack != NULL)
				dj_exec_activate_thread(dj_exec_getCurrentThread());

			throw_pc = pc;
		}

	}

	// if the exception was not caught, terminate the thread
	if (!caught)
	{
		dj_exec_getCurrentThread()->status = THREADSTATUS_FINISHED;
		dj_exec_breakExecution();

		// printf("Uncaught exception[%d]\n", classid.entity_id);
		dj_panic(DJ_PANIC_UNCAUGHT_EXCEPTION);
	}
}

#include "field_instructions.h"
#include "array_instructions.h"
#include "branch_instructions.h"
#include "invoke_instructions.h"
#include "misc_instructions.h"

#define SHORT_ARITHMETIC_OP(op) do { temp2 = popShort(); \
        temp1 = popShort();                        \
        pushShort((int16_t)(temp1 op temp2)); } while(0)

#define INT_ARITHMETIC_OP(op) do { temp2 = popInt(); \
        temp1 = popInt();                        \
        pushInt(temp1 op temp2); } while(0)

#define LONG_ARITHMETIC_OP(op) do { ltemp2 = popLong(); \
        ltemp1 = popLong();                        \
        pushLong((int64_t)(ltemp1 op ltemp2)); } while(0)


/**
 * The execution engine's main run function. Executes [nrOpcodes] instructions, or until execution is stopped explicitly,
 * or until the current method finishes if it was called by RTC compiled code.
 * When a RTC compiled method calls a normal JVM method. We should only execute instructions until the current method is
 * finished, then return control to the RTC code. (RTC_INVOKEsomething)
 * @param nrOpcodes the amount of opcodes to execute in one 'run'.
 */
int dj_exec_run(int nrOpcodes)
{
#ifdef DARJEELING_DEBUG_TRACE
	int oldCallDepth = callDepth;
#endif

	uint8_t opcode, m, n;
	int i;
	nrOpcodesLeft = nrOpcodes;
	int32_t temp1, temp2, temp3;
	int64_t ltemp1, ltemp2;
	ref_t rtemp1, rtemp2, rtemp3;

	dj_hook_call(dj_core_pollingHook, NULL);

	while (nrOpcodesLeft > 0 && dj_exec_runlevel == RUNLEVEL_RUNNING) {
		if (dj_exec_currentMethodIsRTCCompiled()) {
			return 0;
		}
		nrOpcodesLeft--;
		opcode = fetch();

#ifdef DARJEELING_DEBUG
		totalNrOpcodes++;
		oldPc = pc;
#endif

		switch (opcode) {

		// arithmetic
		case JVM_SADD: SHORT_ARITHMETIC_OP(+); break;
		case JVM_SSUB: SHORT_ARITHMETIC_OP(-); break;
		case JVM_SMUL: SHORT_ARITHMETIC_OP(*); break;
		case JVM_SDIV:
			temp2 = popShort();
			temp1 = popShort();
			if (temp2 == 0)
				dj_exec_createAndThrow(ARITHMETIC_EXCEPTION);
			else
				pushShort(temp1 / temp2);
			break;
		case JVM_SNEG: pushShort(-popShort()); break;
		case JVM_SSHR: SHORT_ARITHMETIC_OP(>>); break;
		case JVM_SUSHR:
			temp2 = popShort() & 15;
			temp1 = popShort();
			pushShort(((uint16_t) temp1) >> temp2);
			break;
		case JVM_SSHL: SHORT_ARITHMETIC_OP(<<); break;
		case JVM_SREM: SHORT_ARITHMETIC_OP(%); break;
		case JVM_SAND: SHORT_ARITHMETIC_OP(&); break;
		case JVM_SOR: SHORT_ARITHMETIC_OP(|); break;
		case JVM_SXOR: SHORT_ARITHMETIC_OP(^); break;

		case JVM_IADD: INT_ARITHMETIC_OP(+); break;
		case JVM_ISUB: INT_ARITHMETIC_OP(-); break;
		case JVM_IMUL: INT_ARITHMETIC_OP(*); break;
		case JVM_IDIV:
			temp2 = popInt();
			temp1 = popInt();
			if (temp2 == 0)
				dj_exec_createAndThrow(ARITHMETIC_EXCEPTION);
			else
				pushInt(temp1 / temp2);
			break;

		case JVM_INEG: pushInt(-popInt()); break;
		case JVM_ISHR:
			temp2 = popShort() & 31;
			temp1 = popInt();
			pushInt(((int32_t) temp1) >> temp2);
			break;
		case JVM_IUSHR:
			temp2 = popShort() & 31;
			temp1 = popInt();
			pushInt(((uint32_t) temp1) >> temp2);
			break;
		case JVM_ISHL:
			temp2 = popShort() & 31;
			temp1 = popInt();
			pushInt(((int32_t) temp1) << temp2);
			break;
		case JVM_IREM: INT_ARITHMETIC_OP(%); break;
		case JVM_IAND: INT_ARITHMETIC_OP(&); break;
		case JVM_IOR: INT_ARITHMETIC_OP(|); break;
		case JVM_IXOR: INT_ARITHMETIC_OP(^); break;

		case JVM_LADD: LONG_ARITHMETIC_OP(+); break;
		case JVM_LSUB: LONG_ARITHMETIC_OP(-); break;
		case JVM_LMUL: LONG_ARITHMETIC_OP(*); break;
		case JVM_LDIV:
			temp2 = popLong();
			temp1 = popLong();
			if (temp2 == 0)
				dj_exec_createAndThrow(ARITHMETIC_EXCEPTION);
			else
				pushLong(temp1 / temp2);
			break;
		case JVM_LNEG: pushLong(-popLong()); break;
		case JVM_LSHR: LONG_ARITHMETIC_OP(>>); break;
		case JVM_LUSHR:
			ltemp2 = popShort() & 63;
			ltemp1 = popLong();
			pushLong(((uint64_t) ltemp1) >> ltemp2);
			break;
		case JVM_LSHL: LONG_ARITHMETIC_OP(<<); break;
		case JVM_LREM: LONG_ARITHMETIC_OP(%); break;
		case JVM_LAND: LONG_ARITHMETIC_OP(&); break;
		case JVM_LOR: LONG_ARITHMETIC_OP(|); break;
		case JVM_LXOR: LONG_ARITHMETIC_OP(^); break;

		// TODO use peekInt/pokeInt
		case JVM_S2B: pushShort((int8_t) popShort()); break;
		case JVM_S2C: pushShort((int8_t) popShort()); break;
		case JVM_S2I: pushInt((int32_t) popShort()); break;
		case JVM_S2L: pushLong((int64_t) popShort()); break;

		case JVM_I2B: pushShort((int8_t) popInt()); break;
		case JVM_I2C: pushShort((int8_t) popInt()); break;
		case JVM_I2S: pushShort((int16_t) popInt()); break;
		case JVM_I2L: pushLong((int64_t) popInt()); break;

		case JVM_L2I: pushInt((int32_t) popLong()); break;
		case JVM_L2S: pushShort((int16_t) popLong()); break;

		case JVM_B2C:
			// TODO keep this opcode?
			break;

		case JVM_IINC:
			temp1 = fetch();
			temp2 = (int8_t) fetch();
			setLocalInt(temp1, getLocalInt(temp1) + temp2);
			break;

		case JVM_IINC_W:
			temp1 = fetch();
			temp2 = (int16_t) fetch16();
			setLocalInt(temp1, getLocalInt(temp1) + temp2);
			break;

		case JVM_SINC:
			temp1 = fetch();
			temp2 = (int8_t) fetch();
			setLocalShort(temp1, getLocalShort(temp1) + temp2);
			break;

		case JVM_SINC_W:
			temp1 = fetch();
			temp2 = (int16_t) fetch16();
			setLocalShort(temp1, getLocalShort(temp1) + temp2);
			break;

		// stack and local variables
		case JVM_SCONST_M1: pushShort(-1); break;
		case JVM_SCONST_0: pushShort(0); break;
		case JVM_SCONST_1: pushShort(1); break;
		case JVM_SCONST_2: pushShort(2); break;
		case JVM_SCONST_3: pushShort(3); break;
		case JVM_SCONST_4: pushShort(4); break;
		case JVM_SCONST_5: pushShort(5); break;

		case JVM_ICONST_M1: pushInt(-1); break;
		case JVM_ICONST_0: pushInt(0); break;
		case JVM_ICONST_1: pushInt(1); break;
		case JVM_ICONST_2: pushInt(2); break;
		case JVM_ICONST_3: pushInt(3); break;
		case JVM_ICONST_4: pushInt(4); break;
		case JVM_ICONST_5: pushInt(5); break;

		case JVM_LCONST_0: pushLong(0); break;
		case JVM_LCONST_1: pushLong(1); break;

		case JVM_BIPUSH: pushInt((int8_t) fetch()); break;
		case JVM_BSPUSH: pushShort((int8_t) fetch()); break;
		case JVM_SIPUSH: pushInt((int16_t) fetch16()); break;
		case JVM_SSPUSH: pushShort((int16_t) fetch16()); break;
		case JVM_IIPUSH: pushInt((int32_t) fetch32()); break;
		case JVM_LLPUSH: pushLong((int64_t) fetch64()); break;

		case JVM_LDS: LDS(); break;

		case JVM_SLOAD: pushShort(getLocalShort(fetch())); break;
		case JVM_SLOAD_0: pushShort(getLocalShort(0)); break;
		case JVM_SLOAD_1: pushShort(getLocalShort(1)); break;
		case JVM_SLOAD_2: pushShort(getLocalShort(2)); break;
		case JVM_SLOAD_3: pushShort(getLocalShort(3)); break;

		case JVM_ILOAD: pushInt(getLocalInt(fetch())); break;
		case JVM_ILOAD_0: pushInt(getLocalInt(0)); break;
		case JVM_ILOAD_1: pushInt(getLocalInt(1)); break;
		case JVM_ILOAD_2: pushInt(getLocalInt(2)); break;
		case JVM_ILOAD_3: pushInt(getLocalInt(3)); break;

		case JVM_LLOAD: pushLong(getLocalLong(fetch())); break;
		case JVM_LLOAD_0: pushLong(getLocalLong(0)); break;
		case JVM_LLOAD_1: pushLong(getLocalLong(1)); break;
		case JVM_LLOAD_2: pushLong(getLocalLong(2)); break;
		case JVM_LLOAD_3: pushLong(getLocalLong(3)); break;

		case JVM_ACONST_NULL: pushRef(nullref); break;
		case JVM_ALOAD: pushRef(getLocalRef(fetch())); break;
		case JVM_ALOAD_0: pushRef(getLocalRef(0)); break;
		case JVM_ALOAD_1: pushRef(getLocalRef(1)); break;
		case JVM_ALOAD_2: pushRef(getLocalRef(2)); break;
		case JVM_ALOAD_3: pushRef(getLocalRef(3)); break;

		case JVM_SSTORE: setLocalShort(fetch(), popShort()); break;
		case JVM_SSTORE_0: setLocalShort(0, popShort()); break;
		case JVM_SSTORE_1: setLocalShort(1, popShort()); break;
		case JVM_SSTORE_2: setLocalShort(2, popShort()); break;
		case JVM_SSTORE_3: setLocalShort(3, popShort()); break;

		case JVM_ISTORE: setLocalInt(fetch(), popInt()); break;
		case JVM_ISTORE_0: setLocalInt(0, popInt()); break;
		case JVM_ISTORE_1: setLocalInt(1, popInt()); break;
		case JVM_ISTORE_2: setLocalInt(2, popInt()); break;
		case JVM_ISTORE_3: setLocalInt(3, popInt()); break;

		case JVM_LSTORE: setLocalLong(fetch(), popLong()); break;
		case JVM_LSTORE_0: setLocalLong(0, popLong()); break;
		case JVM_LSTORE_1: setLocalLong(1, popLong()); break;
		case JVM_LSTORE_2: setLocalLong(2, popLong()); break;
		case JVM_LSTORE_3: setLocalLong(3, popLong()); break;

		case JVM_ASTORE: setLocalRef(fetch(), popRef()); break;
		case JVM_ASTORE_0: setLocalRef(0, popRef()); break;
		case JVM_ASTORE_1: setLocalRef(1, popRef()); break;
		case JVM_ASTORE_2: setLocalRef(2, popRef()); break;
		case JVM_ASTORE_3: setLocalRef(3, popRef()); break;

		// Integer stack operations
		case JVM_IPOP: intStack++; break;
		case JVM_IPOP2: intStack += 2; break;

		case JVM_IDUP:
			*intStack = *(intStack + 1);
			intStack--;
			break;

		case JVM_IDUP2:
			*(intStack - 1) = *(intStack + 1);
			*(intStack) = *(intStack + 2);
			intStack -= 2;
			break;

		case JVM_IDUP_X:
			m = fetch();
			n = m & 15;
			m >>= 4;

			// reserve space on the stack for the duplicated data
			intStack -= m;
			
			// m: how many integer slots to duplicate
			// n: how deep to bury them in the stack. (see getIDupInstructions in the infuser)
			if (n == 0) {
				// perform normal dup
				for (i = 0; i < m; i++)
					intStack[+i + 1] = intStack[+i + m + 1];

			} else {
				// move existing stuff forwards
				for (i = 0; i < n; i++)
					intStack[+i + 1] = intStack[+i + m + 1];

				// copy duplicated value into place
				for (i = 0; i < m; i++)
					intStack[+i + n + 1] = intStack[+i + 1];
			}

			break;

		// TODO make faster
		case JVM_IDUP_X1:
			temp1 = popShort();
			temp2 = popShort();
			pushShort(temp1);
			pushShort(temp2);
			pushShort(temp1);
			break;

		// TODO make faster
		case JVM_IDUP_X2:
			temp1 = popShort();
			temp2 = popShort();
			temp3 = popShort();
			pushShort(temp1);
			pushShort(temp3);
			pushShort(temp2);
			pushShort(temp1);
			break;

		// Reference stack operations
		case JVM_APOP: refStack--; break;
		case JVM_APOP2:	refStack -= 2; break;

		case JVM_ADUP:
			*refStack = *(refStack - 1);
			refStack++;
			break;

		case JVM_ADUP2:
			*(refStack) = *(refStack - 2);
			*(refStack + 1) = *(refStack - 1);
			refStack += 2;
			break;

		// TODO make faster
		case JVM_ADUP_X1:
			rtemp1 = popRef();
			rtemp2 = popRef();
			pushRef(rtemp1);
			pushRef(rtemp2);
			pushRef(rtemp1);
			break;

		// TODO make faster
		case JVM_ADUP_X2:
			rtemp1 = popRef();
			rtemp2 = popRef();
			rtemp3 = popRef();
			pushRef(rtemp1);
			pushRef(rtemp3);
			pushRef(rtemp2);
			pushRef(rtemp1);
			break;

		// program flow
		case JVM_GOTO: GOTO(); break;

		case JVM_IF_ICMPEQ: IF_ICMPEQ(); break;
		case JVM_IF_ICMPNE:	IF_ICMPNE(); break;
		case JVM_IF_ICMPLT:	IF_ICMPLT(); break;
		case JVM_IF_ICMPGE:	IF_ICMPGE(); break;
		case JVM_IF_ICMPGT:	IF_ICMPGT(); break;
		case JVM_IF_ICMPLE:	IF_ICMPLE(); break;

		case JVM_IF_SCMPEQ:	IF_SCMPEQ(); break;
		case JVM_IF_SCMPNE:	IF_SCMPNE(); break;
		case JVM_IF_SCMPLT:	IF_SCMPLT(); break;
		case JVM_IF_SCMPGE:	IF_SCMPGE(); break;
		case JVM_IF_SCMPGT:	IF_SCMPGT(); break;
		case JVM_IF_SCMPLE:	IF_SCMPLE(); break;

		case JVM_IIFEQ: IIFEQ(); break;
		case JVM_IIFNE: IIFNE(); break;
		case JVM_IIFLT: IIFLT(); break;
		case JVM_IIFGE: IIFGE(); break;
		case JVM_IIFGT: IIFGT(); break;
		case JVM_IIFLE: IIFLE(); break;

		case JVM_SIFEQ: SIFEQ(); break;
		case JVM_SIFNE: SIFNE(); break;
		case JVM_SIFLT: SIFLT(); break;
		case JVM_SIFGE: SIFGE(); break;
		case JVM_SIFGT: SIFGT(); break;
		case JVM_SIFLE: SIFLE(); break;

		case JVM_IF_ACMPEQ: IF_ACMPEQ(); break;
		case JVM_IF_ACMPNE: IF_ACMPNE(); break;
		case JVM_IFNULL: IFNULL(); break;
		case JVM_IFNONNULL: IFNONNULL(); break;

		case JVM_RETURN: RETURN(); break;
		case JVM_SRETURN: SRETURN(); break;
		case JVM_IRETURN: IRETURN(); break;
		case JVM_LRETURN: LRETURN(); break;
		case JVM_ARETURN: ARETURN(); break;

		case JVM_INVOKESTATIC: INVOKESTATIC(); break;
		case JVM_INVOKESPECIAL: INVOKESPECIAL(); break;
		case JVM_INVOKEVIRTUAL:	INVOKEVIRTUAL(); break;
		case JVM_INVOKEINTERFACE: INVOKEINTERFACE();break;

		// Monitors
		case JVM_MONITORENTER: MONITORENTER(); break;
		case JVM_MONITOREXIT: MONITOREXIT(); break;

		// Arrays and classes
		case JVM_NEW: NEW(); break;
		case JVM_INSTANCEOF: INSTANCEOF(); break;
		case JVM_CHECKCAST: CHECKCAST(); break;

		// Array operations
		case JVM_NEWARRAY: NEWARRAY(); break;
		case JVM_ANEWARRAY: ANEWARRAY(); break;
		case JVM_ARRAYLENGTH: ARRAYLENGTH(); break;

		case JVM_PUTARRAY_B: BASTORE(); break;
		case JVM_PUTARRAY_C: CASTORE(); break;
		case JVM_PUTARRAY_S: SASTORE(); break;
		case JVM_PUTARRAY_I: IASTORE(); break;
		case JVM_LASTORE: LASTORE(); break;
		case JVM_PUTARRAY_A: AASTORE(); break;

		case JVM_GETARRAY_B: BALOAD(); break;
		case JVM_GETARRAY_C: CALOAD(); break;
		case JVM_GETARRAY_S: SALOAD(); break;
		case JVM_GETARRAY_I: IALOAD(); break;
		case JVM_LALOAD: LALOAD(); break;
		case JVM_GETARRAY_A: AALOAD(); break;

		// Static variables
		case JVM_GETSTATIC_B: GETSTATIC_B(); break;
		case JVM_GETSTATIC_C: GETSTATIC_C(); break;
		case JVM_GETSTATIC_S: GETSTATIC_S(); break;
		case JVM_GETSTATIC_I: GETSTATIC_I(); break;
		case JVM_GETSTATIC_L: GETSTATIC_L(); break;
		case JVM_GETSTATIC_A: GETSTATIC_A(); break;

		case JVM_PUTSTATIC_B: PUTSTATIC_B(); break;
		case JVM_PUTSTATIC_C: PUTSTATIC_C(); break;
		case JVM_PUTSTATIC_S: PUTSTATIC_S(); break;
		case JVM_PUTSTATIC_I: PUTSTATIC_I(); break;
		case JVM_PUTSTATIC_L: PUTSTATIC_L(); break;
		case JVM_PUTSTATIC_A: PUTSTATIC_A(); break;

		// Field operations
		case JVM_GETFIELD_B: GETFIELD_B(); break;
		case JVM_GETFIELD_C: GETFIELD_C(); break;
		case JVM_GETFIELD_S: GETFIELD_S(); break;
		case JVM_GETFIELD_I: GETFIELD_I(); break;
		case JVM_GETFIELD_L: GETFIELD_L(); break;
		case JVM_GETFIELD_A: GETFIELD_A(); break;

		case JVM_PUTFIELD_B: PUTFIELD_B(); break;
		case JVM_PUTFIELD_C: PUTFIELD_C(); break;
		case JVM_PUTFIELD_S: PUTFIELD_S(); break;
		case JVM_PUTFIELD_I: PUTFIELD_I(); break;
		case JVM_PUTFIELD_L: PUTFIELD_L(); break;
		case JVM_PUTFIELD_A: PUTFIELD_A(); break;

        case JVM_GETFIELD_A_FIXED:
        case JVM_PUTFIELD_A_FIXED:
            dj_panic(DJ_PANIC_UNIMPLEMENTED_FEATURE);
        break;

		// Case statements
		case JVM_TABLESWITCH: TABLESWITCH(); break;
		case JVM_LOOKUPSWITCH: LOOKUPSWITCH(); break;

		// Exceptions
		case JVM_ATHROW: ATHROW(); break;

		// Long compare
		case JVM_LCMP:
			// TODO maybe find a smarter/quicker way of doing this
			ltemp2 = popLong();
			ltemp1 = popLong();
			if (ltemp1>ltemp2)
				pushShort(1);
			else if (ltemp1<ltemp2)
				pushShort(-1);
			else
				pushShort(0);

			break;

		// misc
		case JVM_NOP: /* do nothing :3 */ break;
		case JVM_BRTARGET: /* do nothing :3 */ break;
		case JVM_MARKLOOP_START:
			pc += (2*fetch()) + 1; // Skip over the markloop instruction
			break;
		case JVM_MARKLOOP_END: /* do nothing :3 */ break;

		default:
			DEBUG_LOG(DBG_DARJEELING, "Unimplemented opcode %d at pc=%d\n", opcode, oldPc);
			dj_exec_createAndThrow(VIRTUALMACHINE_ERROR);
		}

#ifdef DARJEELING_DEBUG_TRACE

		dj_thread *currentThread = dj_exec_getCurrentThread();
		dj_frame *current_frame = currentThread->frameStack;
		if (current_frame==NULL) continue;

		// DEBUG_LOG(DBG_DARJEELING, "%*s", oldCallDepth*2, "");
		DEBUG_LOG(DBG_DARJEELING, "%03d->%03d   ", oldPc, pc);
		DEBUG_LOG(DBG_DARJEELING, "%-15s", jvm_opcodes[opcode]);

		DEBUG_LOG(DBG_DARJEELING, "R<");

		dj_di_pointer method = dj_global_id_getMethodImplementation(current_frame->method);
		int len = dj_di_methodImplementation_getReferenceLocalVariableCount(method);
		for (i=0; i<len; i++)
			DEBUG_LOG(DBG_DARJEELING, (i==0)?" %-9d ":", %-9d ", localReferenceVariables[i]);

		DEBUG_LOG(DBG_DARJEELING, ">");

		DEBUG_LOG(DBG_DARJEELING, "\tI<");

		len = dj_di_methodImplementation_getIntegerLocalVariableCount(method);
		for (i=0; i<len; i++)
			DEBUG_LOG(DBG_DARJEELING, (i==0)?" %-9d ":", %-9d ", localIntegerVariables[i]);

		DEBUG_LOG(DBG_DARJEELING, ">");
		DEBUG_LOG(DBG_DARJEELING, "\tR(");


		// // abuse this method to calculate nr_int_stack and nr_ref_stack for us
		// dj_exec_saveLocalState(current_frame);

		uint8_t nr_ref_stack = refStack - dj_frame_getReferenceStackBase(current_frame);
		uint8_t nr_int_stack = dj_frame_getIntegerStack(current_frame) - intStack;

		ref_t *refStackStart = dj_frame_getReferenceStackBase(current_frame);
		for (i=0; i<nr_ref_stack; i++)
			DEBUG_LOG(DBG_DARJEELING, "%-6d,", refStackStart[i]);

		DEBUG_LOG(DBG_DARJEELING, ")");

		DEBUG_LOG(DBG_DARJEELING, "\tI(");

		int16_t *intStackStart = dj_frame_getIntegerStackBase(current_frame);
		for (i=0; i<nr_int_stack; i++)
			DEBUG_LOG(DBG_DARJEELING, "%-6d,", intStackStart[-i]);

		DEBUG_LOG(DBG_DARJEELING, ")");

		DEBUG_LOG(DBG_DARJEELING, "\t(%d,%d)", current_frame->nr_ref_stack, current_frame->nr_int_stack);

		DEBUG_LOG(DBG_DARJEELING, "\n");

		oldCallDepth = callDepth;

#endif


	}

	return nrOpcodesLeft;
}
#endif // EXECUTION_DISABLEINTERPRETER_COMPLETELY

void rtc_run_interpreter_if_not_aot_compiled() {
    if (!dj_exec_currentMethodIsRTCCompiled()) {
#ifndef EXECUTION_DISABLEINTERPRETER_COMPLETELY
    #ifdef EXECUTION_BREAK_IF_IN_INTERPRETER
        avroraPrintStr("abort entering interpreter");
        asm volatile ("break");
    #else
        while (!dj_exec_currentMethodIsRTCCompiled()) {
            dj_exec_run(RUNSIZE);
        }
    #endif
#else
    avroraPrintStr("abort entering interpreter");
    asm volatile ("break");
#endif
    }
}

/**
 * @}
 */
