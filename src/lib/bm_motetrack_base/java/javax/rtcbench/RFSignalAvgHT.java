package javax.rtcbench;

import javax.rtc.RTC;
import javax.rtc.Lightweight;

public class RFSignalAvgHT {
    public RFSignalAvg[] htData;
    public short size;
    public short capacity;

    public RFSignalAvgHT(short hashCapacity) {
        this.htData = new RFSignalAvg[hashCapacity];
        this.capacity = hashCapacity;
        for (short i=0; i<hashCapacity; i++) {
            this.htData[i] = new RFSignalAvg();
        }        
        init(this);
    }
    @Lightweight
    public static void init(RFSignalAvgHT rfSigPtr) {
        rfSigPtr.size = 0;

        short capacity = rfSigPtr.capacity;
        for (short i=0; i<capacity; i++) {
            RFSignalAvg.init(rfSigPtr.htData[i]);
        }        
    }

    /**
     * Tries to add the content of a new beacon message to this RFSignalAvgHT.  If no cell entry exists
     * for this sourceID, then it creates one.  Otherwise, it just adds the newRSSI to the cell with
     * newSourceID in the f frequency channel index and p power index.  If the hashtable is full, 
     * then it drops the new beacon message.
     * <b>IMPORTANT:</b> If <code>sourceID == 0</code>, then the cell is considered empty 
     * (i.e. no beaconMote may have an ID of 0)!
     * @param HPtr  the current RFSignalAvgHT struct
     * @param newSrcID  the sourceID corresponding to <code>newRssi</code>
     * @param f  the frequency channel index for this beacon message
     * @param p  the transmission power index for this beacon message
     * @param newRssi  the RSSI value
     * @return <code>SUCCESS</code>, if it was added; <code>FAIL</code> if it couldn't be added
     */
    @Lightweight
    public static boolean put(RFSignalAvgHT HPtr, short newSrcID, byte f, short newRssi)
    {
        // very simple hash function, but works well because most sourceID's are below HPtr.capacity
        short hashID = (short)(((short)(newSrcID*13)) % HPtr.capacity);
        short i = hashID;

        do {
            // add rfSignal if the current cell has no entry (i.e. nbrSamples == 0)
            // or the current cell entry is for the same sourceID
            if (HPtr.htData[i].sourceID == 0) {
                HPtr.htData[i].sourceID = newSrcID;  // this only matters when nbrSamples[i] == 0!
                HPtr.size++;
            }
            if (HPtr.htData[i].sourceID == newSrcID) {
                if (f == 0) {
                    HPtr.htData[i].rssiSum_0 += newRssi;   // DANGER! if rssiSum is uint8_t there may be an overflow problem!
                    HPtr.htData[i].nbrSamples_0++;
                } else {
                    HPtr.htData[i].rssiSum_1 += newRssi;   // DANGER! if rssiSum is uint8_t there may be an overflow problem!
                    HPtr.htData[i].nbrSamples_1++;
                }
                return true;                                                 
            }
            i = (short)(((short)(i + 1)) % HPtr.capacity);
        } while (i != hashID);

        // If we didn't find a cell to place our entry, then is means the hastable 
        // is full (i.e. capacity == size), otherwise somethings has gone wrong
        if (HPtr.size == HPtr.capacity) {
            // printfUART("RFSignalAvgHT - put(): WARNING! hash table is full!  Can't add new entry\n", "");
            // return FAIL;
            RTC.avroraPrintHex32(0xBEEF0004);
            RTC.avroraBreak();
        }
        else {
            // printfUART("RFSignalAvgHT - put(): FATAL ERROR! hash table is inconsistent!  Dumping hash table and exiting program ...\n", "");
            // printfUART("              - trying to add: <srcID=%i, RSSI=%i, txPowerIndex=%i>\n", newSrcID, newRssi, p);
            // RFSignalAvgHT_print(HPtr);        
            // EXIT_PROGRAM = 1;
            // return FAIL;
            RTC.avroraPrintHex32(0xBEEF0005);
            RTC.avroraBreak();
        }
        return false;
    }


