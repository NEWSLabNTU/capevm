package org.csiro.darjeeling.infuser.structure;

import java.util.List;
import java.util.ArrayList;
import org.apache.bcel.classfile.JavaClass;
import org.apache.bcel.classfile.AnnotationEntry;
import org.apache.tools.ant.BuildException;
import org.csiro.darjeeling.infuser.bytecode.CodeBlock;
import org.csiro.darjeeling.infuser.bytecode.CodeBlockTransformation;
import org.csiro.darjeeling.infuser.bytecode.Instruction;
import org.csiro.darjeeling.infuser.bytecode.InstructionHandle;
import org.csiro.darjeeling.infuser.bytecode.InstructionList;
import org.csiro.darjeeling.infuser.bytecode.Opcode;
import org.csiro.darjeeling.infuser.bytecode.instructions.GetConstArrayInstruction;
import org.csiro.darjeeling.infuser.bytecode.instructions.LocalIdInstruction;
import org.csiro.darjeeling.infuser.bytecode.instructions.NewArrayInstruction;
import org.csiro.darjeeling.infuser.bytecode.instructions.PushInstruction;
import org.csiro.darjeeling.infuser.bytecode.analysis.GeneratedValueSet;
import org.csiro.darjeeling.infuser.structure.elements.internal.InternalClassDefinition;
import org.csiro.darjeeling.infuser.structure.elements.internal.InternalInfusion;
import org.csiro.darjeeling.infuser.structure.elements.internal.InternalMethodDefinition;
import org.csiro.darjeeling.infuser.structure.elements.internal.InternalMethodImplementation;
import org.csiro.darjeeling.infuser.structure.elements.AbstractField;

public class ConstArrayHandler extends CodeBlockTransformation {
	public ConstArrayHandler(CodeBlock codeBlock)
	{
		super(codeBlock);
	}

	private static class ConstArrayData {
		int length;
		int type;
		AbstractField field;
		int stringTableId;
		int[] data = null;		
	}

	private static final int T_BOOLEAN = 4;
	private static final int T_CHAR = 5;
	private static final int T_FLOAT = 6;
	private static final int T_DOUBLE = 7;
	private static final int T_BYTE = 8;
	private static final int T_SHORT = 9;
	private static final int T_INT = 10;
	private static final int T_LONG = 11;

	private static ArrayList<ConstArrayData> constArrays = new ArrayList<ConstArrayData>();

	public static boolean isConstArrayClass(JavaClass javaClass) {
		for (AnnotationEntry entry : javaClass.getAnnotationEntries()) {
			if (entry.getAnnotationType().contains("ConstArray")) {
				return true;
			}
		}
		return false;
	}

	private static ConstArrayData getConstArrayDataByFieldId(int fieldId) {
		for (ConstArrayData data : constArrays) {
			if (data.field.getGlobalId().getEntityId() == fieldId) {
				return data;
			}
		}
		return null;
	}

	public static boolean isStaticFieldIdConstArray(int fieldId) {
		return getConstArrayDataByFieldId(fieldId) != null;
	}

	public static int getConstArrayStringTableId(int fieldId) {
		return getConstArrayDataByFieldId(fieldId).stringTableId;
	}

	public static void processConstArrayClass(InternalInfusion infusion, InternalClassDefinition classDef, JavaClass javaClass) {

		for (org.apache.bcel.classfile.Method classMethod : javaClass.getMethods())
		{
			InternalMethodDefinition methodDef = InternalMethodDefinition.fromMethod(classMethod);
			InternalMethodImplementation methodImpl = InternalMethodImplementation.fromMethod(classDef, methodDef, classMethod, javaClass.getSourceFileName());
			methodImpl.processCode(infusion);

			if (!methodDef.getName().equals("<clinit>")) {
				throw new BuildException("ConstArrayClasses can only have a single static byte/short/int array and no methods.");
			}

			List<InstructionHandle> handles = methodImpl.getCodeBlock().getInstructions().getInstructionHandles();
			ConstArrayData array = new ConstArrayData();
			int array_index = 0;
			for (int i=0; i<handles.size(); i++) {
				InstructionHandle handle = methodImpl.getCodeBlock().getInstructions().get(i);
				Instruction instruction = handle.getInstruction();

				if (i == 0) {
					array.length = (int)((PushInstruction)instruction).getValue();
					array.data = new int[array.length];
				} else if (i == 1) {
					array.type = (int)((NewArrayInstruction)instruction).getType();
				} else if ((i-2) % 4 == 0) {
					if (instruction.getOpcode() == Opcode.ADUP) {
						// skip ADUP
					} else if (instruction.getOpcode() == Opcode.PUTSTATIC_A) {
						// Record the static field corresponding to this array
						AbstractField field = ((LocalIdInstruction)instruction).getField();
						array.field = field;
						constArrays.add(array);
					} else {
						throw new BuildException("ConstArray: expecting ADUP or PUTSTATIC_A");
					}
				} else if ((i-2) % 4 == 1) {
					if (instruction.getOpcode() == Opcode.RETURN) {
						// This is the end of clinit
						// Put the data in the string table
						byte[] bytearray;
						if (array.type == T_INT) {
							bytearray = new byte[array.length*4];
						} else if (array.type == T_SHORT) {
							bytearray = new byte[array.length*2];
						} else {
							bytearray = new byte[array.length];
						}
						for (int j=0; j<array.length; j++) {
							if (array.type == T_INT) {
								bytearray[4*j+0] = (byte)(array.data[j] >>>  0 & 0xFF);
								bytearray[4*j+1] = (byte)(array.data[j] >>>  8 & 0xFF);
								bytearray[4*j+2] = (byte)(array.data[j] >>> 16 & 0xFF);
								bytearray[4*j+3] = (byte)(array.data[j] >>> 24 & 0xFF);
							} else if (array.type == T_SHORT) {
								bytearray[2*j+0] = (byte)(array.data[j] >>>  0 & 0xFF);
								bytearray[2*j+1] = (byte)(array.data[j] >>>  8 & 0xFF);
							} else {
								bytearray[j]     = (byte)(array.data[j]);
							}
						}
						array.stringTableId = infusion.getStringTable().addByteArray(bytearray).getEntityId();

						System.err.println("Const array: ");
						System.err.println("   length: " + array.length);
						System.err.println("     type: " + array.type);
						System.err.println("    field: " + array.field.getName());
						System.err.println(" stringId: " + array.stringTableId);
						System.err.print("       data:");
						for (int j=0; j<array.length; j++) {
							System.err.print(" " + array.data[j]);
						}
						System.err.println("");

					} else {
						// This is the index into the constant array
						array_index = (int)((PushInstruction)instruction).getValue();
						if (array_index != ((i-2)/4)) {
							throw new BuildException("ConstArray: incorrect index");
						}
					}
				} else if ((i-2) % 4 == 2) {
					// This is the value to store in the constant array
					int value = (int)((PushInstruction)instruction).getValue();
					array.data[array_index] = value;
				} else if ((i-2) % 4 == 3) {
					// Skip putarray
				}
			}
		}
	}

