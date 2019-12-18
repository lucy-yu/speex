#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "speex/speex_callbacks.h"
#include "sb_celp.h"

#include "speex_encode_api.h"
#include "xm_encode.h"
#include "os_support.h"

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
//#include "stdio.h"

#ifdef FIXED_DEBUG
extern long long spx_mips;
#endif

#define FRAME_SIZE 320
#define  FRAME_O_BYTE_MAX   400
#define  SPEEX_ADD_HEADER_ENABLE     1
#define  NFRAME_HEAER        5

#define  SPEEX_COMPLEXITYv  2

typedef struct  _SPEEX_ENCSTATE_
{
    SBEncState st;
    SpeexBits bits;
    unsigned char bit_chars[FRAME_O_BYTE_MAX];
    short adcdata[FRAME_SIZE];
    //SPEEX_EN_FILE_IO enc_io;
    unsigned short frame_size;
}SPEEX_ENCSTATE __attribute__((aligned (4)));

static SPEEX_ENCSTATE* pSpeex_encstate = NULL;
static unsigned char uQuality = 8;
static unsigned short uSample_rate = SAMPLE_RATE_16K;
static spx_int32_t Complexity = SPEEX_COMPLEXITYv;
static int frame_cnt_o = 0;

__attribute__((weak)) void *speex_malloc(size_t size)
{
    return malloc(size);
}

__attribute__((weak)) void speex_free(void *ptr)
{
    return free(ptr);
}

unsigned int aivs_enc_init(const unsigned char quality,  enum Sample_Rate sample_rate)
{
    void *st=NULL;
    spx_int32_t tmp;
    //spx_int32_t skip_group_delay=0;
    if (quality <= 0 || quality >= 10 || sample_rate != SAMPLE_RATE_16K)
    {
        speex_log("aivs_enc_init() Error: the param is invalid\n");
        return AIVS_FAIL;
    }

    frame_cnt_o = 0;

    uQuality = quality;
    uSample_rate = sample_rate;
//    Complexity = complexity;

    if (NULL != pSpeex_encstate)
    {
        speex_free(pSpeex_encstate);
        pSpeex_encstate = NULL;
    }

    pSpeex_encstate = (SPEEX_ENCSTATE*) speex_malloc( sizeof(SPEEX_ENCSTATE) );
    if (!pSpeex_encstate)
    {
        speex_log("aivs_enc_init() Error: malloc memory failed for SPEEX_ENCSTATE\n");
        return AIVS_FAIL;
    }

    memset(pSpeex_encstate, 0, sizeof(SPEEX_ENCSTATE));
    st=&(pSpeex_encstate->st);
    if (uSample_rate <= 8000)
    {
        speex_encoder_init(speex_lib_get_mode(SPEEX_MODEID_NB),st);
        pSpeex_encstate->frame_size=160;
    }else
    {
        speex_encoder_init(speex_lib_get_mode(SPEEX_MODEID_WB),st);
        pSpeex_encstate->frame_size=320;
    }

    tmp=uQuality;
    speex_encoder_ctl(st, SPEEX_SET_QUALITY, &tmp);

    speex_log("speex_encode_int st:%p\n", st);
    //tmp= 2;
    //speex_encoder_ctl(st, SPEEX_SET_COMPLEXITY, &tmp);
    /* Turn this off if you want to measure SNR (on by default) */
   // tmp=1;
    //speex_encoder_ctl(st, SPEEX_SET_HIGHPASS, &tmp);
    //speex_encoder_ctl(st, SPEEX_GET_LOOKAHEAD, &skip_group_delay);

    pSpeex_encstate->bits.chars=(char *)pSpeex_encstate->bit_chars;
    pSpeex_encstate->bits.buf_size=FRAME_O_BYTE_MAX;

    //memcpy(&(pSpeex_encstate->enc_io),&speex_en_io,sizeof(SPEEX_EN_FILE_IO));
    //st.test_overflow=0xab;

    return AIVS_SUCCESS;
};

