package org.csiro.darjeeling.infuser.structure.lightweightmethods;

import java.util.ArrayList;
import org.csiro.darjeeling.infuser.structure.BaseType;
import org.csiro.darjeeling.infuser.bytecode.*;
import org.csiro.darjeeling.infuser.bytecode.instructions.*;
import org.csiro.darjeeling.infuser.structure.elements.internal.InternalMethodImplementation;
import org.csiro.darjeeling.infuser.structure.elements.AbstractMethodDefinition;

public class LightweightMethodImplementations extends LightweightMethod {
	private LightweightMethodImplementations(String className, String methodName) {
		super(className, methodName);
	}

    public static LightweightMethod fft_FIX_MPY_lightweight8() {
        return new LightweightMethod("javax.rtcbench.RTCBenchmark", "FIX_MPY_lightweight8") {
            @Override
            public void setupLightweightMethod() {

                // Java version
                // private static byte FIX_MPY(byte a, byte b)
                // {
                //     short c = (short)((short)((short)a * (short)b) >> 6);
                //     return (byte)((c >> 1) + ((byte)(c & 0x01)));
                // }

                // // Original (non-lightweight) JVM version:
                // sload(0)                      ;                                                             ; 0Short(Byte)                                                ;   ; 0,1
                // sload(1)                      ; 0Short(Byte)                                                ; 0Short(Byte),1Short(Byte)                                   ;   ; 1
                // smul                          ; 0Short(Byte),1Short(Byte)                                   ; 2Short                                                      ; G ; 
                // bspush                        ; 2Short                                                      ; 2Short,3Short(Byte)                                         ;   ; 
                // sshr                          ; 2Short,3Short(Byte)                                         ; 5Short                                                      ;   ; 
                // sstore(2)                     ; 5Short                                                      ;                                                             ;   ; 2
                // sload(2)                      ;                                                             ; 7Short                                                      ;  K; 2
                // sconst_1                      ; 7Short                                                      ; 7Short,8Short(Byte)                                         ;   ; 2
                // sshr                          ; 7Short,8Short(Byte)                                         ; 9Short                                                      ;   ; 2
                // sload(2)                      ; 9Short                                                      ; 9Short,10Short                                              ;   ; 2
                // sconst_1                      ; 9Short,10Short                                              ; 9Short,10Short,11Short(Byte)                                ;   ; 
                // sand                          ; 9Short,10Short,11Short(Byte)                                ; 9Short,12Short                                              ;   ; 
                // s2b                           ; 9Short,12Short                                              ; 9Short,13Byte                                               ;   ; 
                // sadd                          ; 9Short,13Byte                                               ; 14Short                                                     ;   ; 
                // s2b                           ; 14Short                                                     ; 15Byte                                                      ;   ; 
                // sreturn                       ; 15Byte                                                      ;                                                             ;   ; 

                // Lightweight version
                //                   // a, b
                // smul              // a*b
                // bspush 6          // a*b, 6
                // sshr              // c (=(a*b)>>6)
                // idup              // c, c
                // sconst_1          // c, c, 1
                // sshr              // c, c>>1
                // swap              // c>>1, c
                // sconst_1          // c>>1, c, 1
                // sand              // c>>1, c&1
                // sadd              // (c>>1 + c&1)
                // s2b               // (byte)(c>>1 + c&1)
                // sreturn

                addInstruction(new ArithmeticInstruction(Opcode.SMUL));
                addInstruction(new ImmediateBytePushInstruction(Opcode.BSPUSH, 6));
                addInstruction(new ArithmeticInstruction(Opcode.SSHR));
                addInstruction(new StackInstruction(Opcode.IDUP));
                addInstruction(new ConstantPushInstruction(Opcode.SCONST_1, 1));
                addInstruction(new ArithmeticInstruction(Opcode.SSHR));
                addInstruction(new StackInstruction(Opcode.ISWAP_X));
                addInstruction(new ConstantPushInstruction(Opcode.SCONST_1, 1));
                addInstruction(new ArithmeticInstruction(Opcode.SAND));
                addInstruction(new ArithmeticInstruction(Opcode.SADD));
                addInstruction(new ExplicitCastInstruction(Opcode.S2B));
                addInstruction(new SimpleInstruction(Opcode.SRETURN));
            }
        };      
    }

