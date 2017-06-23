#include <stdio.h>
#include <iconv.h>

#define INPUT_BUF_SZ  1024
#define OUTPUT_BUF_SZ (INPUT_BUF_SZ*2)

int encode_to_utf8(const char *input_file, const char *output_file)
{
    FILE *fpi = NULL, *fpo = NULL;
    char ibuf[INPUT_BUF_SZ], obuf[OUTPUT_BUF_SZ];
    char *pi, *po;
    int ret = -1;
    size_t isize, osize;
    iconv_t ic = (iconv_t)-1;;

    fpi = fopen(input_file, "r");
    if (!fpi)
        goto exit;

    fpo = fopen(output_file, "w+");
    if (!fpo)
        goto exit;

    ic = iconv_open("utf-8", "gb2312");
    if (ic == (iconv_t)-1)
        goto exit;

    while ((isize = fread(ibuf, 1, sizeof(ibuf), fpi)) > 0) {
        pi = ibuf;
        po = obuf;
        osize = sizeof(obuf);
        if (iconv(ic, &pi, &isize, &po, &osize) < 0)
            goto exit;
        if (isize != 0)
            goto exit;
        osize = sizeof(obuf) - osize;
        if (osize != fwrite(obuf, 1, osize, fpo))
            goto exit;
    }

    ret = 0;

exit:
    if (fpi)
        fclose(fpi);
    if (fpo)
        fclose(fpo);
    if (ic != (iconv_t)-1)
        iconv_close(ic);
    return ret;

}
