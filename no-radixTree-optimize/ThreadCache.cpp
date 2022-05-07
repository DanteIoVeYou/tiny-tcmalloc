#define _CRT_SECURE_NO_WARNINGS 1
#include "ThreadCache.h"
#include "CentralCache.h"
void* ThreadCache::Allocate(size_t size) { // ���̷߳����ڴ�Ľӿ�
    size_t align_num = SizeClass::Align(size);
    size_t index = SizeClass::Index(size);
    if (_free_lists[index].Empty()) {
        return FetchFromCentralCache(align_num, index); // Ͱ��û���ڴ棬���������ڴ棬�ٷ���
    } 
    else {
        return _free_lists[index].Pop();
    }
}

void ThreadCache::Deallocate(void* ptr, size_t size) {
    assert(ptr);
    assert(size <= MAX_SIZE);
    // �嵽��ȷ��Ͱ��
    size_t index = SizeClass::Index(size); // ������һ��Ͱ����ȥ
    _free_lists[index].Push(ptr);

    // �ж��費��Ҫ����Ͱ���ڴ滹��CentralCache
    // �����ǰͰ�����������Ϲҵ��ڴ����Ŀ���ڵ������������_max_size���ͻ�����һ������������Ҫ�������������һ����Ա������_size����¼���˶����ڴ��
    if (_free_lists[index].Size() >= _free_lists[index].MaxSize()) {
        void* start = _free_lists[index].PopRange(_free_lists[index].MaxSize());
        CentralCache::GetInstance()->GetBackFromThreadCache(start, size);
    }
} // ���̻߳��ڴ�Ľӿ�


void* ThreadCache::FetchFromCentralCache(size_t size, size_t index) {
    // ��index�±��CentralCache��ϣͰ����һ���������ڴ�
    // 1. ����һ���ڴ��
    // 2. �Ѷ�����ڴ����ҵ�ThreadCache�Ķ�Ӧ��ϣͰ����
    // ������õ������������������㷨����ÿ��ThreadCache��Ͱ���¼һ��max_size����Ͱ��CentralCache�����Խ�࣬max_size�ͻ�Խ��
    size_t batch_size = min(_free_lists[index].MaxSize(), SizeClass::Batch(size));
    _free_lists[index].MaxSize()++;

    // ��Ҫ��CentralCache���±�Ϊindex��Ͱ���� batch_size�� size��С���ڴ�飬����һ���ڴ�飬�����ڴ��ͷ�嵽ThreadCache������������
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
} // ThreadCache��ĳ��Ͱû���ڴ��ˣ������ϼ�CentralCache�����ڴ