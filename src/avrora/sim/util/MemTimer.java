/**
 * Copyright (c) 2004-2005, Regents of the University of California
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * Neither the name of the University of California, Los Angeles nor the
 * names of its contributors may be used to endorse or promote products
 * derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

package avrora.sim.util;

import avrora.sim.*;
import cck.text.Terminal;


/**
 * <code>MemTimer</code> lets simulated applications start and stop
 * timers
 *
 * @author John Regehr
 */
public class MemTimer extends Simulator.Watch.Empty {

    int base;
    long start_time = 0;
    int timer_state = 0;
    int timer_number = 1;

    public MemTimer(int b) {
        base = b;
    }

    public void fireBeforeWrite(State state, int data_addr, byte value) {
        if (data_addr != base) {
            Terminal.printRed("Unexpected interception by printer!");
            System.exit(-1);
        }

        Simulator sim = state.getSimulator();
        StringBuffer buf = new StringBuffer();
        SimUtil.getIDTimeString(buf, sim);

        if (value == 0) {
            return;
        } else if (value == -1) {
            if (timer_state != 0) {
                Terminal.printRed("[avrora.c-timer] multiple starts in a row??\nPROBABLY BECAUSE OF A CRASH: ABORTING\n");
                System.exit(-1);
                // buf.append("[avrora.c-timer] multiple starts in a row??");
            } else {
                start_time = state.getCycles();
                buf.append("[avrora.c-timer] start");
            }
            timer_state = 1;
        } else if (value == -2) {
            if (timer_state != 1) {
                buf.append("[avrora.c-timer] multiple stops in a row??");
            } else {
                long stop_time = state.getCycles();
                long duration = stop_time - start_time;
                buf.append("[avrora.c-timer] timer number " + String.valueOf(timer_number) + ": " + String.valueOf(duration) + " cycles");
            }
            timer_state = 0;
        } else if (value > 0) {
            this.timer_number = value;
        }
        Terminal.printRed(buf.toString());
        Terminal.nextln();
    }
}
