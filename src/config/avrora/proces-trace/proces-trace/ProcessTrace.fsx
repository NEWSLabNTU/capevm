#r "binaries/FSharp.Data/FSharp.Data.dll"
#r "binaries/fspickler.1.5.2/lib/net45/FsPickler.dll"

#load "Datatypes.fsx"
#load "Helpers.fsx"
#load "AVR.fsx"
#load "JVM.fsx"

open System
open System.IO
open System.Linq
open System.Text.RegularExpressions
open System.Runtime.Serialization
open FSharp.Data
open Nessos.FsPickler
open Datatypes
open Helpers

type RtcdataXml = XmlProvider<"rtcdata-example.xml", Global=true>
type Rtcdata = RtcdataXml.Methods
type MethodImpl = RtcdataXml.MethodImpl
type ProfilerdataXml = XmlProvider<"profilerdata-example.xml", Global=true>
type Profilerdata = ProfilerdataXml.ExecutionCountPerInstruction
type ProfiledInstruction = ProfilerdataXml.Instruction
type DarjeelingInfusionHeaderXml = XmlProvider<"infusionheader-example.dih", Global=true>
type Dih = DarjeelingInfusionHeaderXml.Dih

let JvmInstructionFromXml (xml : RtcdataXml.JavaInstruction) =
    {
        JvmInstruction.index = xml.Index
        text = xml.Text
    }
let AvrInstructionFromXml (xml : RtcdataXml.AvrInstruction) =
    let opcode = Convert.ToInt32((xml.Opcode.Trim()), 16)
    {
        AvrInstruction.address = Convert.ToInt32(xml.Address.Trim(), 16)
        opcode = opcode
        text = xml.Text
    }

