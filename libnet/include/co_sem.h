/*
*
*
*
*
*
*
*
*
*
*
*
*
* 
 */

#ifndef __CO_SEM__H__
#define __CO_SEM__H__

#include <stdint.h>

class CCoSem
{
public:
    CCoSem();
    ~CCoSem();

public:
    void Post();
    bool Wait(uint32_t dwTimeout);

private:
    uint64_t m_qwCurId;
};


#endif
