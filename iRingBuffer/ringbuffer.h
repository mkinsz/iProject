#ifndef RINGBUFFER_H
#define RINGBUFFER_H

#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <mutex>
#include <condition_variable>

class RingBuffer
{
public:
    RingBuffer(uint32_t nSize);
    ~RingBuffer();

    uint32_t put(void *pFrom, uint32_t nSize);
    uint32_t get(void *pTo, uint32_t nSize);

    uint32_t length();

    uint32_t head();
    uint32_t tail();

    void clear();

private:
    uint8_t  *m_pBuffer = NULL;

    uint32_t m_nSize;
    uint32_t m_nIn;
    uint32_t m_nOut;

    std::mutex m_mutex;
    std::condition_variable m_cv;
};

#endif // RINGBUFFER_H