// Input: the optimised avr code, and a list of tuples of unoptimised avr instructions and the jvm instruction that generated them
// Returns: a list of tuples (optimised avr instruction, unoptimised avr instruction, corresponding jvm index)
//          the optimised avr instruction may be None for instructions that were removed completely by the optimiser
let rec matchOptUnopt (optimisedAvr : AvrInstruction list) (unoptimisedAvr : (AvrInstruction*JvmInstruction) list) =
    let isAOT_PUSH x = AVR.is AVR.PUSH (x.opcode) || AVR.is AVR.ST_XINC (x.opcode) // AOT uses 2 stacks
    let isAOT_POP x = AVR.is AVR.POP (x.opcode) || AVR.is AVR.LD_DECX (x.opcode)
    let isMOV x = AVR.is AVR.MOV (x.opcode)
    let isMOVW x = AVR.is AVR.MOVW (x.opcode)
    let isBREAK x = AVR.is AVR.BREAK (x.opcode)
    let isNOP x = AVR.is AVR.NOP (x.opcode)
    let isJMP x = AVR.is AVR.JMP (x.opcode)
    let isRJMP x = AVR.is AVR.RJMP (x.opcode)
    let isBRANCH x = AVR.is AVR.BREQ (x.opcode) || AVR.is AVR.BRGE (x.opcode) || AVR.is AVR.BRLT (x.opcode) || AVR.is AVR.BRNE (x.opcode)
    let isBRANCH_BY_BYTES x y = isBRANCH x && ((((x.opcode) &&& (0x03F8)) >>> 2) = y) // avr opcode BRxx 0000 00kk kkkk k000, with k the offset in WORDS (thus only shift right by 2, not 3, to get the number of bytes)

    let isMOV_MOVW_PUSH_POP x = isMOV x || isMOVW x || isAOT_PUSH x || isAOT_POP x
    match optimisedAvr, unoptimisedAvr with
    // Identical instructions: match and consume both
    | optimisedHead :: optimisedTail, (unoptimisedHead, jvmHead) :: unoptTail when optimisedHead.text = unoptimisedHead.text
        -> (Some optimisedHead, unoptimisedHead, jvmHead) :: matchOptUnopt optimisedTail unoptTail
    // Match a MOV to a single PUSH instruction (bit arbitrary whether to count the cycle for the PUSH or POP that was optimised)
    | optMOV :: optTail, (unoptPUSH, jvmHead) :: unoptTail when isMOV(optMOV) && isAOT_PUSH(unoptPUSH)
        -> (Some optMOV, unoptPUSH, jvmHead)
            :: matchOptUnopt optTail unoptTail
    // Match a MOVW to two PUSH instructions (bit arbitrary whether to count the cycle for the PUSH or POP that was optimised)
    | optMOVW :: optTail, (unoptPUSH1, jvmHead1) :: (unoptPUSH2, jvmHead2) :: unoptTail when isMOVW(optMOVW) && isAOT_PUSH(unoptPUSH1) && isAOT_PUSH(unoptPUSH2)
        -> (Some optMOVW, unoptPUSH1, jvmHead1)
            :: (None, unoptPUSH2, jvmHead2)
            :: matchOptUnopt optTail unoptTail
    // If the unoptimised head is a MOV PUSH or POP, skip it
    | _, (unoptimisedHead, jvmHead) :: unoptTail when isMOV_MOVW_PUSH_POP(unoptimisedHead)
        -> (None, unoptimisedHead, jvmHead) :: matchOptUnopt optimisedAvr unoptTail
    // BREAK signals a branchtag that would have been replaced in the optimised code by a
    // branch to the real address, possibly followed by one or two NOPs
    | _, (unoptBranchtag, jvmBranchtag) :: (unoptBranchtag2, jvmBranchtag2) :: unoptTail when isBREAK(unoptBranchtag)
        -> match optimisedAvr with
            // Short conditional jump, followed by a JMP or RJMP which was generated by a GOTO -> only the BR should match with the current JVM instruction
            | optBR :: optJMPRJMP :: optNoJMPRJMP :: optTail when isBRANCH(optBR) && (isJMP(optJMPRJMP) || isRJMP(optJMPRJMP)) && not (isJMP(optNoJMPRJMP) || isRJMP(optNoJMPRJMP)) && isBREAK(fst(unoptTail |> List.head))
                -> (Some optBR, unoptBranchtag, jvmBranchtag)
                    :: matchOptUnopt (optJMPRJMP :: optNoJMPRJMP :: optTail) unoptTail
            // Long conditional jump (branch by 2 or 4 bytes to jump over the next RJMP or JMP)
            | optBR :: optJMP :: optTail when ((isBRANCH_BY_BYTES optBR 2) || (isBRANCH_BY_BYTES optBR 4)) && isJMP(optJMP)
                -> (Some optBR, unoptBranchtag, jvmBranchtag)
                    :: (Some optJMP, unoptBranchtag, jvmBranchtag)
                    :: matchOptUnopt optTail unoptTail
            // Mid range conditional jump (branch by 2 or 4 bytes to jump over the next RJMP or JMP)
            | optBR :: optRJMP :: optTail when ((isBRANCH_BY_BYTES optBR 2) || (isBRANCH_BY_BYTES optBR 4)) && isRJMP(optRJMP)
                -> (Some optBR, unoptBranchtag, jvmBranchtag)
                    :: (Some optRJMP, unoptBranchtag, jvmBranchtag)
                    :: matchOptUnopt optTail unoptTail
            // Short conditional jump
            | optBR :: optTail when isBRANCH(optBR)
                -> (Some optBR, unoptBranchtag, jvmBranchtag)
                    :: matchOptUnopt optTail unoptTail
            // Uncondtional long jump
            | optJMP :: optTail when isJMP(optJMP)
                -> (Some optJMP, unoptBranchtag, jvmBranchtag)
                    :: matchOptUnopt optTail unoptTail
            // Uncondtional mid range jump
            | optRJMP :: optTail when isRJMP(optRJMP)
                -> (Some optRJMP, unoptBranchtag, jvmBranchtag)
                    :: matchOptUnopt optTail unoptTail
            | head :: tail -> failwith ("Incorrect branchtag @ address " + head.address.ToString() + ": " + head.text + " / " + unoptBranchtag2.text)
            | _ -> failwith "Incorrect branchtag"
    | [], [] -> [] // All done.
    | head :: tail, [] -> failwith ("Some instructions couldn't be matched(1): " + head.text)
    | [], (head, jvm) :: tail -> failwith ("Some instructions couldn't be matched(2): " + head.text)
    | head1 :: tail1, (head2, jvm) :: tail2 -> failwith ("Some instructions couldn't be matched: " + head1.address.ToString() + ":" + head1.text + "   +   " + head2.address.ToString() + ":" + head2.text)

