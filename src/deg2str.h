/**************************************************************
	ASTRONOMICAL ROUTINE LIBRARY
	
	ASCII to double and double to ASCII 
	conversion routines prototypes.
	
	(C)2000 by Rumen G.Bogdanovski
***************************************************************/
#if !defined(__DEG2STR_H)
#define __DEG2STR_H

#define Err_NOERR  0
#define Err_INVSTR 1
#define Err_OUTOFR 2

//uncoment one of those to get needed hour and angle formats  
//#define HUNDRETHS				// XX:XX:XX.XX
#define TENTHS					// XX:XX:XX.X
//#define SECONDS				// XX:XX:XX

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

int a2dd(double *dd, const char *a);
char *dd2a(double a, int plus);

int a2dh(double *dh, const char *a);
char *dh2a(double h);

void dd2dms(double ang, unsigned char *deg, unsigned char *min, unsigned char *sec, char *sign);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif  /* __DEG2STR_H */
