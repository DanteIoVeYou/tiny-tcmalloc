#include "common.h"

class ThreadCache {
public:
    ThreadCache() {}
    ~ThreadCache() {}
    void* Allocate(size_t size); // 给线程分配内存的接口
    void Deallocate(void* ptr, size_t size); // 让线程还内存的接口
    void* FetchFromCentralCache(size_t size, size_t index); // ThreadCache的某个桶没有内存了，就向上级CentralCache申请内存
private:
    FreeList _free_lists[BUCKET_AMOUNT]; // 总共208个桶
};

static _declspec(thread) ThreadCache* pThreadCache = nullptr;