// Input: the original Java instructions, trace data from avrora profiler, and the output from matchOptUnopt
// Returns: a list of ResultJava records, showing the optimised code per original JVM instructions, and amount of cycles spent per optimised instruction
let addCountersAndDebugData (jvmInstructions : JvmInstruction list) (countersForAddressAndInst : int -> int -> ExecCounters) (matchedResults : (AvrInstruction option*AvrInstruction*JvmInstruction) list) (djDebugDatas : DJDebugData list) =
    List.zip jvmInstructions (DJDebugData.empty :: djDebugDatas) // Add an empty entry to debug data to match the method preamble
        |> List.map
        (fun (jvm, debugdata) ->
            let resultsForThisJvm = matchedResults |> List.filter (fun b -> let (_, _, jvm2) = b in jvm.index = jvm2.index) in
            let resultsWithCounters = resultsForThisJvm |> List.map (fun (opt, unopt, _) ->
                  let counters = match opt with
                                 | None -> ExecCounters.empty
                                 | Some(optValue) -> countersForAddressAndInst optValue.address optValue.opcode
                  { unopt = unopt; opt = opt; counters = counters }) in
            let foldAvrCountersToJvmCounters a b =
                { cycles = a.cycles+b.cycles; executions = (if a.executions > 0 then a.executions else b.executions); size = a.size+b.size; count = a.count+b.count }
            { jvm = jvm
              avr = resultsWithCounters
              counters = resultsWithCounters |> List.map (fun r -> r.counters) |> List.fold (foldAvrCountersToJvmCounters) ExecCounters.empty
              djDebugData = debugdata })

let getTimersFromStdout (stdoutlog : string list) =
    let pattern = "Timer (\w+) ran (\d+) times for a total of (\d+) cycles."
    stdoutlog |> List.map (fun line -> Regex.Match(line, pattern))
              |> List.filter (fun regexmatch -> regexmatch.Success)
              |> List.map (fun regexmatch -> (regexmatch.Groups.[1].Value, Int32.Parse(regexmatch.Groups.[3].Value)))
              |> List.sortBy (fun (timer, cycles) -> match timer with "NATIVE" -> 1 | "AOT" -> 2 | "JAVA" -> 3 | _ -> 4)

let getNativeInstructionsFromObjdump (objdumpOutput : string list) (countersForAddressAndInst : int -> int -> ExecCounters) =
    let startIndex = objdumpOutput |> List.findIndex (fun line -> Regex.IsMatch(line, "^[0-9a-fA-F]+ <rtcbenchmark_measure_native_performance(\.constprop\.\d*)?>:$"))
    let disasmTail = objdumpOutput |> List.skip (startIndex + 1)
    let endIndex = disasmTail |> List.findIndex (fun line -> Regex.IsMatch(line, "^[0-9a-fA-F]+ <.*>:$"))
    let disasm = disasmTail |> List.take endIndex |> List.filter ((<>) "")
    let pattern = "^\s*([0-9a-fA-F]+):((\s[0-9a-fA-F][0-9a-fA-F])+)\s+(\S.*)$"
    let regexmatches = disasm |> List.map (fun line -> Regex.Match(line, pattern))
    let avrInstructions = regexmatches |> List.map (fun regexmatch ->
        let opcodeBytes = regexmatch.Groups.[2].Value.Split(' ') |> Array.map (fun x -> Convert.ToInt32(x.Trim(), 16))
        let opcode = if (opcodeBytes.Length = 2)
                     then ((opcodeBytes.[1] <<< 8) + opcodeBytes.[0])
                     else ((opcodeBytes.[3] <<< 24) + (opcodeBytes.[2] <<< 16) + (opcodeBytes.[1] <<< 8) + opcodeBytes.[0])
        {
            AvrInstruction.address = Convert.ToInt32(regexmatch.Groups.[1].Value, 16)
            opcode = opcode
            text = regexmatch.Groups.[4].Value
        })
    avrInstructions |> List.map (fun avr -> (avr, countersForAddressAndInst avr.address avr.opcode))

