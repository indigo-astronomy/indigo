#ifndef DSLR_MODEL_INFO_H
#define DSLR_MODEL_INFO_H

static struct dslr_model_info {
  const char	*name;
  int		 width, height;
  float		 pixel_size;
} dslr_model_info[] = {
  { "CANON EOS REBEL XTI", 3888, 2592, 5.7  },  // == Kiss Digital X (J) == 400D (E)
  { "CANON EOS REBEL XT", 3456, 2304, 6.4  },  // == Kiss Digital N (J) == 350D (E)
  { "CANON EOS REBEL XSI", 4272, 2848, 5.19  },  // == Kiss X2 (J) == 450D (E)
  { "CANON EOS REBEL XS", 3888, 2592, 5.7  },  // == Kiss F (J) == 1000D (E)
  { "CANON EOS REBEL T1I", 4752, 3168, 4.69  },  // == Kiss X3 (J) == 500D (E)
  { "CANON EOS REBEL T2I", 5184, 3456, 4.3  },  // == Kiss X4 (J) == 550D (E)
  { "CANON EOS REBEL T3I", 5184, 3456, 4.3  },  // == Kiss X5 (J) == 600D (E)
  { "CANON EOS REBEL T3", 4272, 2848, 5.19  },  // == Kiss X50 (J) == 1100D (E)
  { "CANON EOS REBEL T4I", 5184, 3456, 4.3  },  // == Kiss X6i (J) == 650D (E)
  { "CANON EOS REBEL T5I", 5184, 3456, 4.3  },  // == Kiss X7i (J) == 700D (E)
  { "CANON EOS REBEL T5", 5184, 3456, 4.3  },  // == Kiss X70 (J) == 1200D, Hi (E)
  { "CANON EOS REBEL T6I", 6000, 4000, 3.71  },  // == Kiss X8i (J) == 750D (E)
  { "CANON EOS REBEL T6S", 6000, 4000, 3.71  },  // == 8000D (J) == 760D (E)
  { "CANON EOS REBEL T6", 5184, 3456, 4.3  },  // == Kiss X80 (J) == 1300D (E)
  { "CANON EOS REBEL T7", 6000, 4000, 3.71  },  // == Kiss X90 (J) == 2000D/1500D (E)
  { "CANON EOS REBEL T7I", 6000, 4000, 3.71  },  // == Kiss X9i (J) == 800D (E)
  { "CANON EOS REBEL T100", 5184, 3456, 4.3  },  // == 4000D (E)
  { "CANON EOS REBEL SL1", 5184, 3456, 4.3  },  // == Kiss X7 (J) == 100D (E)
  { "CANON EOS REBEL SL2", 6000, 4000, 3.71  },  // == Kiss X9 (J) == 200D (E)
  { "CANON EOS KISS X2", 4272, 2848, 5.19  },  // Digital Rebel XSi (A) == 450D (E)
  { "CANON EOS KISS X3", 4752, 3168, 4.69  },  // == Rebel T1i (A) == 500D (E)
  { "CANON EOS KISS X4", 5184, 3456, 4.3  },  // Rebel T2i (A) == 550D (E)
  { "CANON EOS KISS X50", 4272, 2848, 5.19  },  // == Rebel T3 (A) == 1100D (E)
  { "CANON EOS KISS X5", 5184, 3456, 4.3  },  // == Rebel T3i (A) == 600D (E)
  { "CANON EOS KISS X6I", 5184, 3456, 4.3  },  // == Rebel T4i (A) == 650D (E)
  { "CANON EOS KISS X7I", 5184, 3456, 4.3  },  // == Rebel T5i (A) == 700D (E)
  { "CANON EOS KISS X70", 5184, 3456, 4.3  },  // == Rebel T5 (A) == 1200D, Hi (E)
  { "CANON EOS KISS X7", 5184, 3456, 4.3  },  // == Rebel SL1 (A) == 100D (E)
  { "CANON EOS KISS X8I", 6000, 4000, 3.71  },  // == Rebel T6i (A) == 750D (E)
  { "CANON EOS KISS X80", 5184, 3456, 4.3  },  // == Rebel T6 (A) == 1300D (E)
  { "CANON EOS KISS X9I", 6000, 4000, 3.71  },  // == Rebel T7i (A) == 800D (E)
	{ "CANON EOS KISS X90", 6000, 4000, 3.71 },  // Rebel T7 (A) == 2000D/1500D (E)
	{ "CANON EOS KISS X9", 6000, 4000, 3.71 },  // == Rebel SL2 (A) == 200D (E)
	{ "CANON EOS KISS X", 3888, 2592, 5.7 },  // == Digital Rebel XTi (A) == 400D (E)
	{ "CANON EOS KISS N", 3456, 2304, 6.4 },  // == Digital Rebel XT (A) == 350D (E)
  { "CANON EOS KISS F", 3888, 2592, 5.7  },  // == Digital Rebel XS (A) == 1000D (E)
  { "CANON EOS 1000D", 3888, 2592, 5.7  },  // == Kiss F (J) == Digital Rebel XS (A)
  { "CANON EOS 1100D", 4272, 2848, 5.19  },  // == Kiss X50 (J) == Rebel T3 (A)
  { "CANON EOS 1200D", 5184, 3456, 4.3  },  // == Kiss X70 (J) == Rebel T5 (A)
  { "CANON EOS 1300D", 5184, 3456, 4.3  },  // == Kiss X80 (J) == Rebel T6 (A)
  { "CANON EOS 2000D", 6000, 4000, 3.71  },  // == Kiss X90 (J) == Rebel T7 (A)
  { "CANON EOS 4000D", 5184, 3456, 4.3  },  // == Rebel T100 (A)
  { "CANON EOS 8000D", 6000, 4000, 3.71  },  // == Rebel T6s (A) == 760D (E)
  { "CANON EOS 100D", 5184, 3456, 4.3  },  // == Kiss X7 (J) == Rebel SL1 (A)
  { "CANON EOS 200D", 6000, 4000, 3.71  },  // == Kiss X9 (J) == Rebel SL2 (A)
  { "CANON EOS 350D", 3456, 2304, 6.4  },  // == Kiss Digital N (J) == Digital Rebel XT (A)
  { "CANON EOS 400D", 3888, 2592, 5.7  },  // == Kiss Digital X (J) == Digital Rebel XTi (A)
  { "CANON EOS 450D", 4272, 2848, 5.19  },  // == Kiss X2 (J) == Digital Rebel XSi (A)
  { "CANON EOS 500D", 4752, 3168, 4.69  },  // == Kiss X3 (J) == Rebel T1i (A)
  { "CANON EOS 550D", 5184, 3456, 4.3  },  // == Kiss X4 (J) == Rebel T2i (A)
  { "CANON EOS 600D", 5184, 3456, 4.3  },  // == Kiss X5 (J) == Rebel T3i (A)
  { "CANON EOS 650D", 5184, 3456, 4.3  },  // == Kiss X6i (J) == Rebel T4i (A)
  { "CANON EOS 700D", 5184, 3456, 4.3  },  // == Kiss X7i (J) == Rebel T5i (A)
  { "CANON EOS 750D", 6000, 4000, 3.71  },  // == Kiss X8i (J) == Rebel T6i (A)
  { "CANON EOS 760D", 6000, 4000, 3.71  },  // == 8000D (J) == Rebel T6s (A)
  { "CANON EOS 800D", 6000, 4000, 3.71  },  // == Kiss X9i (J) == Rebel T7i (A)
	// Canon common
  { "CANON EOS 20D", 3504, 2336, 6.4  },
  { "CANON EOS 20DA", 3504, 2336, 6.4  },
  { "CANON EOS 30D", 3504, 2336, 6.4  },
  { "CANON EOS 40D", 3888, 2592, 5.7  },
  { "CANON EOS 50D", 4752, 3168, 4.69  },
  { "CANON EOS 60D", 5184, 3456, 4.3  },
  { "CANON EOS 70D", 5472, 3648, 4.11  },
  { "CANON EOS 80D", 6000, 4000, 3.71  },
  { "CANON EOS 1DS MARK III", 5616, 3744, 6.41  },
  { "CANON EOS 1D MARK III", 3888, 2592, 5.7  },
  { "CANON EOS 1D MARK IV", 4896, 3264, 5.69  },
  { "CANON EOS 1D X MARK II", 5472, 3648, 6.54  },
  { "CANON EOS 1D X", 5184, 3456, 6.94  },
  { "CANON EOS 1D C", 5184, 3456, 6.94  },
  { "CANON EOS 5D MARK II", 5616, 3744, 6.41  },
  { "CANON EOS 5DS", 8688, 5792, 4.14  },
  { "CANON EOS 5D", 4368, 2912, 8.2  },
  { "CANON EOS 6D", 5472, 3648, 6.54  },
  { "CANON EOS 7D MARK II", 5472, 3648, 4.1  },
  { "CANON EOS 7D", 5184, 3456, 4.3  },
  { "NIKON D40", 3008, 2000, 7.8  },
  { "NIKON D50", 3008, 2000, 7.8  },
  { "NIKON D60", 3872, 2592, 6.0  },
  { "NIKON D70", 3008, 2000, 7.8  },
  { "NIKON D80", 3872, 2592, 6.0  },
  { "NIKON D90", 4288, 2848, 5.5  },
  { "NIKON D600", 6016, 4016, 6.0  },
  { "NIKON D610", 6016, 4016, 6.0  },
  { "NIKON D700", 4256, 2832, 8.5  },
  { "NIKON D750", 6016, 4016, 6.0  },
  { "NIKON D800", 7360, 4912, 4.9  },
  { "NIKON D810", 7360, 4912, 4.9  },
  { "NIKON D3000", 3872, 2592, 6.0  },
  { "NIKON D3100", 4608, 3602, 5.0  },
  { "NIKON D3200", 6016, 4000, 3.9  },
  { "NIKON D3300", 6000, 4000, 3.9  },
  { "NIKON D5000", 4288, 2848, 5.5  },
  { "NIKON D5100", 4928, 3264, 4.8  },
  { "NIKON D5200", 6000, 4000, 3.9  },
  { "NIKON D5300", 6000, 4000, 3.9  },
  { "NIKON D7000", 4928, 3264, 4.8  },
  { "NIKON D7100", 6000, 4000, 3.9  },
  { NULL, 0, 0, 0 }
};

#endif				/* DSLR_MODEL_INFO_H */
