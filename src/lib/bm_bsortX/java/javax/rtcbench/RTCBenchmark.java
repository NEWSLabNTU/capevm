package javax.rtcbench;

import javax.rtc.RTC;

public class RTCBenchmark {
	private final static short NUMNUMBERS = 256;

    public static String name = "BUBBLESORT NOT OPTIMISED";
    public static native void test_native();
    public static boolean test_java() {
		int numbers[] = new int[NUMNUMBERS]; // Not including this in the timing since we can't do it in C

		// Fill the array
		for (int i=0; i<NUMNUMBERS; i++)
			numbers[i] = (short)(NUMNUMBERS - 1 - i);

		// Then sort it
		rtcbenchmark_measure_java_performance(numbers);

        for (int k=0; k<NUMNUMBERS; k++) {
            if (numbers[k] != k) {
                return false;
            }
        }

        return true;
	}

	public static void rtcbenchmark_measure_java_performance(int[] numbers) {
		for (short i=0; i<NUMNUMBERS; i++) {
			for (short j=0; j<NUMNUMBERS-i-1; j++) {
				if (numbers[j]>numbers[j+1]) {
					int temp = numbers[j];
					numbers[j] = numbers[j+1];
					numbers[j+1] = temp;
				}
			}
		}
	}
}