	private Opcode getConstArrayOpcode(Opcode opcode) {
		switch (opcode) {
			case GETARRAY_B: return Opcode.GETCONSTARRAY_B;
			case GETARRAY_C: return Opcode.GETCONSTARRAY_C;
			case GETARRAY_S: return Opcode.GETCONSTARRAY_S;
			case GETARRAY_I: return Opcode.GETCONSTARRAY_I;
			default: return null;
		}
	}

	@Override
	protected void transformInternal()
	{
		InstructionList instructions = codeBlock.getInstructions();

		// Replace all GETARRAY_* instructions that get their data from a const array with a GETCONSTARRAY_* instruction
		for (InstructionHandle handle : instructions.getInstructionHandles())
		{
			Instruction instruction = handle.getInstruction();
			if (getConstArrayOpcode(instruction.getOpcode()) != null) {
				GeneratedValueSet valueSet = handle.getPreState().getStack().peek(1);
				if (valueSet.isConstArrayGETSTATICValue()) {
					// This is an array load from a constant array. Replace it with a constant array load instruction.
					handle.setInstruction(new GetConstArrayInstruction(getConstArrayOpcode(instruction.getOpcode()), (byte)valueSet.getConstArrayFieldId()));
				}
			}
		}

		// Remove all GETSTATIC_A that load a const array onto the stack
		for (int i=0; i<instructions.size(); i++)
		{
			InstructionHandle handle = instructions.get(i);
			Instruction instruction = handle.getInstruction();
			if (instruction.getOpcode() == Opcode.GETSTATIC_A) {
				if (handle.getPostState().getStack().peek().isConstArrayGETSTATICValue()) {
					// This is a GETSTATIC_A that loads a const array. Remove it.
					instructions.remove(handle);
					i--; // Instructions will have shifted by 1 and theoretically the next could also load a const array, so revisit this index.
				}
			}
		}
	}
}


// [ant:infuser] ########################################ConstArray class: javax.rtcbench.Data
// [ant:infuser] ######################################## method: <clinit>
// [ant:infuser] ######################################## instruction: bspush
// [ant:infuser] ######################################## instruction: newarray

// [ant:infuser] ######################################## instruction: adup
// [ant:infuser] ######################################## instruction: sconst_0
// [ant:infuser] ######################################## instruction: bspush
// [ant:infuser] ######################################## instruction: putarray_b

// [ant:infuser] ######################################## instruction: adup
// [ant:infuser] ######################################## instruction: sconst_1
// [ant:infuser] ######################################## instruction: bspush
// [ant:infuser] ######################################## instruction: putarray_b

// [ant:infuser] ######################################## instruction: adup
// [ant:infuser] ######################################## instruction: sconst_2
// [ant:infuser] ######################################## instruction: bspush
// [ant:infuser] ######################################## instruction: putarray_b

// [ant:infuser] ######################################## instruction: adup
// [ant:infuser] ######################################## instruction: sconst_3
// [ant:infuser] ######################################## instruction: bspush
// [ant:infuser] ######################################## instruction: putarray_b

// [ant:infuser] ######################################## instruction: adup
// [ant:infuser] ######################################## instruction: sconst_4
// [ant:infuser] ######################################## instruction: bspush
// [ant:infuser] ######################################## instruction: putarray_b

// [ant:infuser] ######################################## instruction: adup
// [ant:infuser] ######################################## instruction: sconst_5
// [ant:infuser] ######################################## instruction: bspush
// [ant:infuser] ######################################## instruction: putarray_b

// [ant:infuser] ######################################## instruction: putstatic_a
// [ant:infuser] ######################################## instruction: return