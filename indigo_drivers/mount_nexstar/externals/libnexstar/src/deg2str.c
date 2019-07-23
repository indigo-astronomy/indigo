/**************************************************************
	ASTRONOMICAL ROUTINE LIBRARY
	
	ASCII to double and double to ASCII 
	conversion routines.
	
	(C)2000 by Rumen G.Bogdanovski
***************************************************************/
#include<stdlib.h>
#include<stdio.h>
#include<math.h>
#include<string.h>
#include<ctype.h>

#include "deg2str.h"

int a2dd(double *dd, const char *a)
{  
	int i;
	double 	deg,min,sec,sign=1;
	char *buff,*b1,format[30], *pnt;
	
	buff=(char*)a;                     //clear the spaces
	while (isspace(buff[0])) buff++;
	i=strlen(buff)-1;
	while (isspace(buff[i])) i--;
	buff[i+1]='\0';
	
	if (buff[0]=='-') { sign=-1; buff++; }
	if (buff[0]=='+') buff++;
	
	if ((buff=(char*)strtok_r(buff,":", &pnt))==NULL) return Err_INVSTR;
	deg=(double)strtoul(buff,&b1,10);
	if((buff[0]=='\0')||(b1[0]!='\0')) return Err_INVSTR;
	
 	if ((buff=(char*)strtok_r(NULL,":", &pnt))==NULL) return Err_INVSTR;
	min=(double)strtoul(buff,&b1,10);
	if((buff[0]=='\0')||(b1[0]!='\0')) return Err_INVSTR;
	
 	if ((buff=(char*)strtok_r(NULL,"\0", &pnt))==NULL) return Err_INVSTR;
	sec=(double)strtod(buff,&b1);
	if((buff[0]=='\0')||(b1[0]!='\0')) return Err_INVSTR;
	
	if((min>=60)||(min<0)||(sec>=60)||(sec<0)) return Err_OUTOFR;
	
	*dd=sign*(deg+min/60+sec/3600);
	
	return Err_NOERR;
}


char *dd2a(double a, int plus)
{
	int sign=1;
	int min,deg;
	double sec;
	static char str[30], *fc;
	
	if(a<0) { a*=-1; sign=-1; }
	
#ifdef NUNDRETHS	
	a*=360000;
	a=rint(a);
	a/=360000;
#endif 
#ifdef TENTHS	
	a*=36000;
	a=rint(a);
	a/=36000;
#endif 
#ifdef SECONDS	
	a*=3600;
	a=rint(a);
	a/=3600;
#endif 
	
	deg=(int)a;
	sec=(a-(int)(a))*3600;
	min=sec/60;
	sec=fabs(sec-min*60);


#ifdef HUNDRETHS
	if (sign < 0) sprintf(str,"-%d:%02d:%05.2f",deg,min,sec);
	else if (plus>0) 
		sprintf(str,"+%d:%02d:%05.2f",deg,min,sec);
		else sprintf(str,"%d:%02d:%05.2f",deg,min,sec);
#endif
#ifdef TENTHS	
	if (sign < 0) sprintf(str,"-%d:%02d:%04.1f",deg,min,sec);
	else if (plus>0) 
		sprintf(str,"+%d:%02d:%04.1f",deg,min,sec);
		else sprintf(str,"%d:%02d:%04.1f",deg,min,sec);
#endif
#ifdef SECONDS	
	if (sign < 0) sprintf(str,"-%d:%02d:%02d",deg,min,(int)sec);
	else if (plus>0) 
		sprintf(str,"+%d:%02d:%02d",deg,min,(int)sec);
		else sprintf(str,"%d:%02d:%02d",deg,min,(int)sec);
#endif
	if ((fc = strchr(str, ',')))
		*fc = '.';	
	return str;
}

int a2dh(double *dh, const char *a)
{  
	int i;
	double 	hour,min,sec;
	char *buff,*b1, *pnt;
	
	buff=(char*)a;                     //clear the spaces
	while (isspace(buff[0])) buff++;
	i=strlen(buff)-1;
	while (isspace(buff[i])) i--;
	buff[i+1]='\0';
	
	if ((buff=(char*)strtok_r(buff,":", &pnt))==NULL) return Err_INVSTR;
	hour=(double)strtoul(buff,&b1,10);
	if((buff[0]=='\0')||(b1[0]!='\0')) return Err_INVSTR;
	
 	if ((buff=(char*)strtok_r(NULL,":", &pnt))==NULL) return Err_INVSTR;
	min=(double)strtoul(buff,&b1,10);
	if((buff[0]=='\0')||(b1[0]!='\0')) return Err_INVSTR;
	
 	if ((buff=(char*)strtok_r(NULL,"\0", &pnt))==NULL) return Err_INVSTR;
	sec=(double)strtod(buff,&b1);
	if((buff[0]=='\0')||(b1[0]!='\0')) return Err_INVSTR;
	
	if((hour<0)||(hour>=24)||(min>=60)||(min<0)||(sec>=60)||(sec<0)) 
	   return Err_OUTOFR;
	
	*dh=hour+min/60+sec/3600;
	
	return Err_NOERR;
}

char *dh2a(double h)
{
	int min,hour;
	double sec;
	static char str[30];
	
	if((h<0)||(h>=24)) return NULL;
	
#ifdef NUNDRETHS	
	h*=360000;
	h=rint(h);
	h/=360000;
#endif 
#ifdef TENTHS	
	h*=36000;
	h=rint(h);
	h/=36000;
#endif 
#ifdef SECONDS	
	h*=3600;
	h=rint(h);
	h/=3600;
#endif	
	hour=(int)h;
	sec=(h-(int)(h))*3600;
	min=sec/60;
	sec=sec-min*60;
	
#ifdef HUNDRETHS	
	sprintf(str,"%02d:%02d:%05.2f",hour,min,sec); 
#endif

#ifdef TENTHS	
	sprintf(str,"%02d:%02d:%04.1f",hour,min,sec); 
#endif
#ifdef SECONDS	
	sprintf(str,"%02d:%02d:%02d",hour,min,(int)sec); 
#endif

	return str;
}

void dd2dms(double ang, unsigned char *deg, unsigned char *min, unsigned char *sec, char *sign) {
	if (ang >= 0) {
		int a = (int)(ang * 3600 + 0.5);
		*deg = (unsigned char)(a / 3600);
		int f = (int)(a % 3600);
		*min = (unsigned char)(f / 60);
		*sec = (unsigned char)(f % 60);
		*sign = 0;
		return;
	} else {
		ang *= -1;
		int a = (int)(ang * 3600 + 0.5);
		*deg = (unsigned char)(a / 3600);
		int f = (int)(a % 3600);
		*min = (unsigned char)(f / 60);
		*sec = (unsigned char)(f % 60);
		*sign = 1;
		return;
	}
}
