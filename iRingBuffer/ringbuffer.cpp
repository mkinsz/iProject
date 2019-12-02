/*
 * =====================================================================================
 *       Copyright (c), 2019-2020, MK.
 *       Filename:  ringbuffer.cpp
 *
 *    Description:  环形缓冲区的实现
 *         Others:  1.min的妙用，(验证剩余有效空间和要求要读出或者写入空间 取最小值)
 *                  2.利用unsigned int 的回环,in 和 out一直在加，加到0xffffffff则归为0，任然满足计算偏移等。
 *                  3.分为2部进行copy，一为当前偏移到size-1 二为剩余部分0到(len减去一中的个数)
 *                  4.unsiged int下的(in - out)始终为in和out之间的距离，(in溢出后in:0x1 - out:0xffffffff = 2任然满足)(缓冲区中未脏的数据).
 *                  5.计算偏移(in) & (size - 1) <==> in%size
 *        Version:  1.0
 *        Date:     Wednesday, March 20, 2019 10:00:00 CST
 *       Revision:  none
 *       Compiler:  nmake
 *
 *         Author:  MK
 *   Organization:  MK
 *        History:  Created by hesir
 * =====================================================================================
 */

#include "ringbuffer.h"

//判断x是否是2的次方
#define is_power_of_two(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
//取a和b中最小值
#define min(a, b) (((a) < (b)) ? (a) : (b))
//取比n大最小2的次方值
#define roundup_pow_of_two(n)      \
    (1UL    <<                     \
    (                              \
    (                              \
    (n) & (1UL << 31) ? 31 :       \
    (n) & (1UL << 30) ? 30 :       \
    (n) & (1UL << 29) ? 29 :       \
    (n) & (1UL << 28) ? 28 :       \
    (n) & (1UL << 27) ? 27 :       \
    (n) & (1UL << 26) ? 26 :       \
    (n) & (1UL << 25) ? 25 :       \
    (n) & (1UL << 24) ? 24 :       \
    (n) & (1UL << 23) ? 23 :       \
    (n) & (1UL << 22) ? 22 :       \
    (n) & (1UL << 21) ? 21 :       \
    (n) & (1UL << 20) ? 20 :       \
    (n) & (1UL << 19) ? 19 :       \
    (n) & (1UL << 18) ? 18 :       \
    (n) & (1UL << 17) ? 17 :       \
    (n) & (1UL << 16) ? 16 :       \
    (n) & (1UL << 15) ? 15 :       \
    (n) & (1UL << 14) ? 14 :       \
    (n) & (1UL << 13) ? 13 :       \
    (n) & (1UL << 12) ? 12 :       \
    (n) & (1UL << 11) ? 11 :       \
    (n) & (1UL << 10) ? 10 :       \
    (n) & (1UL <<  9) ?  9 :       \
    (n) & (1UL <<  8) ?  8 :       \
    (n) & (1UL <<  7) ?  7 :       \
    (n) & (1UL <<  6) ?  6 :       \
    (n) & (1UL <<  5) ?  5 :       \
    (n) & (1UL <<  4) ?  4 :       \
    (n) & (1UL <<  3) ?  3 :       \
    (n) & (1UL <<  2) ?  2 :       \
    (n) & (1UL <<  1) ?  1 :       \
    (n) & (1UL <<  0) ?  0 : -1    \
    ) + 1                          \
    )                              \
    )

RingBuffer::RingBuffer(uint32_t nSize)
{
    if(!is_power_of_two(nSize))
        nSize = roundup_pow_of_two(nSize);

    m_pBuffer = (uint8_t*)malloc(nSize);

    assert(m_pBuffer);

    m_nSize = nSize;
    m_nIn = m_nOut = 0;
}

RingBuffer::~RingBuffer()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    if(m_pBuffer) {
        free(m_pBuffer);
        m_pBuffer = NULL;
    }
    m_cv.notify_all();
}

uint32_t RingBuffer::put(void *pFrom, uint32_t nSize)
{
    std::lock_guard<std::mutex> lk(m_mutex);

    uint32_t nLen, nOff;

    //min(大小 - 已经用了的, nSize) 计算剩余可用空间
    nSize = min(m_nSize - (m_nIn - m_nOut), nSize);
    nOff  = (m_nIn + 0) & (m_nSize - 1); // ==> (m_nIn+0)%m_nSize
    nLen  = min(nSize, m_nSize - nOff);

    //nSize 的部分 nLen
    memcpy(m_pBuffer + nOff, pFrom, nLen);

    //nSize 的剩余部分，如果nSize == nLen则啥也不干
    memcpy(m_pBuffer, (uint8_t*)pFrom+nLen, nSize-nLen);

    m_nIn += nSize;

    m_cv.notify_one();

    return nSize;
}

uint32_t RingBuffer::get(void *pTo, uint32_t nSize)
{
    std::unique_lock<std::mutex> lk(m_mutex);

    m_cv.wait(lk, [&] { return m_nIn != m_nOut; });

    uint32_t nLen, nOff;

    //验证nSize是否大于缓冲区存的值
    nSize = min(m_nIn - m_nOut, nSize);
    nOff = (m_nOut + 0) & (m_nSize - 1); //==> (m_nOut + 0)%m_nSize
    nLen = min(nSize, m_nSize - nOff);

    //nSize 的部分 nLen
    memcpy(pTo, m_pBuffer+nOff, nLen);

    //nSize 的剩余部分，如果nSize == nLen则啥也不干
    memcpy((uint8_t*)pTo+nLen, m_pBuffer, nSize-nLen);

    m_nOut += nSize;

    return nSize;
}

uint32_t RingBuffer::length()
{
    std::lock_guard<std::mutex> lk(m_mutex);

    uint32_t nLen = m_nIn - m_nOut;

    return nLen;
}

uint32_t RingBuffer::head()
{
    std::lock_guard<std::mutex> lk(m_mutex);

    uint32_t nOff  = (m_nIn + 0) & (m_nSize - 1);

    return nOff;
}

uint32_t RingBuffer::tail()
{
    std::lock_guard<std::mutex> lk(m_mutex);

    uint32_t nOff  = (m_nOut + 0) & (m_nSize - 1);

    return nOff;
}

void RingBuffer::clear()
{
    std::lock_guard<std::mutex> lk(m_mutex);
    if(m_pBuffer) {
        free(m_pBuffer);
        m_pBuffer = NULL;
    }
}
