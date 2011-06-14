#include "strings.h"
#include <wchar.h>
#include <ctype.h>

iconv_t to_wchar = iconv_open("WCHAR_T", LOCALE_CHARSET);
iconv_t to_utf8  = iconv_open(LOCALE_CHARSET,   "WCHAR_T");


char     buff_wchar[CONVERT_BUFFER_SIZE];
char     buff_utf8[CONVERT_BUFFER_SIZE];
char     lower_buffer[CONVERT_BUFFER_SIZE];

char *utf8_lowercase(char *str)
{

    // Force locale to be LOCALE_CHARSET
    setlocale(LC_CTYPE, "ru_RU."LOCALE_CHARSET);

    int i;
    size_t res;

    char  *ret      = NULL;
    char  *str_ptr  = str;
    size_t str_len  = strlen(str);

    if (str_len >= CONVERT_BUFFER_SIZE){
        LOG(L_ERROR, "str_len must be < %d at %s:%d (%s)\n",
            CONVERT_BUFFER_SIZE,
            __FILE__, __LINE__, __FUNCTION__
        );
        return NULL;
    }

    char    *buff_wchar_ptr     = buff_wchar;
    size_t   buff_wchar_max_len = CONVERT_BUFFER_SIZE;

    char    *buff_utf8_ptr      = buff_utf8;
    size_t   buff_utf8_max_len  = CONVERT_BUFFER_SIZE;

    wchar_t *result_ptr = (wchar_t *)buff_wchar;
    size_t wchar_len;
    size_t real_wchars;
    // clear buffers
    memset(buff_wchar, 0x00, CONVERT_BUFFER_SIZE);
    memset(buff_utf8,  0x00, CONVERT_BUFFER_SIZE);

    // convert to wide symbols
    res = iconv(to_wchar,
        &str_ptr, &str_len,
        &buff_wchar_ptr, &buff_wchar_max_len
    );
    if (res == (size_t) -1){
        //ERROR_LOG(L_ERROR, "iconv() failed\n");
        //ERROR_LOG(L_ERROR, "at %s:%d (%s)\n", __FILE__, __LINE__, __FUNCTION__);
        return NULL;
    }

    // convert wide symbols to lower case
    wchar_len = (CONVERT_BUFFER_SIZE - buff_wchar_max_len);
    real_wchars = wchar_len/sizeof(wchar_t);
    for (i = 0; i < real_wchars; i++){
        result_ptr[i] = towlower(result_ptr[i]);
    }

    // convert back to utf8
    buff_wchar_ptr = buff_wchar;

    res = iconv(to_utf8,
        &buff_wchar_ptr, &wchar_len,
        &buff_utf8_ptr,  &buff_utf8_max_len
    );
    if (res == (size_t) -1){
        LOG(L_ERROR, "Cannot convert to lowercase: '%s'\n", str);
        LOG(L_ERROR, "iconv() failed");
        LOG(L_ERROR, "at %s:%d (%s)\n", __FILE__, __LINE__, __FUNCTION__);
        buff_utf8[0] = 0x00;
        return (char *)buff_utf8;
    }

    buff_utf8[CONVERT_BUFFER_SIZE - buff_utf8_max_len] = 0x00;
    return (char *)buff_utf8;
}

void str_replace(char *str, char needle, char replace){
    while(*str != '\0'){
        if(*str == needle)
            *str = replace;
        str++;
    }
}

void str_rtrim(char *str){
    size_t len = strlen(str);
    while(len > 0){
        len--; //len = index now
        if( str[len] == ' ' || 
            str[len] == '\t' ||
            str[len] == '\n' ||
            str[len] == '\r' ||
            str[len] == '\x0B'
        ){
            str[len] = '\0';
        } else {
            break;
        }
    }
}

size_t remove_ending_newline(char *str){
    size_t len = strlen(str);
    if( len > 0 && str[len - 1] == '\n'){
        str[len - 1] = '\0';
        len--;
    }
    return len;
}

