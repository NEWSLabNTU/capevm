package javax.rtcbench;

import javax.darjeeling.Stopwatch;

public class RTCBenchmark {
    public static String name = "COREMARK";
    public static native void test_native();
    public static boolean test_java() {
        rtcbenchmark_measure_java_performance();
        return true;
    }

    public static void rtcbenchmark_measure_java_performance() {
        Stopwatch.resetAndStart();

        CoreMain.core_mark_main();

        Stopwatch.measure();
    }

    public static void nothing() {}
}