let parseDJDebug (allLines : string list) =
  let regexLine = Regex("^\s*(?<byteOffset>\d\d\d\d);[^;]*;[^;]*;(?<text>[^;]*);(?<stackBefore>[^;]*);(?<stackAfter>[^;]*);.*")
  let regexStackElement = Regex("^(?<byteOffset>\d+)(?<datatype>[a-zA-Z]+)$")

  let startIndex = allLines |> List.findIndex (fun line -> Regex.IsMatch(line, "^\s*method.*rtcbenchmark_measure_java_performance$"))
  let linesTail = allLines |> List.skip (startIndex + 3)
  let endIndex = linesTail |> List.findIndex (fun line -> Regex.IsMatch(line, "^\s*}\s*$"))
  let lines = linesTail |> List.take endIndex |> List.filter ((<>) "")

  // System.Console.WriteLine (String.Join("\r\n", lines))

  let regexLineMatches = lines |> List.map (fun x -> regexLine.Match(x))
  let byteOffsetToInstOffset = regexLineMatches |> List.mapi (fun i m -> (Int32.Parse(m.Groups.["byteOffset"].Value.Trim()), i)) |> Map.ofList
  let stackStringToStack (stackString: string) =
      let split = stackString.Split(',')
                  |> Seq.toList
                  |> List.map (fun x -> match x.IndexOf('(') with // Some elements are in the form 20Short(Byte) to indicate Darjeeling knows the short value only contains a byte. Strip this information for now.
                                        | -1 -> x
                                        | index -> x.Substring(0, index))
                  |> List.filter ((<>) "")
      let regexElementMatches = split |> List.map (fun x -> regexStackElement.Match(x))
      // Temporarily just fill each index with 0 since the debug output from Darjeeling has a bug when instructions are replaced. The origin of each stack element is determined before replacing DUP and POP instructions.
      // So after replacing them, the indexes are no longer valid. Too much work to fix it properly in DJ for now. Will do that later if necessary.3
      // regexElementMatches |> List.map (fun m -> let byteOffset = Int32.Parse(m.Groups.["byteOffset"].Value) in
      //                                           let instOffset = if (Map.containsKey byteOffset byteOffsetToInstOffset)
      //                                                            then Map.find byteOffset byteOffsetToInstOffset
      //                                                            else failwith ("Key not found!!!! " + byteOffset.ToString() + " in " + string) in
      //                                           let datatype = (m.Groups.["datatype"].Value |> StackDatatypeFromString) in
      //                                           { StackElement.origin=instOffset; datatype=datatype })
      regexElementMatches |> List.map (fun m -> { StackElement.origin=0; datatype=(m.Groups.["datatype"].Value |> StackDatatypeFromString) })
  regexLineMatches |> List.mapi (fun i m -> { byteOffset = Int32.Parse(m.Groups.["byteOffset"].Value.Trim());
                                          instOffset = i;
                                          text = m.Groups.["text"].Value.Trim();
                                          stackBefore = (m.Groups.["stackBefore"].Value.Trim() |> stackStringToStack);
                                          stackAfter = (m.Groups.["stackAfter"].Value.Trim()  |> stackStringToStack) })

let getLoops results =
  let jvmInstructions = results.jvmInstructions |> List.map (fun jvm -> jvm.jvm)
  let filteredInstructions = jvmInstructions |> List.filter (fun jvm -> jvm.text.Contains("MARKLOOP") || jvm.text.Contains("JVM_BRTARGET") || jvm.text.Contains("Branch target"))
  filteredInstructions |> List.map (fun jvm -> String.Format("{0} {1}\n\r", jvm.index, jvm.text)) |> List.fold (+) ""

