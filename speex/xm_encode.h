#ifndef __XM_ENCODE_H__
#define __XM_ENCODE_H__

#include <stdio.h>
#include <stdlib.h>

#define AIVS_SUCCESS 0
#define AIVS_FAIL 1
#define DEF_QUALITY 8
#define COMPLEXITY  2

enum Sample_Rate
{
    SAMPLE_RATE_8K = 8000,
    SAMPLE_RATE_16K = 16000
};

/* 初始化编码环境 */
/*
  Param：
     quality  : 编码质量 推荐8. quality小，压缩比高，消耗cpu资源越少
     sample_rate : 必须为SAMPLE_RATE_16K
     complexity : how the search is performed，小，占用资源少
*/
unsigned int aivs_enc_init(const unsigned char quality, enum Sample_Rate sample_rate);
//对数据进行编码
/*
  pInputData：编码数据。采样格式为S16_LE
  inputDataLen: pInputData 数据长度
  pOutputData:编码输出结果数据.内存由调用者分配和释放。
  outputLen:编码结果pOutputData长度.用户据此长度发送内容pOutputData,或者缓存多个pOutputData一起发送。
*/
unsigned int aivs_enc_stream(const short* pInputData, int inputDataLen, unsigned char* pOutputData, int* outputLen);

//销毁编码环境
unsigned int aivs_enc_destory();


// __attribute__((weak))  函数 如需覆盖系统默认malloc
// 请重新实现此speex_malloc 覆盖系统默认malloc
void *speex_malloc(size_t size);

// __attribute__((weak))  函数 如需覆盖系统默认malloc
//   请重新实现speex_free
void speex_free(void *ptr);


/* 默认不开日志 有需要定制  */
#include "stdio.h"
#include "stdarg.h"

void speex_log(const char *format, ...);

#define LOG_SIZE  512

__attribute__((weak)) void speex_log(const char *format, ...)
{
    va_list argptr;
    va_start(argptr, format);
    char buffer[512];
    vsnprintf(buffer, 512, format, argptr);
    va_end(argptr);

    printf("%s\n", buffer);
}

#endif
