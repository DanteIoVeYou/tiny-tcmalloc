#define _CRT_SECURE_NO_WARNINGS 1
#include "ThreadCache.h"
#include "CentralCache.h"
void* ThreadCache::Allocate(size_t size) { // 给线程分配内存的接口
    size_t align_num = SizeClass::Align(size);
    size_t index = SizeClass::Index(size);
    if (_free_lists[index].Empty()) {
        return FetchFromCentralCache(align_num, index); // 桶里没有内存，向上申请内存，再分配
    } 
    else {
        return _free_lists[index].Pop();
    }
}

void ThreadCache::Deallocate(void* ptr, size_t size) {
    assert(ptr);
    assert(size <= MAX_SIZE);
    // 插到正确的桶里
    size_t index = SizeClass::Index(size); // 还到哪一个桶里面去
    _free_lists[index].Push(ptr);

    // 判断需不需要将该桶的内存还给CentralCache
    // 如果当前桶的自由链表上挂的内存块数目大于等于自由链表的_max_size，就还给上一级，所以我们要给自由链表添加一个成员变量：_size，记录挂了多少内存块
    if (_free_lists[index].Size() >= _free_lists[index].MaxSize()) {
        void* start = _free_lists[index].PopRange(_free_lists[index].MaxSize());
        CentralCache::GetInstance()->GetBackFromThreadCache(start, size);
    }
} // 让线程还内存的接口


void* ThreadCache::FetchFromCentralCache(size_t size, size_t index) {
    // 向index下标的CentralCache哈希桶申请一批过量的内存
    // 1. 返回一个内存块
    // 2. 把多余的内存块给挂到ThreadCache的对应哈希桶里面
    // 这里采用的是慢增长反馈调节算法，在每个ThreadCache的桶里记录一个max_size，该桶向CentralCache申请的越多，max_size就会越大
    size_t batch_size = min(_free_lists[index].MaxSize(), SizeClass::Batch(size));
    _free_lists[index].MaxSize()++;

    // 需要向CentralCache的下标为index的桶申请 batch_size个 size大小的内存块，返回一个内存块，其余内存块头插到ThreadCache的自由链表中
    void* start = nullptr;
    void* end = nullptr;
    size_t actual_batch_size = CentralCache::GetInstance()->GiveToThreadCache(start, end, batch_size, size);

    if (start == nullptr) {
        int x = 0;
    }
    if (actual_batch_size == 1) {
        assert(start == end);
    }
    else {
        _free_lists[index].PushRange(NextObj(start), end, actual_batch_size - 1);
    }
    return start;
} // ThreadCache的某个桶没有内存了，就向上级CentralCache申请内存