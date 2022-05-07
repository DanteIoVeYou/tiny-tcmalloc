#include "common.h"

class ThreadCache {
public:
    ThreadCache() {}
    ~ThreadCache() {}
    void* Allocate(size_t size); // ���̷߳����ڴ�Ľӿ�
    void Deallocate(void* ptr, size_t size); // ���̻߳��ڴ�Ľӿ�
    void* FetchFromCentralCache(size_t size, size_t index); // ThreadCache��ĳ��Ͱû���ڴ��ˣ������ϼ�CentralCache�����ڴ�
private:
    FreeList _free_lists[BUCKET_AMOUNT]; // �ܹ�208��Ͱ
};

static _declspec(thread) ThreadCache* pThreadCache = nullptr;