unsigned int aivs_enc_destory()
{
    if (NULL != pSpeex_encstate)
    {
        speex_free(pSpeex_encstate);
        pSpeex_encstate = NULL;
    }
    uQuality = 8;
    uSample_rate = SAMPLE_RATE_16K;
    Complexity = SPEEX_COMPLEXITYv;
    frame_cnt_o = 0;

    return AIVS_SUCCESS;
}

static inline void e_output_data_bydata(unsigned char* des,unsigned char *src, unsigned short len)
{
    unsigned char  tv[4] = {0xAA, 0xEA, 0xBD, 0xAC};

#if SPEEX_ADD_HEADER_ENABLE
    if (frame_cnt_o%NFRAME_HEAER == 0)
    {
        speex_log("add sync head data len:\n", len);
        memcpy(des, tv, sizeof(tv)/sizeof(unsigned char));
        des += sizeof(tv)/sizeof(unsigned char);
        memcpy(des, &len, sizeof(unsigned short));
        des += sizeof(unsigned short);
        frame_cnt_o = 0;
    }else{
        frame_cnt_o++;
    }
#endif
    memcpy(des, src, len);
}

unsigned int aivs_enc_stream(const short* pInputData, int inputDataLen, unsigned char* pOutputData, int* outputLen)
{
    if (NULL == pInputData || inputDataLen <= 0 || pOutputData == NULL || outputLen == NULL)
    {
        speex_log("aivs_enc_stream() Error: param is invalid!\n");
        return AIVS_FAIL;
    }
    speex_log("loop encode start %s  %s  %d \n", __FILE__, __FUNCTION__, __LINE__);
    void *st = &(pSpeex_encstate->st);
    int offset = 0;
    unsigned char* ptmp = pOutputData;
    int encodeLen = 0;
    int inputShortCount = inputDataLen/2;
    unsigned char  tv[4] = {0xAA, 0xEA, 0xBD, 0xAC};
    unsigned int encodeFrameLen = 0;

    while ((offset + pSpeex_encstate->frame_size) <= inputShortCount)
    {
        //memset(pSpeex_encstate->adcdata, 0, sizeof(short)*FRAME_SIZE);
        speex_log("loop encode start len:%d offset:%d \n", encodeLen, offset);
        memcpy(pSpeex_encstate->adcdata, pInputData + offset, sizeof(short)*pSpeex_encstate->frame_size);
        speex_log("111111111111111111111");
        speex_bits_reset(&pSpeex_encstate->bits);

        speex_log("22222222222222");
        speex_encode_int(st, pSpeex_encstate->adcdata, &pSpeex_encstate->bits);

        speex_log("3333333333333333333");
        encodeFrameLen = (pSpeex_encstate->bits.nbBits+7)>>3;

#if SPEEX_ADD_HEADER_ENABLE
        if (frame_cnt_o%NFRAME_HEAER == 0)
        {
            speex_log("add sync head data len\n");
            memcpy(ptmp, tv, sizeof(tv)/sizeof(unsigned char));
            ptmp += sizeof(tv)/sizeof(unsigned char);
            memcpy(ptmp, &encodeFrameLen, sizeof(unsigned short));
            ptmp += sizeof(unsigned short);
            frame_cnt_o = 0;
            encodeLen +=6;
        }else{
            frame_cnt_o++;
        }
#endif
        speex_log("44444444444444");

        memcpy(ptmp, pSpeex_encstate->bit_chars, encodeFrameLen);
        /* offset data len */
        ptmp += encodeFrameLen;
        encodeLen += encodeFrameLen;
        // get next Frame
        offset += pSpeex_encstate->frame_size;

        speex_log("loop encode end len:%d offset:%d \n", encodeLen, offset);
    }
    if (offset < inputShortCount)
    {
        memset(pSpeex_encstate->adcdata, 0, sizeof(short)*FRAME_SIZE);
        memcpy(pSpeex_encstate->adcdata, pInputData + offset, sizeof(short) * (inputShortCount - offset));
        speex_bits_reset(&pSpeex_encstate->bits);
        speex_encode_int(st, pSpeex_encstate->adcdata, &pSpeex_encstate->bits);
        encodeFrameLen = (pSpeex_encstate->bits.nbBits+7)>>3;

#if SPEEX_ADD_HEADER_ENABLE
        if (frame_cnt_o%NFRAME_HEAER == 0)
        {
            speex_log("add sync head data len\n");
            memcpy(ptmp, tv, sizeof(tv)/sizeof(unsigned char));
            ptmp += sizeof(tv)/sizeof(unsigned char);
            memcpy(ptmp, &encodeFrameLen, sizeof(unsigned short));
            ptmp += sizeof(unsigned short);
            frame_cnt_o = 0;
            encodeLen +=6;
        }else{
            frame_cnt_o++;
        }
#endif
        memcpy(ptmp, pSpeex_encstate->bit_chars, encodeFrameLen);
        /* offset data len */
        ptmp += encodeFrameLen;
        encodeLen += encodeFrameLen;
        // get next Frame
        offset += pSpeex_encstate->frame_size;
        speex_log("warning: InputDataLen suggest to be 640*n\n");
    }
    *outputLen = encodeLen;

    return AIVS_SUCCESS;
}

