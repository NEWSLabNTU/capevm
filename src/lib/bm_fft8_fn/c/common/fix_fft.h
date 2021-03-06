#ifndef FIXFFT_H
#define FIXFFT_H


/*
  fix_fft() - perform forward/inverse fast Fourier transform.
  fr[n],fi[n] are real and imaginary arrays, both INPUT AND
  RESULT (in-place FFT), with 0 <= n < 2**m; set inverse to
  0 for forward transform (FFT), or 1 for iFFT.

  Results are from 0 to n/2, (fr[0];fi[0]) contains DC component.
*/
// Renamed to rtcbenchmark_measure_native_performance
// to measure performance in AOT benchmark scripts
// int fix_fft(char fr[], char fi[], int m, int inverse);
int rtcbenchmark_measure_native_performance(char fr[], char fi[], int m, int inverse);


/*
  fix_fftr() - forward/inverse FFT on array of real numbers.
  Real FFT/iFFT using half-size complex FFT by distributing
  even/odd samples into real/imaginary arrays respectively.
  In order to save data space (i.e. to avoid two arrays, one
  for real, one for imaginary samples), we proceed in the
  following two steps: a) samples are rearranged in the real
  array so that all even samples are in places 0-(N/2-1) and
  all imaginary samples in places (N/2)-(N-1), and b) fix_fft
  is called with fr and fi pointing to index 0 and index N/2
  respectively in the original array. The above guarantees
  that fix_fft "sees" consecutive real samples as alternating
  real and imaginary samples in the complex array.
*/
int fix_fftr(char f[], int m, int inverse);


/* 
	fft_windowing() - perform windowing on data to eliminate noise
	The windowing function is performed on the sample data, calculating a
	"fade in/fade out", thereby minimizing errors resulting from real-world data
	which is not the exact frequency loop a FFT would require.
	Works like a sort of lowpass filter.
	Called with the sample data array as a parameter. Real-world sample data normally
	does not contain imaginary components - if windowing for complex data is needed call it
	on the complex and imaginary part separately.

	Parameters: f is source and target array, m is number of bits (e.g. 0 <= n < 2**m)
*/
void fft_windowing(char f[], int m);


#endif 