    /** 
     * Constructs a Signature from the Hashtable HPtr.  The Signature will contain the 
     * RFSignals with the strongest RSSIs (from HPtr) and sorted by sourceID.  All decisions
     * regarding the strongest RSSIs is based on freqChan index 0 and txPower index 0! <br>
     * <b>WARNING:</b>  After calling this  function, HPtr  becomes inconsistent because some 
     * of its data gets overwritten; therefore it should no longer be used.
     *
     * @param retSigPtr  return the constructed signature through this pointer
     * @param retSrcIDMaxRSSIPtr  return the sourceID of RFSignal with max RSSI through this pointer
     * @param HPtr  the RFSignalAvgHT structure.
     */
    // Changed retSrcIDMaxRSSIPtr from a pointer parameter to the return value.
    static short makeSignature(Signature retSigPtr, RFSignalAvgHT HPtr)
    {
        short i = 0, k = 0, f = 0; //, p = 0;
        short maxRssiIndex = 0;
        short retSrcIDMaxRSSIPtr;

        // (1) - If the hashtable is empty, then we can't construct a Signature
        if (HPtr.size == 0) {
            // *retSrcIDMaxRSSIPtr = 0;
            retSrcIDMaxRSSIPtr = 0;      // the hash table is empty, so can't construct Signature    
        } else if (HPtr.size <= Signature.NBR_RFSIGNALS_IN_SIGNATURE) {
        // (2) - If we can include all RFSignals, there is no need to first sort the hashtable by strongest RSSI
            // (a) Initialize the signature
            //Signature_init(retSigPtr);
            k = -1;
            maxRssiIndex = 0;

            // (b) Construct the Signature
            for (i = 0; i < HPtr.capacity; ++i) {
                if (HPtr.htData[i].sourceID > 0) {  // ASSUMING sourceID=0 means the cell is empty!
                    // we have a cell entry, add it to the Signature
                    retSigPtr.rfSignals[++k].sourceID = HPtr.htData[i].sourceID;

                    // for (f = 0; f < MoteTrackParams.NBR_FREQCHANNELS; ++f) {
                        if (HPtr.htData[i].nbrSamples_0 > 0) {
                            // WARNING! integer division, may lose some precision!
                            retSigPtr.rfSignals[k].rssi_0 = (byte)(HPtr.htData[i].rssiSum_0 / HPtr.htData[i].nbrSamples_0);
                        }
                        else
                            retSigPtr.rfSignals[k].rssi_0 = 0;

                        if (HPtr.htData[i].nbrSamples_1 > 0) {
                            // WARNING! integer division, may lose some precision!
                            retSigPtr.rfSignals[k].rssi_1 = (byte)(HPtr.htData[i].rssiSum_1 / HPtr.htData[i].nbrSamples_1);
                        }
                        else
                            retSigPtr.rfSignals[k].rssi_1 = 0;
                    // }
                    
                    // base decision on the 1st freqChan and strongest TXPower (assuming it is index 0)!!!
                    if (retSigPtr.rfSignals[k].rssi_0 > retSigPtr.rfSignals[maxRssiIndex].rssi_0)
                        maxRssiIndex = k;                       
                }    
            }
            
            retSrcIDMaxRSSIPtr = retSigPtr.rfSignals[maxRssiIndex].sourceID;
            // *retSrcIDMaxRSSIPtr = retSigPtr.rfSignals[maxRssiIndex].sourceID;

            // (c) Sort the RFSignals in Signature by sourceID

            //printfUART("\nBefore sort:\n", "");
            //for (i = 0; i < HPtr.size; ++i)
            //    printfUART("%i  ", retSigPtr.rfSignals[i].sourceID);

            // KLDEBUG - should use qsort.  However MSP430 libc doesn't define qsort!!!!
            // #if defined(PLATFORM_TELOS)
                RFSignal.sortSrcID(retSigPtr.rfSignals, HPtr.size);
            // #else
            //    qsort(retSigPtr.rfSignals, HPtr.size, sizeof(RFSignal), RFSignal_compare);
            // #endif

            //printfUART("\nAfter sort:\n", "");
            //for (i = 0; i < HPtr.size; ++i)
            //    printfUART("%i  ", retSigPtr.rfSignals[i].sourceID);

        }

        // (3) - We can't include all RFSignals, so sort them by strongest RSSIs 
        //       and include only the strongest ones
        else {
            // (a) Initialize the signature
            Signature.init(retSigPtr);


            // (b) IMPORTANT Compute the rssiAvg !!!  We store the results in HPtr, starting
            //     from index 0 and going to index HPtr.size.  Note, we are destroying the HPtr!
            k = 0;
            for (i = 0; i < HPtr.capacity; ++i) {
                if (HPtr.htData[i].sourceID > 0) {  // ASSUMING sourceID=0 means the cell is empty!    
                
                    HPtr.htData[k].sourceID = HPtr.htData[i].sourceID;

                    // for (f = 0; f < MoteTrackParams.NBR_FREQCHANNELS; ++f) {
                        if (HPtr.htData[i].nbrSamples_0 > 0) {
                            // WARNING 1: placeing result in the k-th cell of HPtr, overwriting what was there!
                            // WARNING 2: integer division, may lose some precision!
                            HPtr.htData[k].rssiSum_0 = (short)(HPtr.htData[i].rssiSum_0 / HPtr.htData[i].nbrSamples_0);
                            HPtr.htData[k].nbrSamples_0 = 1;  // statement not necessary, but keeps datastructure consistent
                        }
                        else
                            HPtr.htData[k].rssiSum_0 = 0;

                        if (HPtr.htData[i].nbrSamples_1 > 0) {
                            // WARNING 1: placeing result in the k-th cell of HPtr, overwriting what was there!
                            // WARNING 2: integer division, may lose some precision!
                            HPtr.htData[k].rssiSum_1 = (short)(HPtr.htData[i].rssiSum_1 / HPtr.htData[i].nbrSamples_1);
                            HPtr.htData[k].nbrSamples_1 = 1;  // statement not necessary, but keeps datastructure consistent
                        }
                        else
                            HPtr.htData[k].rssiSum_1 = 0;
                    // }
                    k++;
                }               
            }


            // (c) Sort by RSSI so that large RSSIs are at the beginning of the array
            // KLDEBUG -- temporarily commented the line below because MSP430 doesn't define qsort, and 
            // otherwise it does not compile.  SHOULD NOT BE COMMENTED!!!!!!!!!!!!!!!!!!!!!!!
            // qsort(HPtr.htData, HPtr.size, sizeof(RFSignalAvg), RFSignalAvg_revCompareRSSI);

            // at this point the strongest RSSI is at 1st freqChan, index 0
            // *retSrcIDMaxRSSIPtr = HPtr.htData[0].sourceID;
            retSrcIDMaxRSSIPtr = HPtr.htData[0].sourceID;
            
                                                              
            // (d) IMPORTANT: construct the Signature with the strongest RSSIs  
            //     based on the 1st freqChan and strongest TXPower (assuming it is index 0)!!!
            // we need to copy the RFSignals in reverse order because 
            // the larger RSSIs are at the end of the array
            for (i = 0; i < Signature.NBR_RFSIGNALS_IN_SIGNATURE && i < HPtr.size; ++i) {
             
                retSigPtr.rfSignals[i].sourceID = HPtr.htData[i].sourceID;
                // for (f = 0; f < MoteTrackParams.NBR_FREQCHANNELS; ++f)
                    retSigPtr.rfSignals[i].rssi_0 = (byte)(HPtr.htData[i].rssiSum_0);
                    retSigPtr.rfSignals[i].rssi_1 = (byte)(HPtr.htData[i].rssiSum_1);
            }


            // (e) Sort the RFSignals in Signature by sourceID
            // KLDEBUG - should use qsort.  However MSP430 libc doesn't define qsort!!!!
            // #if defined(PLATFORM_TELOS)
                RFSignal.sortSrcID(retSigPtr.rfSignals, HPtr.size);
            // #else  // PLATFORM_MICAZ || PLATFORM_MICA2 || PLATFORM_MICA2DOT
            //    qsort(retSigPtr.rfSignals, HPtr.size, sizeof(RFSignal), RFSignal_compare);
            // #endif

        }
        return retSrcIDMaxRSSIPtr;
    }
}