    public static LightweightMethod fft_FIX_MPY_lightweight16() {
        return new LightweightMethod("javax.rtcbench.RTCBenchmark", "FIX_MPY_lightweight16") {
            @Override
            public void setupLightweightMethod() {
            	// Lightweight method made using @Lightweight attribute
				// --- LISTING: JVM LOOPS RTCBenchmark.fix_mpy (SS)S

				// --- LISTING: ONLY JVM                                         RTCBenchmark.fix_mpy (SS)S
				//                                                                 cycles                      exec   avg |  cnt bytes
				// MethodPreamble                                                    6144   2.8%   4.0%C        768   8.0 |    1     8   0.6%   0.6%C 0->0: 
				// JVM_SSTORE_1                                                      6144   2.8%   4.0%C        768   8.0 |    1     8   0.6%   0.6%C 4->2:Short 
				// JVM_SSTORE_0                                                      6144   2.8%   4.0%C        768   8.0 |    1     8   0.6%   0.6%C 2->0: 
				// JVM_SLOAD_0                                                          0   0.0%   0.0%C          0   0.0 |    1     0   0.0%   0.0%C 0->2:Short 
				// JVM_SLOAD_1                                                          0   0.0%   0.0%C          0   0.0 |    1     0   0.0%   0.0%C 2->4:Short,Short 
				// JVM_SIMUL                                                        36752  16.5%  24.1%C        768  47.9 |    1    14   1.0%   1.1%C 4->4:Int 
				// JVM_BSPUSH         15                                             1536   0.7%   1.0%C        768   2.0 |    1     4   0.3%   0.3%C 4->6:Int,Short 
				// JVM_ISHR                                                         83712  37.6%  54.8%C        768 109.0 |    1    14   1.0%   1.1%C 6->4:Int 
				// JVM_I2S                                                              0   0.0%   0.0%C          0   0.0 |    1     0   0.0%   0.0%C 4->2:Short 
				// JVM_SRETURN                                                      12288   5.5%   8.0%C        768  16.0 |    1    16   1.2%   1.2%C 2->0: 

                addInstruction(new ArithmeticInstruction(Opcode.SIMUL));
                addInstruction(new ImmediateBytePushInstruction(Opcode.BSPUSH, 15));
                addInstruction(new ArithmeticInstruction(Opcode.ISHR));
                addInstruction(new ArithmeticInstruction(Opcode.I2S));
                addInstruction(new SimpleInstruction(Opcode.SRETURN));
            }
        };      
    }