#ifdef LOCAL_TEST
int main(int argc, char **argv)
{
    char *inFile, *bitsFile;
    int enclen = 0;
    short  indata[FRAME_SIZE];
    unsigned char* outdata = NULL;
    FILE *fin = NULL, *fbits = NULL;

//    char *filetemp;
//    FILE *fileframe;
#if 0
    aivs_enc_init(DEF_QUALITY,SAMPLE_RATE_16K,COMPLEXITY);

    inFile = argv[1];
    bitsFile = argv[2];


    aivs_enc_file(inFile, bitsFile);
    aivs_enc_destory();

#else
    aivs_enc_init(DEF_QUALITY,SAMPLE_RATE_16K);

    inFile = argv[1];
    bitsFile = argv[2];
//    filetemp  = argv[3];
//    fileframe = fopen(filetemp, "wb");

    frame_cnt_o = 0;
    fin = fopen(inFile, "rb");
    if (NULL == fin)
    {
        speex_log("Error: fopen Infile failed!\n");
        return AIVS_FAIL;
    }
    fbits = fopen(bitsFile, "wb");
    if (NULL == fbits)
    {
        speex_log("Error: fopen OutFile failed!\n");
        fclose(fin);
        return AIVS_FAIL;
    }

    outdata = (unsigned char*)speex_malloc(FRAME_SIZE*sizeof(unsigned char));
    if (NULL == outdata)
    {
        speex_log("Error: malloc outdata failed\n");
        fclose(fin);
        fclose(fbits);
        return AIVS_FAIL;
    }

    while (!feof(fin))
    {
        if (feof(fin))
            break;

        memset(indata, 0, sizeof(short)*FRAME_SIZE);
        fread(indata, 1, sizeof(short)*FRAME_SIZE, fin);
        memset(outdata, 0, FRAME_SIZE*sizeof(unsigned char));

        aivs_enc_stream(indata, sizeof(indata), outdata, &enclen);

        fwrite(outdata,sizeof(unsigned char), enclen, fbits);
        //fwrite(indata, sizeof(short), FRAME_SIZE, fileframe);
        //break;
    }
    aivs_enc_destory();
    speex_free(outdata);
    if (NULL != fin)
    {
        fclose(fin);
        fin = NULL;
    }
    if (NULL != fbits)
    {
        fclose(fbits);
        fbits = NULL;
    }
#endif
    return 0;
}
#endif