char *utf8_lowercase_with_replace(char *str)
{

    // Force locale to be LOCALE_CHARSET
    setlocale(LC_CTYPE, "ru_RU."LOCALE_CHARSET);

    int i;
    size_t res;

    char  *ret      = NULL;
    char  *str_ptr  = str;
    size_t str_len  = strlen(str);

    if (str_len >= CONVERT_BUFFER_SIZE){
        LOG(L_ERROR, "str_len must be < %d at %s:%d (%s)\n",
            CONVERT_BUFFER_SIZE,
            __FILE__, __LINE__, __FUNCTION__
        );
        return NULL;
    }

    char    *buff_wchar_ptr     = buff_wchar;
    size_t   buff_wchar_max_len = CONVERT_BUFFER_SIZE;

    char    *buff_utf8_ptr      = buff_utf8;
    size_t   buff_utf8_max_len  = CONVERT_BUFFER_SIZE;

    wchar_t *result_ptr = (wchar_t *)buff_wchar;
    size_t wchar_len;
    size_t real_wchars = 0;

    // clear buffers
    memset(buff_wchar, 0x00, CONVERT_BUFFER_SIZE);
    memset(buff_utf8,  0x00, CONVERT_BUFFER_SIZE);

    // convert to wide symbols
    res = iconv(to_wchar,
        &str_ptr, &str_len,
        &buff_wchar_ptr, &buff_wchar_max_len
    );
    if (res == (size_t) -1){
        LOG(L_ERROR, "iconv() failed");
        LOG(L_ERROR, "at %s:%d (%s)\n", __FILE__, __LINE__, __FUNCTION__);
        return NULL;
    }

    // convert wide symbols to lower case
    wchar_len = (CONVERT_BUFFER_SIZE - buff_wchar_max_len);
    real_wchars = wchar_len/sizeof(wchar_t);
    for (i = 0; i < real_wchars; i++){
        if(iswalpha(result_ptr[i]) || iswdigit(result_ptr[i])) {
            result_ptr[i] = towlower(result_ptr[i]);
        }
        else {
            result_ptr[i] = L' ';
        }
    }

    // convert back to utf8
    buff_wchar_ptr = buff_wchar;

    res = iconv(to_utf8,
        &buff_wchar_ptr, &wchar_len,
        &buff_utf8_ptr,  &buff_utf8_max_len
    );
    if (res == (size_t) -1){
        LOG(L_ERROR, "Cannot convert to lowercase: '%s'\n", str);
        LOG(L_ERROR, "iconv() failed");
        LOG(L_ERROR, "at %s:%d (%s)\n", __FILE__, __LINE__, __FUNCTION__);
        buff_utf8[0] = 0x00;
        return (char *)buff_utf8;
    }

    buff_utf8[CONVERT_BUFFER_SIZE - buff_utf8_max_len] = 0x00;
    return (char *)buff_utf8;
}

char to_hex(char code) {
    static char hex[] = "0123456789abcdef";
    return hex[code & 15];
}

char from_hex(char ch) {
  return isdigit(ch) ? ch - '0' : tolower(ch) - 'a' + 10;
}

char *url_encode(char *str, char *buf, size_t buf_len) {
    if(buf_len < (strlen(str) * 3 + 1)) {
        fprintf(stderr, "Buffer is too small, at leest %ld bytes needed\n", 
            (strlen(str) * 3 +1));
        return NULL;
    }
    char *pstr = str, *pbuf = buf;
    while (*pstr) {
        if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~') 
            *pbuf++ = *pstr;
        else if (*pstr == ' ') 
            *pbuf++ = '+';
        else 
            *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}

char *url_decode(char *str, char *buf, size_t buf_len) {
    if(buf_len < (strlen(str) + 1)) {
        fprintf(stderr, "Buffer is too small, at leest %ld bytes needed\n", 
            (strlen(str) +1));
        return NULL;
    }

    char *pstr = str, *pbuf = buf;
    while (*pstr) {
        if (*pstr == '%') {
            if (pstr[1] && pstr[2]) {
                *pbuf++ = from_hex(pstr[1]) << 4 | from_hex(pstr[2]);
                pstr += 2;
            }
        } else if (*pstr == '+') { 
            *pbuf++ = ' ';
        } else {
            *pbuf++ = *pstr;
        }
        pstr++;
    }
    *pbuf = '\0';
    return buf;
}

char *strtolower(char *s) {
    if(NULL == s) return NULL;
    int i;
    int n = strlen(s);
    for(i=0; i<n && i<CONVERT_BUFFER_SIZE-1; i++) {
        lower_buffer[i]= tolower(s[i]);
    }
    lower_buffer[++i] = '\0';
    return lower_buffer;
}