	public static LightweightMethod coremark_ee_isdigit_lightweight() {
		return new LightweightMethod("javax.rtcbench.CoreState", "ee_isdigit_lightweight") {
			@Override
			public void setupLightweightMethod() {
				InstructionHandle brtarget0 = new InstructionHandle(new BranchTargetInstruction(Opcode.BRTARGET));
				InstructionHandle brtarget1 = new InstructionHandle(new BranchTargetInstruction(Opcode.BRTARGET));

				// idup
				// bspush 48
				// if_scmplt brtarget 0
				// bspush 57
				// if_scmpgt brtarget 1
				// sconst_1
				// sreturn
				// brtarget 0
				// ipop // To clear the copy of the parameter. We won't need anymore.
				// brtarget 1
				// sconst_0
				// sreturn

				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new ImmediateBytePushInstruction(Opcode.BSPUSH, 48));
				addInstruction(new BranchInstruction(Opcode.IF_SCMPLT, 0)).setBranchHandle(brtarget0);
				addInstruction(new ImmediateBytePushInstruction(Opcode.BSPUSH, 57));
				addInstruction(new BranchInstruction(Opcode.IF_SCMPGT, 0)).setBranchHandle(brtarget1);
				addInstruction(new ConstantPushInstruction(Opcode.SCONST_1, 1));
				addInstruction(new SimpleInstruction(Opcode.SRETURN));
				addInstructionHandle(brtarget0);
				addInstruction(new StackInstruction(Opcode.IPOP));				
				addInstructionHandle(brtarget1);
				addInstruction(new ConstantPushInstruction(Opcode.SCONST_0, 0));
				addInstruction(new SimpleInstruction(Opcode.SRETURN));
			}
		};
	}

	public static LightweightMethod testISWAP() {
		return new LightweightMethod("javax.rtcbench.RTCBenchmark", "testISWAP") {
			@Override
			public void setupLightweightMethod() {

				// iswap_x
				// s2i
				// iadd
				// ireturn

                addInstruction(new StackInstruction(Opcode.ISWAP_X));
				addInstruction(new ArithmeticInstruction(Opcode.S2I));
				addInstruction(new ArithmeticInstruction(Opcode.IADD));
				addInstruction(new SimpleInstruction(Opcode.IRETURN));
			}
		};
	}
	public static LightweightMethod testLOAD_STORE() {
		return new LightweightMethod("javax.rtcbench.RTCBenchmark", "testLOAD_STORE") {
			@Override
			public void setupLightweightMethod() {
				LocalVariable shortVariable = addLocalVariable(0, BaseType.Short);
				LocalVariable intVar = addLocalVariable(1, BaseType.Int);

				// sstore_2
				// istore_0
				// s2i
				// iload_0
				// iadd
				// ireturn

                addInstruction(new LoadStoreInstruction(Opcode.ISTORE, shortVariable));
                addInstruction(new LoadStoreInstruction(Opcode.ISTORE, intVar));
                addInstruction(new LoadStoreInstruction(Opcode.ILOAD, shortVariable));
				addInstruction(new ArithmeticInstruction(Opcode.S2I));
                addInstruction(new LoadStoreInstruction(Opcode.ILOAD, intVar));
				addInstruction(new ArithmeticInstruction(Opcode.IADD));
				addInstruction(new SimpleInstruction(Opcode.IRETURN));
			}
		};
	}
	public static LightweightMethod isOddShort() {
		return new LightweightMethod("javax.rtcbench.RTCBenchmark", "isOddShort") {
			@Override
			public void setupLightweightMethod() {
				InstructionHandle brtarget0 = new InstructionHandle(new BranchTargetInstruction(Opcode.BRTARGET));

				// sconst_1
				// sand
				// sreturn

				addInstruction(new ConstantPushInstruction(Opcode.SCONST_1, 1));
				addInstruction(new ArithmeticInstruction(Opcode.SAND));
				addInstruction(new SimpleInstruction(Opcode.SRETURN));
			}
		};
	}

	public static LightweightMethod isOddInt() {
		return new LightweightMethod("javax.rtcbench.RTCBenchmark", "isOddInt") {
			@Override
			public void setupLightweightMethod() {

				// i2s
				// sconst_1
				// sand
				// sreturn

				addInstruction(new ExplicitCastInstruction(Opcode.I2S));
				addInstruction(new ConstantPushInstruction(Opcode.SCONST_1, 1));
				addInstruction(new ArithmeticInstruction(Opcode.SAND));
				addInstruction(new SimpleInstruction(Opcode.SRETURN));
			}
		};
	}

	public static LightweightMethod isNull() {
		return new LightweightMethod("javax.rtcbench.RTCBenchmark", "isNull") {
			@Override
			public void setupLightweightMethod() {
				InstructionHandle brtarget0 = new InstructionHandle(new BranchTargetInstruction(Opcode.BRTARGET));

				// ifnull brtarget 0
				// sconst_1
				// sret
				// brtarget 0
				// sconst_0
				// sret

				addInstruction(new BranchInstruction(Opcode.IFNULL, 0)).setBranchHandle(brtarget0);
				addInstruction(new ConstantPushInstruction(Opcode.SCONST_0, 0));
				addInstruction(new SimpleInstruction(Opcode.SRETURN));
				addInstructionHandle(brtarget0);
				addInstruction(new ConstantPushInstruction(Opcode.SCONST_1, 1));
				addInstruction(new SimpleInstruction(Opcode.SRETURN));
			}
		};
	}

	public static LightweightMethod timesTenTestHighStackShort() {
		return new LightweightMethod("javax.rtcbench.RTCBenchmark", "timesTenTestHighStackShort") {
			@Override
			public void setupLightweightMethod() {

				// test if the infuser handles the case where the lightweight method has a stack higher than the parameters,
				// and higher than the calling method correctly. it needs to increase maxIntStack for the caller.

				// idup
				// idup
				// idup
				// idup
				// idup
				// idup
				// idup
				// idup
				// idup
				// sadd
				// sadd
				// sadd
				// sadd
				// sadd
				// sadd
				// sadd
				// sadd
				// sadd
				// bspush 42
				// sreturn

				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new StackInstruction(Opcode.IDUP));
				addInstruction(new ArithmeticInstruction(Opcode.SADD));
				addInstruction(new ArithmeticInstruction(Opcode.SADD));
				addInstruction(new ArithmeticInstruction(Opcode.SADD));
				addInstruction(new ArithmeticInstruction(Opcode.SADD));
				addInstruction(new ArithmeticInstruction(Opcode.SADD));
				addInstruction(new ArithmeticInstruction(Opcode.SADD));
				addInstruction(new ArithmeticInstruction(Opcode.SADD));
				addInstruction(new ArithmeticInstruction(Opcode.SADD));
				addInstruction(new ArithmeticInstruction(Opcode.SADD));
				addInstruction(new SimpleInstruction(Opcode.SRETURN));
			}
		};
	}

	public static LightweightMethod timesTenTestHighStackRef() {
		return new LightweightMethod("javax.rtcbench.RTCBenchmark", "timesTenTestHighStackRef") {
			@Override
			public void setupLightweightMethod() {

				// test if the infuser handles the case where the lightweight method has a stack higher than the parameters,
				// and higher than the calling method correctly. it needs to increase maxIntStack for the caller.

				// adup
				// adup
				// adup
				// adup
				// adup
				// adup
				// adup
				// adup
				// adup
				// apop
				// apop
				// apop
				// apop
				// apop
				// apop
				// apop
				// apop
				// apop
				// apop
				// sreturn

				addInstruction(new StackInstruction(Opcode.ADUP));
				addInstruction(new StackInstruction(Opcode.ADUP));
				addInstruction(new StackInstruction(Opcode.ADUP));
				addInstruction(new StackInstruction(Opcode.ADUP));
				addInstruction(new StackInstruction(Opcode.ADUP));
				addInstruction(new StackInstruction(Opcode.ADUP));
				addInstruction(new StackInstruction(Opcode.ADUP));
				addInstruction(new StackInstruction(Opcode.ADUP));
				addInstruction(new StackInstruction(Opcode.ADUP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ArithmeticInstruction(Opcode.APOP));
				addInstruction(new ImmediateBytePushInstruction(Opcode.BSPUSH, 42));
				addInstruction(new SimpleInstruction(Opcode.SRETURN));
			}
		};
	}
}
