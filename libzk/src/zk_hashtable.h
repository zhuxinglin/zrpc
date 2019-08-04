/*
* zk hash table
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

#ifndef __ZK_HASH_TABLE_H_
#define __ZK_HASH_TABLE_H_

#include <list>
#include <string>
#include "hashtable/hashtable_itr.h"

namespace zkapi
{

class ZkHashTable
{
public:
    static hashtable *create_zk_hashtable();
    static void destroy_zk_hashtable(hashtable* h);
    static void collect_keys(hashtable *ht, std::list<std::string> &l);
};


}


#endif
