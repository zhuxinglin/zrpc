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
*/

#ifndef __SHM__STRUCT_H__
#define __SHM__STRUCT_H__

#define SHM_ADDR_HEADER_KEY     0x7FFF6478
#define SHM_ADDR_DATA_KEY       0x7FFF6479

#define HASH_ADD_COLLIDE_MARK   0x8FFFFFFF
#define HASH_COLLIDE_MARK       0x80000000
#define HASH_DATA_LENGTH        0x7FFFFFFF

// 共享数据头，用于数据更改时交换使用
typedef struct _ShmAddrHeader
{
    uint32_t dwBlockSize;        // 总数据块大小
    uint32_t dwChange;           // 是否发生了变化
    uint8_t wSycLock;            // 同步锁
    uint8_t wReserve[3];         // 保留
}ShmAddrHeader;

typedef struct _ShmDataHeader
{
    uint32_t dwHashTableCount;   // hash 数据组大小
    uint32_t dwChange;           // 是否发生了变化
    char szData[0];
}ShmDataHeader;

// 共享数据hash表
typedef struct _ShmHashTable
{
    // 存放数据的地址
    // 最高位为冲突位 (1表示冲突，0不冲突)
    uint32_t dwDataAddr;
}ShmHashTable;

#pragma pack(2)
typedef struct _ShmHashData
{
    // 数据Key长度
    uint8_t wKeyLen;
    // 空闭长度
    uint8_t wBackLen;
    // 锁
    uint8_t wLock;
    // 保留
    uint8_t wReserve;
    // 引用计数
    uint16_t wReference;
    // 数据值长度
    uint16_t wValueLen;
    // 冲突地址
    uint32_t dwDataAddr;

    // 数据地址
    char szData[0];
}ShmHashData;
#pragma pack()


static inline uint64_t ELFHashCode(const char* key)
{
    uint64_t h = 0;
    uint64_t g;
    while (*key)
    {
        h = (h << 4) + *key;
        ++ key;
        g = h & 0xF0000000;
        if (g) h ^= g >> 24;
        h &= ~g;
    }
    return h;
}

#endif

