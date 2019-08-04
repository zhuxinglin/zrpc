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

#include "zk_hashtable.h"
#include <string.h>
#include "hashtable/hashtable.h"

using namespace zkapi;

static unsigned int string_hash_djb2(void *str)
{
    unsigned int hash = 5381;
    int c;
    const char *cstr = (const char *)str;
    while ((c = *cstr++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    return hash;
}

static int string_equal(void *key1, void *key2)
{
    return strcmp((const char *)key1, (const char *)key2) == 0;
}

hashtable *ZkHashTable::create_zk_hashtable()
{
    return create_hashtable(32U, string_hash_djb2, string_equal);
}

void ZkHashTable::destroy_zk_hashtable(hashtable* h)
{
    hashtable_destroy(h, 0);
}

void ZkHashTable::collect_keys(hashtable *ht, std::list<std::string> &l)
{
    int count = hashtable_count(ht);
    hashtable_itr *it = hashtable_iterator(ht);
    for (int i = 0; i < count; ++ i)
    {
        std::string key = (char*)hashtable_iterator_key(it);
        hashtable_iterator_advance(it);
        l.push_back(key);
    }
    free(it);
}
