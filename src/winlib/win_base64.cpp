#include <win_base64.h>
#include <win_err.h>

#if _MSC_VER >= 1910
#pragma warning(push)
#pragma warning(disable:5045)
#endif

unsigned char b64_chr[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

unsigned int b64_int(unsigned int ch)
{

    // ASCII to base64_int
    // 65-90  Upper Case  >>  0-25
    // 97-122 Lower Case  >>  26-51
    // 48-57  Numbers     >>  52-61
    // 43     Plus (+)    >>  62
    // 47     Slash (/)   >>  63
    // 61     Equal (=)   >>  64~
    if (ch == 43)
        return 62;
    if (ch == 47)
        return 63;
    if (ch == 61)
        return 64;
    if ((ch > 47) && (ch < 58))
        return ch + 4;
    if ((ch > 64) && (ch < 91))
        return ch - 'A';
    if ((ch > 96) && (ch < 123))
        return (ch - 'a') + 26;
    return 0;
}
int b64e_size(unsigned int in_size)
{

    // size equals 4*floor((1/3)*(in_size+2));
    int i, j = 0;
    for (i = 0; i < (int)in_size; i++) {
        if (i % 3 == 0)
            j += 1;
    }
    return (4 * j);
}

int b64d_size(unsigned int in_size)
{

    return (int)((3 * in_size) / 4);
}


/**
 * base64_encode - Base64 encode
 * @src: Data to be encoded
 * @len: Length of the data to be encoded
 * @out_len: Pointer to output length variable, or %NULL if not used
 * Returns: Allocated buffer of out_len bytes of encoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer. Returned buffer is
 * nul terminated to make it easier to use as a C string. The nul terminator is
 * not included in out_len.
 */
int encode_base64(unsigned char* pbuffer, int insize, char* pencbuf, int outsize)
{
    unsigned int i = 0, j = 0, k = 0, s[3];
    int ret;
    unsigned char* out = (unsigned char*)pencbuf;

    if (outsize < b64e_size((unsigned int)insize)) {
        ret = -ERROR_INSUFFICIENT_BUFFER;
        SETERRNO(ret);
        return ret;
    }

    for (i = 0; i < (unsigned int)insize; i++) {
        s[j++] = *(pbuffer + i);
        if (j == 3) {
            out[k + 0] = b64_chr[ (s[0] & 255) >> 2 ];
            out[k + 1] = b64_chr[ ((s[0] & 0x03) << 4) + ((s[1] & 0xF0) >> 4) ];
            out[k + 2] = b64_chr[ ((s[1] & 0x0F) << 2) + ((s[2] & 0xC0) >> 6) ];
            out[k + 3] = b64_chr[ s[2] & 0x3F ];
            j = 0; k += 4;
        }
    }

    if (j) {
        if (j == 1)
            s[1] = 0;
        out[k + 0] = b64_chr[ (s[0] & 255) >> 2 ];
        out[k + 1] = b64_chr[ ((s[0] & 0x03) << 4) + ((s[1] & 0xF0) >> 4) ];
        if (j == 2)
            out[k + 2] = b64_chr[ ((s[1] & 0x0F) << 2) ];
        else
            out[k + 2] = '=';
        out[k + 3] = '=';
        k += 4;
    }

    out[k] = '\0';

    return (int)k;
}


/**
 * base64_decode - Base64 decode
 * @src: Data to be decoded
 * @len: Length of the data to be decoded
 * @out_len: Pointer to output length variable
 * Returns: Allocated buffer of out_len bytes of decoded data,
 * or %NULL on failure
 *
 * Caller is responsible for freeing the returned buffer.
 */
int decode_base64(char* pencbuf, int insize, unsigned char* pbuffer, int outsize)
{
    unsigned int i = 0, j = 0, k = 0, s[4];
    int ret;
    unsigned char* out = pbuffer;
    if (outsize < b64d_size((unsigned int)insize)) {
        ret = -ERROR_INSUFFICIENT_BUFFER;
        SETERRNO(ret);
        return ret;
    }

    for (i = 0; i < (unsigned int)insize; i++) {
        s[j++] = b64_int((unsigned int)(*(pencbuf + i)));
        if (j == 4) {
            out[k + 0] = ((s[0] & 255) << 2) + ((s[1] & 0x30) >> 4);
            if (s[2] != 64) {
                out[k + 1] = ((s[1] & 0x0F) << 4) + ((s[2] & 0x3C) >> 2);
                if ((s[3] != 64)) {
                    out[k + 2] = (unsigned char)(((s[2] & 0x03) << 6) + (s[3])); k += 3;
                } else {
                    k += 2;
                }
            } else {
                k += 1;
            }
            j = 0;
        }
    }

    return (int)k;
}

int base64_splite_line(char* pencbuf, int inlen, int linelen, char**ppencline, int *poutsize)
{
    int retsize = 0;
    int retlen = 0;
    char* pretline = NULL;
    int ret;
    int outlen = 0;
    int i;

    if (pencbuf == NULL) {
        if (ppencline && *ppencline) {
            free(*ppencline);
            *ppencline = NULL;
        }

        if (poutsize) {
            *poutsize = 0;
        }
        return 0;
    }

    if (ppencline == NULL || poutsize == NULL || linelen == 0) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    if (inlen > linelen) {
        retlen = inlen + (inlen / linelen) + 2;
    } else {
        retlen = inlen + 2;
    }

    pretline = *ppencline;
    retsize = *poutsize;

    if (retsize < retlen || pretline == NULL) {
        if (retsize < retlen) {
            retsize = retlen;
        }
        DEBUG_INFO("retsize [%d]", retsize);
        pretline = (char*)malloc((size_t)retsize);
        if (pretline == NULL) {
            GETERRNO(ret);
            goto fail;
        }
    }

    memset(pretline, 0, (size_t)retsize);
    outlen = 0;
    for (i = 0; i < inlen; i++) {
    	pretline[outlen] = pencbuf[i];
    	outlen ++;
        if ((i % linelen) == (linelen - 1)) {
        	DEBUG_INFO("i [%d]", i);
            pretline[outlen] = '\n';
            outlen ++;
        }
        ASSERT_IF(outlen < retsize);
    }

    pretline[outlen] = '\0';

    if (*ppencline && *ppencline != pretline) {
        free(*ppencline);
    }
    *ppencline = pretline;
    *poutsize = retsize;


    return outlen;
fail:
    /*because pretline == NULL only this goto fail, can be here*/
    //if ( pretline != NULL) {
    //    free(pretline);
    //}
    pretline = NULL;
    SETERRNO(ret);
    return ret;
}

int base64_compact_line(char* pencbuf, int enclen, char** ppencnoline, int* poutsize)
{
    char* pretenc = NULL;
    int outlen = 0;
    int i;
    int retsize = 0;
    int ret;

    if (pencbuf == NULL) {
        if (ppencnoline && *ppencnoline) {
            free(*ppencnoline);
            *ppencnoline = NULL;
        }

        if (poutsize) {
            *poutsize = 0;
        }
        return 0;
    }

    if (ppencnoline == NULL || poutsize == NULL) {
        ret = -ERROR_INVALID_PARAMETER;
        SETERRNO(ret);
        return ret;
    }

    pretenc = *ppencnoline;
    retsize = *poutsize;
    if (pretenc == NULL || retsize < (enclen + 1))  {
        if (retsize < (enclen + 1)) {
            retsize = enclen + 1;
        }
        pretenc = (char*)malloc((size_t)retsize);
        if (pretenc == NULL) {
            GETERRNO(ret);
            goto fail;
        }
    }
    memset(pretenc , 0, (size_t)retsize);
    outlen = 0;
    for (i = 0; i < enclen; i++) {
        if (pencbuf[i] != '\r' && pencbuf[i] != '\n') {
            pretenc[outlen] = pencbuf[i];
            outlen ++;
        }
    }

    if (*ppencnoline && *ppencnoline != pretenc) {
        free(*ppencnoline);
    }

    *ppencnoline = pretenc;
    *poutsize = retsize;
    return outlen;
fail:
    SETERRNO(ret);
    return ret;
}

#if _MSC_VER >= 1910
#pragma warning(pop)
#endif