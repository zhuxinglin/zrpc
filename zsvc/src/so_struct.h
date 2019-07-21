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

#ifndef __SO_STRUCT_H__
#define __SO_STRUCT_H__

#include "plugin_base.h"
#include <dlfcn.h>
#include <memory>

namespace zrpc
{

typedef struct _SoFunAddr
{
    void *pSoHandle;
    zplugin::SoExportFunAddr pFun;
    zplugin::CPluginBase *pPlugin;
    std::string sSoName;
    int iDelFlag;
    volatile uint32_t dwCount;
    std::shared_ptr<zplugin::CSharedData> pSo;
} CSoFunAddr;

typedef std::set<uint64_t> set_key;
typedef set_key::iterator set_key_it;
typedef std::map<CSoFunAddr *, set_key *> map_so_info;
typedef map_so_info::iterator map_so_info_it;

}

#endif
