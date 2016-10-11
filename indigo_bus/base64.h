#ifndef __BASE64_H
#define __BASE64_H

#ifdef __cplusplus
extern "C" {
#endif

extern int base64_encode(unsigned char *out, const unsigned char *in, int inlen);
extern int base64_decode(char *out, const char *in);
extern int base64_decode_fast(char *out, const char *in, int inlen);

#ifdef __cplusplus
}
#endif

#endif
