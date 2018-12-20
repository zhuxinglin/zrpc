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

#ifndef __SO_UNINSTALL_H__
#define __SO_UNINSTALL_H__

#include <task_base.h>
#include "so_plugin.h"
#include <so_struct.h>

class CSoUninstall : public ITaskBase
{
public:
    CSoUninstall();
    ~CSoUninstall();

public:
    void SetPluginObj(CSoPlugin* pPlugin);
    void SetSoMap(map_so_info* pmapSo);

private:
    virtual void Run();
    virtual void Release();

private:
    CSoPlugin* m_pPlugin;
    map_so_info* m_pmapSo;
};


#endif
