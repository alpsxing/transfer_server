#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/des.h>


static unsigned char crypto_key[]
        = { 'p', 'k', '$', '@', 'g', 't', 'j', 't'};

static unsigned char crypto_ivi[]
        = { 't', 'h', 'v', 'n', '#', '&', '@', '@'};

int encode_file(const char *input, const char *ofname)
{
    FILE *fpi = NULL, *fpo = NULL;
    int ret = -1;
    long sz, outsz;
    unsigned char *buf = NULL;
    des_key_schedule ks;
    des_cblock iv;

    fpi = fopen(input, "r");
    if (!fpi)
        goto exit;

    if (fseek(fpi, 0L, SEEK_END))
        goto exit;

    sz = ftell(fpi);
    if (sz <= 0)
        goto exit;

    rewind(fpi);

    outsz = sz + 8;
    outsz = ((outsz + 7) / 8) * 8;

    buf = (unsigned char *)malloc(outsz);
    if (!buf)
        goto exit;
    memset(buf, 0, outsz);

    if (sz != fread(buf, 1, sz, fpi))
        goto exit;

    buf[sz] = (unsigned char)(sz >> 24);
    buf[sz+1] = (unsigned char)((sz >> 16) & 0xFF);
    buf[sz+2] = (unsigned char)((sz >> 8) & 0xFF);
    buf[sz+3] = (unsigned char)(sz & 0xFF);
    buf[sz+4] = 'A';
    buf[sz+5] = 'B';
    buf[sz+6] = 'C';
    buf[sz+7] = 'D';

    if (des_key_sched((C_Block *)crypto_key, ks))
        goto exit;

    memcpy(iv,(unsigned char *)crypto_ivi, sizeof(crypto_ivi));

    des_ncbc_encrypt(buf, buf,
                     outsz, ks,
                     (C_Block *)iv, DES_ENCRYPT);

    fpo = fopen(ofname, "w+");
    if (!fpo)
        goto exit;

    if (outsz != fwrite(buf, 1, outsz, fpo))
        goto exit;

    ret = 0;
exit:
    if (fpi)
        fclose(fpi);
    if (fpo)
        fclose(fpo);
    if (buf)
        free(buf);
    return ret;
}

int decode_file(const char *input, const char *ofname)
{
    FILE *fpi = NULL, *fpo = NULL;
    int ret = -1;
    long sz;
    unsigned char *buf = NULL;
    des_key_schedule ks;
    des_cblock iv;

    fpi = fopen(input, "r");
    if (!fpi)
        goto exit;

    if (fseek(fpi, 0L, SEEK_END))
        goto exit;

    sz = ftell(fpi);
    if (sz <= 0)
        goto exit;

    rewind(fpi);

    buf = (unsigned char *)malloc(sz);
    if (!buf)
        goto exit;
    memset(buf, 0, sz);

    if (sz != fread(buf, 1, sz, fpi))
        goto exit;

    if (des_key_sched((C_Block *)crypto_key, ks))
        goto exit;

    memcpy(iv,(unsigned char *)crypto_ivi, sizeof(crypto_ivi));

    des_ncbc_encrypt(buf, buf,
                     sz, ks,
                     (C_Block *)iv, DES_DECRYPT);

    fpo = fopen(ofname, "w+");
    if (!fpo)
        goto exit;

    if (sz != fwrite(buf, 1, sz, fpo))
        goto exit;

    ret = 0;
exit:
    if (fpi)
        fclose(fpi);
    if (fpo)
        fclose(fpo);
    if (buf)
        free(buf);
    return ret;
}