// Process trace main function
let processTrace benchmark (dih : Dih) (rtcdata : Rtcdata) (countersForAddressAndInst : int -> int -> ExecCounters) (stdoutlog : string list) (disasm : string list) (djdebuglines : string list) =
    // Find the methodImplId for a certain method in a Darjeeling infusion header
    let findRtcbenchmarkMethodImplId (dih : Dih) methodName =
        let infusionName = dih.Infusion.Header.Name
        let methodDef = dih.Infusion.Methoddeflists |> Seq.find (fun def -> def.Name = methodName)
        let methodImpl = dih.Infusion.Methodimpllists |> Seq.find (fun impl -> impl.MethoddefEntityId = methodDef.EntityId && impl.MethoddefInfusion = infusionName)
        methodImpl.EntityId

    let methodImplId = findRtcbenchmarkMethodImplId dih "rtcbenchmark_measure_java_performance"
    let methodImpl = rtcdata.MethodImpls |> Seq.find (fun impl -> impl.MethodImplId = methodImplId)

    let optimisedAvr = methodImpl.AvrInstructions |> Seq.map AvrInstructionFromXml |> Seq.toList
    let unoptimisedAvrWithJvmIndex =
        methodImpl.JavaInstructions |> Seq.map (fun jvm -> jvm.UnoptimisedAvr.AvrInstructions |> Seq.map (fun avr -> (AvrInstructionFromXml avr, JvmInstructionFromXml jvm)))
                                    |> Seq.concat
                                    |> Seq.toList

    let matchedResult = matchOptUnopt optimisedAvr unoptimisedAvrWithJvmIndex
    let djdebugdata = parseDJDebug djdebuglines
    let processedJvmInstructions = addCountersAndDebugData (methodImpl.JavaInstructions |> Seq.map JvmInstructionFromXml |> Seq.toList) countersForAddressAndInst matchedResult djdebugdata
    let stopwatchTimers = getTimersFromStdout stdoutlog

    let countersPerJvmOpcodeAOTJava =
        processedJvmInstructions
            |> List.filter (fun r -> r.jvm.text <> "Method preamble")
            |> groupFold (fun r -> r.jvm.text.Split().First()) (fun r -> r.counters) (+) ExecCounters.empty
            |> List.map (fun (opc, cnt) -> (JVM.getCategoryForJvmOpcode opc, opc, cnt))
            |> List.sortBy (fun (cat, opc, _) -> cat+opc)

    let countersPerAvrOpcodeAOTJava =
        processedJvmInstructions
            |> List.map (fun r -> r.avr)
            |> List.concat
            |> List.filter (fun avr -> avr.opt.IsSome)
            |> List.map (fun avr -> (AVR.getOpcodeForInstruction avr.opt.Value.opcode avr.opt.Value.text, avr.counters))
            |> groupFold fst snd (+) ExecCounters.empty
            |> List.map (fun (opc, cnt) -> (AVR.opcodeCategory opc, AVR.opcodeName opc, cnt))
            |> List.sortBy (fun (cat, opc, _) -> cat+opc)

    let nativeCInstructions = getNativeInstructionsFromObjdump disasm countersForAddressAndInst
    let countersPerAvrOpcodeNativeC =
        nativeCInstructions
            |> List.map (fun (avr, cnt) -> (AVR.getOpcodeForInstruction avr.opcode avr.text, cnt))
            |> groupFold fst snd (+) ExecCounters.empty
            |> List.map (fun (opc, cnt) -> (AVR.opcodeCategory opc, AVR.opcodeName opc, cnt))
            |> List.sortBy (fun (cat, opc, _) -> cat+opc)

    let codesizeJava = methodImpl.JvmMethodSize
    let codesizeJavaBranchCount = methodImpl.BranchCount
    let codesizeJavaBranchTargetCount = methodImpl.BranchTargets |> Array.length
    let codesizeJavaMarkloopCount = methodImpl.MarkloopCount
    let codesizeJavaMarkloopTotalSize = methodImpl.MarkloopTotalSize
    let codesizeJavaWithoutBranchOverhead = codesizeJava - (2*codesizeJavaBranchCount) - codesizeJavaBranchTargetCount
    let codesizeJavaWithoutBranchMarkloopOverhead = codesizeJava - (2*codesizeJavaBranchCount) - codesizeJavaBranchTargetCount - codesizeJavaMarkloopTotalSize
    let codesizeAOT =
        let numberOfBranchTargets = methodImpl.BranchTargets |> Array.length
        methodImpl.AvrMethodSize - (4*numberOfBranchTargets) // We currently keep the branch table at the head of the method, but actually we don't need it anymore after code generation, so it shouldn't count for code size.
    let codesizeC =
        let startAddress = (nativeCInstructions |> List.head |> fst).address
        let lastInList x = x |> List.reduce (fun _ x -> x)
        let endAddress = (nativeCInstructions |> lastInList |> fst).address
        endAddress - startAddress + 2 // assuming the function ends in a 2 byte opcode.
    let getTimer timer =
        match stopwatchTimers |> List.tryFind (fun (t,c) -> t=timer) with
        | Some(x) -> x |> snd
        | None -> 0

    {
        benchmark = benchmark
        jvmInstructions = processedJvmInstructions
        nativeCInstructions = nativeCInstructions |> Seq.toList
        passedTestJava = stdoutlog |> List.exists (fun line -> line.Contains("JAVA OK."))
        passedTestAOT = stdoutlog |> List.exists (fun line -> line.Contains("RTC OK."))

        codesizeJava = codesizeJava
        codesizeJavaBranchCount = codesizeJavaBranchCount
        codesizeJavaBranchTargetCount = codesizeJavaBranchTargetCount
        codesizeJavaMarkloopCount = codesizeJavaMarkloopCount
        codesizeJavaMarkloopTotalSize = codesizeJavaMarkloopTotalSize
        codesizeJavaWithoutBranchOverhead = codesizeJavaWithoutBranchOverhead
        codesizeJavaWithoutBranchMarkloopOverhead = codesizeJavaWithoutBranchMarkloopOverhead
        codesizeAOT = codesizeAOT
        codesizeC = codesizeC

        cyclesStopwatchJava = (getTimer "JAVA")
        cyclesStopwatchAOT = (getTimer "AOT")
        cyclesStopwatchC = (getTimer "NATIVE")

        countersPerJvmOpcodeAOTJava = countersPerJvmOpcodeAOTJava
        countersPerAvrOpcodeAOTJava = countersPerAvrOpcodeAOTJava
        countersPerAvrOpcodeNativeC = countersPerAvrOpcodeNativeC
    }

