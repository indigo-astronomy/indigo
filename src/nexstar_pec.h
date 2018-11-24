/**************************************************************
	Celestron NexStar compatible telescope control library
	
	(C)2013-2016 by Rumen G.Bogdanovski
***************************************************************/
#if !defined(__NEXSTAR_PEC_H)
#define __NEXSTAR_PEC_H

#define PEC_START 1
#define PEC_STOP 0

#ifdef __cplusplus /* If this is a C++ compiler, use C linkage */
extern "C" {
#endif

int pec_index_found(int dev);
int pec_seek_index(int dev);

int pec_record(int dev, char action);
int pec_record_complete(int dev);

int pec_playback(int dev, char action);
int pec_get_playback_index(int dev);

int pec_get_data_len(int dev);

int _pec_set_data(int dev, unsigned char index, char data);
int _pec_get_data(int dev, unsigned char index);

int pec_set_data(int dev, float *data, int len);
int pec_get_data(int dev, float *data, const int max_len);

#ifdef __cplusplus /* If this is a C++ compiler, end C linkage */
}
#endif

#endif /* __NEXSTAR_PEC_H */
