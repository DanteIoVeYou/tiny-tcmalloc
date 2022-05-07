#pragma once

#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#endif //����32/64λOS��ҳ��

#ifdef _WIN32
#include <Windows.h>
#endif
// lib header file
#include <iostream>
#include <vector>
#include <unordered_map>
#include <ctime>
#include <cassert>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>
#define INIT_POOL_SIZE (1 << 13)
#define INIT_POOL_PAGE 2
#define MAX_SIZE 256*1024
#define BUCKET_AMOUNT 208
#define MAX_PAGE 129
#define PAGE_SHIFT 13
static void*& NextObj(void* obj) {
    return *(void**)obj;
}


// ��ҳΪ��λ����
inline static void* SystemAlloc(size_t kpage)
{
#ifdef _WIN32
    void* ptr = VirtualAlloc(0, (kpage << 13), MEM_COMMIT | MEM_RESERVE,
        PAGE_READWRITE);
#else
    // linux��brk mmap��
#endif
    if (ptr == nullptr)
        throw std::bad_alloc();
    return ptr;
}

inline static void SystemFree(void* ptr) {
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
#endif
}

class FreeList {
public:
    FreeList() : _free_list(nullptr) {}

    // ͷ��һ���ڴ��
    void Push(void* obj) {
        assert(obj);
        NextObj(obj) = _free_list;
        _free_list = obj;
        _size++;
    }
    //ͷ��һ���ڴ��
    void PushRange(void* start, void* end, size_t num) {
        if (start == nullptr) {
            int y = 0;
            y++;
        }
        assert(start);
        assert(end);
        NextObj(end) = _free_list;
        _free_list = start;
        _size += num;
    }
    // ͷɾ
    void* Pop() {
        assert(_free_list != nullptr);
        void* obj = _free_list;
        _free_list = NextObj(obj);
        _size--;
        return obj;
    }
    // ͷɾһ���ڴ�
    void* PopRange(size_t num) {
        assert(num <= _size);
        void* start = _free_list;
        void* end = _free_list;
        for (size_t i = 0; i < num - 1; i++) {
            end = NextObj(end);
        }
        _free_list = NextObj(end);
        NextObj(end) = nullptr;
        _size -= num;
        return start;
    }
    bool Empty() {
        return _free_list == nullptr;
    }
    size_t Size() {
        return _size;
    }
    size_t& MaxSize() {
        return _max_size;
    }
private:
    void* _free_list;
    size_t _max_size = 1;
    size_t _size = 0;
};

class SizeClass {
public:
    // �����ڴ������
    static size_t _Align(size_t size, size_t align_num) {
        if (size % align_num == 0) {
            return size;
        }
        else {
            return (size / align_num + 1) * align_num;
        }
    }
    static size_t Align(size_t size) {
        if (size <= 128) {
            return _Align(size, 8);
        }
        else if (size <= 1024) {
            return _Align(size, 16);
        }
        else if (size <= 1024 * 8) {
            return _Align(size, 128);

        }
        else if (size <= 1024 * 64) {
            return _Align(size, 1024);

        }
        else if (size <= 1024 * 256) {
            return _Align(size, 8192);
        }
        else { // >256KB
            return _Align(size, 8 * 1024);
        }
    }
    // ��������xx�ֽڵ��ڴ���Ҫȥ�ĸ��±�Ĺ�ϣͰ����ȡ
    static size_t _Index(size_t size, size_t align_num) {
        if (size % align_num == 0) {
            return size / align_num - 1;
        }
        else {
            return size / align_num;
        }
    }
    static size_t Index(size_t size) {
        static size_t bucket[4] = { 16, 56, 56, 56 };
        if (size <= 128) {
            return _Index(size, 8);
        }
        else if (size <= 1024) {
            return _Index(size, 16) + bucket[0];
        }
        else if (size <= 8 * 1024) {
            return _Index(size, 128) + bucket[0] + bucket[1];
        }
        else if (size <= 64 * 1024) {
            return _Index(size, 128 * 8) + bucket[0] + bucket[1] + bucket[2];
        }
        else if (size <= 256 * 1024) {
            return _Index(size, 1024 * 8) + bucket[0] + bucket[1] + bucket[2] + bucket[3];
        }
        else {
            //error
            assert(false);
            return -1;
        }
    }
    // ThreadCache����һ���ڴ����������
    static size_t Batch(size_t size) {
        size_t batch_size = MAX_SIZE / size;
        if (batch_size >= 512) {
            batch_size = 512;
        }
        else if (batch_size <= 2) {
            batch_size = 2;
        }
        return batch_size;
    }
    static size_t PageBatch(size_t size) {
        size_t batch_size = Batch(size);
        size_t total_size = size * batch_size;
        size_t page_size = (total_size >> 13);
        if (page_size < 1) {
            page_size = 1;
        }
        return page_size;
    }
};

struct Span {
    // ҳ��
    PAGE_ID _page_id = 0;
    // ҳ������
    size_t _page_amount = 0;
    // ҳ��ÿ���ڴ��Ĵ�С
    size_t _page_block_size = 0;
    // ��ʹ���ڴ������
    size_t _used_amount = 0;
    // �Ƿ��ڱ�ʹ��
    bool _is_used = false;
    // ǰ��ָ��
    Span* _prev = nullptr;
    Span* _next = nullptr;
    // �����к��ڴ�����������
    void* _free_list = nullptr;
};

class SpanList {
public:
    SpanList() : _head(new Span) {
        _head->_next = _head;
        _head->_prev = _head;
    }
    Span* Begin() {
        return _head->_next;
    }
    Span* End() {
        return _head;
    }
    void Insert(Span* pos, Span* new_span) {
        // ��posǰ�����new_span
        Span* next = pos;
        Span* prev = pos->_prev;
        prev->_next = new_span;
        new_span->_prev = prev;
        new_span->_next = next;
        next->_prev = new_span;
    }
    void PushFront(Span* new_span) {
        Insert(Begin(), new_span);
    }
    Span* Erase(Span* pos) {
        assert(pos);
        Span* next = pos->_next;
        Span* prev = pos->_prev;
        prev->_next = next;
        next->_prev = prev;
        return pos;
    }
    Span* PopFront() {
        assert(Begin() != nullptr);
        return Erase(_head->_next);
    }
    bool Empty() {
        return _head == _head->_next;
    }
public:
    Span* _head;
    std::mutex _mtx; // Ͱ��
};