let resultsToString (results : SimulationResults) =
    let totalCyclesAOTJava = results.countersAOTTotal.cycles
    let totalCyclesNativeC = results.countersCTotal.cycles
    let totalBytesAOTJava = results.countersAOTTotal.size
    let totalBytesNativeC = results.countersCTotal.size
    let cyclesToSlowdown cycles1 cycles2 =
        String.Format ("{0:0.00}", float cycles1 / float cycles2)
    let stackToString stack =
        String.Join(",", stack |> List.map (fun el -> el.datatype |> StackDatatypeToString))

    let countersToString totalCycles totalBytes (counters : ExecCounters) =
        String.Format("cyc:{0,8} {1,5:0.0}% {2,5:0.0}%C exe:{3,8}  avg:{4,5:0.0} byt:{5,5} {6,5:0.0}% {7,5:0.0}%C",
                      counters.cycles,
                      100.0 * float (counters.cycles) / float totalCycles,
                      100.0 * float (counters.cycles) / float totalCyclesNativeC,
                      counters.executions,
                      counters.average,
                      counters.size,
                      100.0 * float (counters.size) / float totalBytes,
                      100.0 * float (counters.size) / float totalBytesNativeC)

    let resultJavaListingToString (result : ProcessedJvmInstruction) =
        String.Format("{0,-60}{1} {2}->{3}:{4}\r\n",
                      result.jvm.text,
                      countersToString totalCyclesAOTJava totalBytesAOTJava result.counters,
                      result.djDebugData.stackSizeBefore,
                      result.djDebugData.stackSizeAfter,
                      stackToString result.djDebugData.stackAfter)

    let resultsAvrToString (avrResults : ResultAvr list) =
        let avrInstOption2Text = function
            | Some (x : AvrInstruction)
                -> String.Format("0x{0,6:X6}: {1,-15}", x.address, x.text)
            | None -> "" in
        avrResults |> List.map (fun r -> String.Format("        {0,-15} -> {1,-36} {2,8} {3,14}\r\n",
                                                      r.unopt.text,
                                                      avrInstOption2Text r.opt,
                                                      r.counters.executions,
                                                      r.counters.cycles))
                   |> List.fold (+) ""

    let nativeCInstructionToString ((inst, counters) : AvrInstruction*ExecCounters) =
        String.Format("0x{0,6:X6}: {1}    {2}\r\n", inst.address, (countersToString totalCyclesNativeC totalBytesNativeC counters), inst.text)
    let opcodeResultsToString totalCycles totalBytes opcodeResults =
            opcodeResults
            |> List.map (fun (category, opcode, counters)
                             ->  String.Format("{0,-20}{1,-20} total {2}\r\n",
                                               category,
                                               opcode,
                                               (countersToString totalCycles totalBytes counters)))
            |> List.fold (+) ""
    let categoryResultsToString totalCycles totalBytes categoryResults =
            categoryResults
            |> List.map (fun (category, counters)
                             -> String.Format("{0,-40} total {1}\r\n",
                                              category,
                                              (countersToString totalCycles totalBytes counters)))
            |> List.fold (+) ""

    let testResultAOT = if results.passedTestAOT then "PASSED" else "FAILED"
    let testResultJAVA = if results.passedTestJava then "PASSED" else "FAILED"
    seq {
        yield "================== " + results.benchmark + ": AOT " + testResultAOT + ", Java " + testResultJAVA + " =================="
        yield String.Format ("--- STACK max                               {0,14}", (results.maxJvmStackInBytes))
        yield String.Format ("          avg/executed jvm                  {0,14:000.00}", results.avgJvmStackInBytes)
        yield String.Format ("          avg change/exec jvm               {0,14:000.00}", results.avgJvmStackChangeInBytes)
        yield "============================ STOPWATCHES ============================="
        yield String.Format ("--- STOPWATCHES Native C                    {0,14}", results.cyclesStopwatchC)
        yield String.Format ("                AOT                         {0,14}", results.cyclesStopwatchAOT)
        yield String.Format ("                JAVA                        {0,14}", results.cyclesStopwatchJava)
        yield String.Format ("                AOT/C                       {0,14}", (cyclesToSlowdown results.cyclesStopwatchAOT results.cyclesStopwatchC))
        yield String.Format ("                Java/C                      {0,14}", (cyclesToSlowdown results.cyclesStopwatchJava results.cyclesStopwatchC))
        yield String.Format ("                Java/AOT                    {0,14}", (cyclesToSlowdown results.cyclesStopwatchJava results.cyclesStopwatchAOT))
        yield "============================ MAIN COUNTERS ==========================="
        yield String.Format ("--- NAT.C    Cycles                         {0,12} (stopwatch {1}, ratio {2})", results.executedCyclesC, results.cyclesStopwatchC, (cyclesToSlowdown results.cyclesStopwatchC results.executedCyclesC))
        yield String.Format ("             Bytes                          {0,12} (address range {1}, ratio {2}) (off by 2 expected for methods ending in JMP)", totalBytesNativeC, results.codesizeC, (cyclesToSlowdown results.codesizeC totalBytesNativeC))
        yield String.Format ("             Total                          {0}", (countersToString totalCyclesNativeC totalBytesNativeC results.countersCTotal))
        yield String.Format ("              load/store                    {0}", (countersToString totalCyclesNativeC totalBytesNativeC results.countersCLoadStore))
        yield String.Format ("              push/pop int                  {0}", (countersToString totalCyclesNativeC totalBytesNativeC results.countersCPushPop))
        yield String.Format ("              mov(w)                        {0}", (countersToString totalCyclesNativeC totalBytesNativeC results.countersCMov))
        yield String.Format ("              others                        {0}", (countersToString totalCyclesNativeC totalBytesNativeC results.countersCOthers))
        yield ""
        yield String.Format ("--- AOT      Cycles                         {0,12} (stopwatch {1}, ratio {2})", results.executedCyclesAOT, results.cyclesStopwatchAOT, (cyclesToSlowdown results.cyclesStopwatchAOT results.executedCyclesAOT))
        yield String.Format ("             Bytes                          {0,12} (methodImpl.AvrMethodSize {1}, ratio {2})", totalBytesAOTJava, results.codesizeAOT, (cyclesToSlowdown results.codesizeAOT totalBytesAOTJava))
        yield String.Format ("             Total                          {0}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersAOTTotal))
        yield String.Format ("              load/store                    {0}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersAOTLoadStore))
        yield String.Format ("              push/pop int                  {0}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersAOTPushPopInt))
        yield String.Format ("              push/pop ref                  {0}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersAOTPushPopRef))
        yield String.Format ("              mov(w)                        {0}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersAOTMov))
        yield String.Format ("              others                        {0}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersAOTOthers))
        yield ""
        yield String.Format ("--- OVERHEAD Total                          {0,14}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersOverheadTotal))
        yield String.Format ("              load/store                    {0,14}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersOverheadLoadStore))
        yield String.Format ("              pushpop                       {0,14}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersOverheadPushPop))
        yield String.Format ("              mov(w)                        {0,14}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersOverheadMov))
        yield String.Format ("              others                        {0,14}", (countersToString totalCyclesAOTJava totalBytesAOTJava results.countersOverheadOthers))
        yield "============================ CODE SIZE ==============================="
        yield String.Format ("--- CODE SIZE   Native C                    {0,14}", (results.codesizeC))
        yield String.Format ("                AOT                         {0,14}", (results.codesizeAOT))
        yield String.Format ("                Java total                  {0,14}", (results.codesizeJava))
        yield String.Format ("                     branch count           {0,14}", (results.codesizeJavaBranchCount))
        yield String.Format ("                     branch target count    {0,14}", (results.codesizeJavaBranchTargetCount))
        yield String.Format ("                     markloop count         {0,14}", (results.codesizeJavaMarkloopCount))
        yield String.Format ("                     markloop size          {0,14}", (results.codesizeJavaMarkloopTotalSize))
        yield String.Format ("                     total-branch overhead  {0,14}", (results.codesizeJavaWithoutBranchOverhead))        
        yield String.Format ("                     total-br/mloop overh.  {0,14}", (results.codesizeJavaWithoutBranchMarkloopOverhead))        
        yield String.Format ("                AOT/C                       {0,14}", (cyclesToSlowdown results.codesizeAOT results.codesizeC))
        yield String.Format ("                AOT/Java                    {0,14}", (cyclesToSlowdown results.codesizeAOT results.codesizeJava))
        yield "============================ DETAILED COUNTERS ======================="
        yield "--- SUMMED: PER JVM CATEGORY"
        yield categoryResultsToString totalCyclesAOTJava totalCyclesNativeC results.countersPerJvmOpcodeCategoryAOTJava
        yield ""
        yield "--- SUMMED: PER AVR CATEGORY (Java AOT)"
        yield categoryResultsToString totalCyclesAOTJava totalCyclesNativeC results.countersPerAvrOpcodeCategoryAOTJava
        yield ""
        yield "--- SUMMED: PER AVR CATEGORY (NATIVE C)"
        yield categoryResultsToString totalCyclesNativeC totalCyclesNativeC results.countersPerAvrOpcodeCategoryNativeC
        yield ""
        yield "--- SUMMED: PER JVM OPCODE"
        yield opcodeResultsToString totalCyclesAOTJava totalCyclesNativeC results.countersPerJvmOpcodeAOTJava
        yield ""
        yield "--- SUMMED: PER AVR OPCODE (Java AOT)"
        yield opcodeResultsToString totalCyclesAOTJava totalCyclesNativeC results.countersPerAvrOpcodeAOTJava
        yield ""
        yield "--- SUMMED: PER AVR OPCODE (NATIVE C)"
        yield opcodeResultsToString totalCyclesNativeC totalCyclesNativeC results.countersPerAvrOpcodeNativeC
        yield ""
        yield "============================ LISTINGS ================================"
        yield ""
        yield "--- LISTING: NATIVE C AVR\r\n"
        yield results.nativeCInstructions
                |> List.map nativeCInstructionToString
                |> List.fold (+) ""
        yield ""
        yield "--- LISTING: ONLY JVM"
        yield results.jvmInstructions
                |> List.map resultJavaListingToString
                |> List.fold (+) ""
        yield ""
        yield "--- LISTING: JVM LOOPS"
        yield (getLoops results)
        yield ""
        yield "--- LISTING: ONLY OPTIMISED AVR"
        yield results.jvmInstructions
                |> List.map (fun r -> (r |> resultJavaListingToString) + (r.avr |> List.filter (fun avr -> avr.opt.IsSome) |> resultsAvrToString))
                |> List.fold (+) ""
        yield ""
        yield "--- LISTING: COMPLETE LISTING"
        yield results.jvmInstructions
                |> List.map (fun r -> (r |> resultJavaListingToString) + (r.avr |> resultsAvrToString))
                |> List.fold (+) ""
    } |> Seq.fold (fun acc x -> acc + "\r\n" + x) ""

let main(args : string[]) =
    let benchmark = (Array.get args 1)
    let builddir = (Array.get args 2)
    let outputfilename = (Array.get args 3)

    let dih = DarjeelingInfusionHeaderXml.Load(String.Format("{0}/infusion-bm_{1}/bm_{1}.dih", builddir, benchmark))
    let rtcdata = RtcdataXml.Load(String.Format("{0}/rtcdata.xml", builddir))
    let profilerdata = ProfilerdataXml.Load(String.Format("{0}/profilerdata.xml", builddir)).Instructions |> Seq.toList
    let profilerdataPerAddress = profilerdata |> List.map (fun x -> (Convert.ToInt32(x.Address.Trim(), 16), x))
    let countersForAddressAndInst address inst =
        match profilerdataPerAddress |> List.tryFind (fun (address2,inst) -> address = address2) with
        | None -> failwith (String.Format ("No profilerdata found for address {0}", address))
        | Some(_, profiledInstruction) ->
            {
                executions = profiledInstruction.Executions
                cycles = (profiledInstruction.Cycles+profiledInstruction.CyclesSubroutine)
                count = 1
                size = AVR.instructionSize inst
            }
    let stdoutlog = System.IO.File.ReadLines(String.Format("{0}/stdoutlog.txt", builddir)) |> Seq.toList
    let disasm = System.IO.File.ReadLines(String.Format("{0}/darjeeling.S", builddir)) |> Seq.toList
    let djdebuglines = System.IO.File.ReadLines(String.Format("{0}/infusion-bm_{1}/jlib_bm_{1}.debug", builddir, benchmark)) |> Seq.toList
    let results = processTrace benchmark dih rtcdata countersForAddressAndInst stdoutlog disasm djdebuglines

    let txtFilename = outputfilename + ".txt"
    let xmlFilename = outputfilename + ".xml"

    File.WriteAllText (txtFilename, (resultsToString results))
    Console.Error.WriteLine ("Wrote output to " + txtFilename)

    let xmlSerializer = FsPickler.CreateXmlSerializer(indent = true)
    File.WriteAllText (xmlFilename, (xmlSerializer.PickleToString results))
    Console.Error.WriteLine ("Wrote output to " + xmlFilename)
    1

main(fsi.CommandLineArgs)
// main([|
//         "sortO"
//         "/Users/niels/src/rtc/src/build/avrora"
//         "/Users/niels/src/rtc/src/config/avrora/tmpoutput"
//      |])



















