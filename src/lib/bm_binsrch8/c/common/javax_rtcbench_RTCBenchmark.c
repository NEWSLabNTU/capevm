#include <stdint.h>
#include "config.h"
#include "heap.h"

// Split into separate function to avoid the compiler just optimising away the whole test.

// Based on binary search benchmark in TakaTuka.
// http://sourceforge.net/p/takatuka/code/HEAD/tree/
// Should be the same code that Joshua Ellul used for his thesis.
// Modified slightly to do the same test Joshua did:
// "The binary search test performs 1,000 binary searches for the worst case search in 100 16 bit values." on p75 of his thesis

void __attribute__((noinline)) rtcbenchmark_measure_native_performance(uint16_t NUMNUMBERS, int8_t numbers[]);

void javax_rtcbench_RTCBenchmark_void_test_native() {
    uint16_t NUMNUMBERS = 100;
    int8_t *numbers = dj_mem_checked_alloc(NUMNUMBERS*sizeof(int8_t), CHUNKID_RTCNATIVETESTDATA);

    for (uint8_t loop = 0; loop < NUMNUMBERS; loop++) {
        numbers[loop] = (loop - 30);
    }

    rtcbenchmark_measure_native_performance(NUMNUMBERS, numbers);
}

// Faisal Aslam's original code below:


//  * Copyright 2010 Christian Schindelhauer, Peter Thiemann, Faisal Aslam, Luminous Fennell and Gidon Ernst.
//  * All Rights Reserved.
//  * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
//  * 
//  * This code is free software; you can redistribute it and/or modify
//  * it under the terms of the GNU General Public License version 3
//  * only, as published by the Free Software Foundation.
//  * 
//  * This code is distributed in the hope that it will be useful, but
//  * WITHOUT ANY WARRANTY; without even the implied warranty of
//  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
//  * General Public License version 3 for more details (a copy is
//  * included in the LICENSE file that accompanied this code).
//  * 
//  * You should have received a copy of the GNU General Public License
//  * version 3 along with this work; if not, write to the Free Software
//  * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
//  * 02110-1301 USA
//  * 
//  * Please contact Faisal Aslam 
//  * (aslam AT informatik.uni-freibug.de or studentresearcher AT gmail.com)
//  * if you need additional information or have any questions.
 
// package binarySearch;

// public class Main {

//     private static final byte NOT_FOUND = -1;
//     private static short length = 100;
//     private static short test[] = new short[length];

//     public static void main(String[] args) {
//     System.out.println("started");
//         for (int loop = 0; loop < length; loop++) {
//         System.out.println(loop);
//             test[loop] = (short) (loop - 30);
//         }
//         /*for (int loop = 0; loop < 5; loop++) {
//             binarySearch(test, length-50);
//         }*/
//         System.out.println(binarySearch(test, length-50));
//         System.out.println(System.currentTimeMillis());
//     }

//     public static int binarySearch(short[] list, int toFind) {
//         int low = 0;
//         int high = list.length - 1;
//         int mid;
//         while (low <= high) {
//             mid = (low + high) / 2;
//             if (list[mid] < toFind) {
//                 low = mid + 1;
//             } else if (list[mid] > toFind) {
//                 high = mid - 1;
//             } else {
//                 return mid;
//             }
//         }
//         return NOT_FOUND;     // NOT_FOUND = -1
//     }
// }


