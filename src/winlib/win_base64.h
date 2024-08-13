#ifndef __WIN_BASE64_H_D96624A48FC1E891D941C47A81BE5355__
#define __WIN_BASE64_H_D96624A48FC1E891D941C47A81BE5355__

#undef __WINLIB_INNER_INCLUDE__
#define __WINLIB_INNER_INCLUDE__
#include <win_inner.h>
#undef __WINLIB_INNER_INCLUDE__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus*/

WINLIB_API int encode_base64(unsigned char* pbuffer,int insize, char* pencbuf,int outsize);
WINLIB_API int decode_base64(char* pencbuf, int insize,unsigned char* pbuffer, int outsize);
WINLIB_API int base64_splite_line(char* pencbuf,int inlen,int linelen,char**ppencline,int *poutsize);
WINLIB_API int base64_compact_line(char* pencbuf,int inlen,char** ppencnoline, int* poutsize);

#ifdef __cplusplus
};
#endif /* __cplusplus*/

#endif /* __WIN_BASE64_H_D96624A48FC1E891D941C47A81BE5355__ */
