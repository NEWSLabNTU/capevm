namespace ProcessOutput

module JVM =
    let jvmOpcodeCategories = 
        [("01) Ref stack ld/st", ["JVM_ALOAD"; "JVM_ALOAD_0"; "JVM_ALOAD_1"; "JVM_ALOAD_2"; "JVM_ALOAD_3"; "JVM_ASTORE"; "JVM_ASTORE_0"; "JVM_ASTORE_1"; "JVM_ASTORE_2"; "JVM_ASTORE_3"; "JVM_GETFIELD_A"; "JVM_PUTFIELD_A"; "JVM_GETFIELD_A_FIXED"; "JVM_PUTFIELD_A_FIXED"; "JVM_GETSTATIC_A"; "JVM_PUTSTATIC_A"; "JVM_APOP"; "JVM_ADUP"]);
         ("02) Int stack ld/st", ["JVM_SLOAD"; "JVM_SLOAD_0"; "JVM_SLOAD_1"; "JVM_SLOAD_2"; "JVM_SLOAD_3"; "JVM_ILOAD"; "JVM_ILOAD_0"; "JVM_ILOAD_1"; "JVM_ILOAD_2"; "JVM_ILOAD_3"; "JVM_SSTORE"; "JVM_SSTORE_0"; "JVM_SSTORE_1"; "JVM_SSTORE_2"; "JVM_SSTORE_3"; "JVM_ISTORE"; "JVM_ISTORE_0"; "JVM_ISTORE_1"; "JVM_ISTORE_2"; "JVM_ISTORE_3"; "JVM_IPOP"; "JVM_IPOP2"; "JVM_IDUP"; "JVM_IDUP2"; "JVM_IDUP_X"; "JVM_GETFIELD_B"; "JVM_GETFIELD_C"; "JVM_GETFIELD_S"; "JVM_GETFIELD_I"; "JVM_PUTFIELD_B"; "JVM_PUTFIELD_C"; "JVM_PUTFIELD_S"; "JVM_PUTFIELD_I"; "JVM_GETSTATIC_B"; "JVM_GETSTATIC_C"; "JVM_GETSTATIC_S"; "JVM_GETSTATIC_I"; "JVM_PUTSTATIC_B"; "JVM_PUTSTATIC_C"; "JVM_PUTSTATIC_S"; "JVM_PUTSTATIC_I"]);
         ("03) Constant load", ["JVM_SCONST_M1"; "JVM_SCONST_0"; "JVM_SCONST_1"; "JVM_SCONST_2"; "JVM_SCONST_3"; "JVM_SCONST_4"; "JVM_SCONST_5"; "JVM_ICONST_M1"; "JVM_ICONST_0"; "JVM_ICONST_1"; "JVM_ICONST_2"; "JVM_ICONST_3"; "JVM_ICONST_4"; "JVM_ICONST_5"; "JVM_ACONST_NULL"; "JVM_BSPUSH"; "JVM_BIPUSH"; "JVM_SSPUSH"; "JVM_SIPUSH"; "JVM_IIPUSH"; "JVM_LDS"]);
         ("04) Array ld/st", ["JVM_GETARRAY_B"; "JVM_GETARRAY_C"; "JVM_GETARRAY_S"; "JVM_GETARRAY_I"; "JVM_GETARRAY_A"; "JVM_PUTARRAY_B"; "JVM_PUTARRAY_C"; "JVM_PUTARRAY_S"; "JVM_PUTARRAY_I"; "JVM_PUTARRAY_A"]);
         ("05) Branches", ["JVM_SIFEQ"; "JVM_SIFNE"; "JVM_SIFLT"; "JVM_SIFGE"; "JVM_SIFGT"; "JVM_SIFLE"; "JVM_IIFEQ"; "JVM_IIFNE"; "JVM_IIFLT"; "JVM_IIFGE"; "JVM_IIFGT"; "JVM_IIFLE"; "JVM_IFNULL"; "JVM_IFNONNULL"; "JVM_IF_SCMPEQ"; "JVM_IF_SCMPNE"; "JVM_IF_SCMPLT"; "JVM_IF_SCMPGE"; "JVM_IF_SCMPGT"; "JVM_IF_SCMPLE"; "JVM_IF_ICMPEQ"; "JVM_IF_ICMPNE"; "JVM_IF_ICMPLT"; "JVM_IF_ICMPGE"; "JVM_IF_ICMPGT"; "JVM_IF_ICMPLE"; "JVM_IF_ACMPEQ"; "JVM_IF_ACMPNE"; "JVM_GOTO"; "JVM_TABLESWITCH"; "JVM_LOOKUPSWITCH"; "JVM_BRTARGET"; "JVM_MARKLOOP" ]);
         ("06) Math", ["JVM_SADD"; "JVM_SSUB"; "JVM_SMUL"; "JVM_SDIV"; "JVM_SREM"; "JVM_SNEG"; "JVM_SIMUL"; "JVM_IADD"; "JVM_ISUB"; "JVM_IMUL"; "JVM_IDIV"; "JVM_IREM"; "JVM_INEG"; "JVM_IINC"; "JVM_IINC_W"; "JVM_SINC"; "JVM_SINC_W"]);
         ("07) Bit shifts", ["JVM_SSHL"; "JVM_SSHR"; "JVM_SUSHR"; "JVM_ISHL"; "JVM_ISHR"; "JVM_IUSHR"; "JVM_SSHL_CONST"; "JVM_SSHR_CONST"; "JVM_SUSHR_CONST"; "JVM_ISHL_CONST"; "JVM_ISHR_CONST"; "JVM_IUSHR_CONST"]);
         ("08) Bit logic", ["JVM_SAND"; "JVM_SOR"; "JVM_SXOR"; "JVM_IAND"; "JVM_IOR"; "JVM_IXOR"]);
         ("09) Conversions", ["JVM_S2B"; "JVM_S2C"; "JVM_S2I"; "JVM_I2S"]);
         ("10) Markloop", ["JVM_MARKLOOP_START"; "JVM_MARKLOOP_END"]);
         ("11) Invoke", ["JVM_INVOKEVIRTUAL"; "JVM_INVOKESPECIAL"; "JVM_INVOKESTATIC"; "JVM_INVOKEINTERFACE"; "JVM_INVOKELIGHT"]);
         ("12) Others", ["MethodPreamble"; "JVM_START_AOT_MEASUREMENT"; "JVM_STOP_AOT_MEASUREMENT"; "JVM_NOP"; "JVM_SRETURN"; "JVM_IRETURN"; "JVM_ARETURN"; "JVM_RETURN"; "JVM_NEW"; "JVM_NEWARRAY"; "JVM_ANEWARRAY"; "JVM_ARRAYLENGTH"; "JVM_CHECKCAST"; "JVM_INSTANCEOF"])] in

    let getCategoryForJvmOpcode opcode =
        match jvmOpcodeCategories |> List.tryFind (fun (cat, opcodes) -> opcodes |> List.exists ((=) opcode)) with
        | Some(cat, _) -> cat
        | None -> "13) ????"

    let getAllJvmOpcodeCategories =
        jvmOpcodeCategories |> List.map (fun (cat, opcodes) -> cat)
