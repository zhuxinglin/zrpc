package shmconfig

/*
#cgo LDFLAGS:-L. -lshmconfig -lstdc++
#cgo CFLAGS: -I./
#include "shm_config_go.h"
#include <stdlib.h>
*/
import "C"
import "unsafe"

func GetValue(key, defaul_tvalue string) string {
    var pValue *C.char;
    var k *C.char = C.CString(key);
    iRet := C.GetValue(k, &pValue);
    defer C.free(unsafe.Pointer(k))
    if iRet < 0{
        return defaul_tvalue;
    }
    defer C.FreeValue(pValue);
    return C.GoStringN(pValue, iRet)
}

func GetValueI(key string, defaul_tvalue uint64) uint64 {
    var k *C.char = C.CString(key);
    val := C.GetValueI(k, C.uint64_t(defaul_tvalue));
    defer C.free(unsafe.Pointer(k))
    return uint64(val);
}
