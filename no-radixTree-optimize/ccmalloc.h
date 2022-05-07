#pragma once
#include "ThreadCache.h"
#include "PageCache.h"
// ConCurrentAlloc == ccmalloc
// ConCurrentFree == ccfree

static void* ccmalloc(size_t size) {
    // 1.�������256kB���ڴ�
    if (size > MAX_SIZE) {
        // ����ҳ��
        PAGE_ID kpage = (SizeClass::Align(size)) >> PAGE_SHIFT;
        
        //// �������256kB ����С�ڵ���(MAX_PAGE-1)ҳ���ڴ�
        //if (size <= 128 * 8 * 1024) {
        //    PageCache::GetInstance()->_mtx.lock();
        //    Span* span = PageCache::GetInstance()->NewSpan(kpage);
        //    span->_page_block_size = size;
        //    PageCache::GetInstance()->_mtx.unlock();
        //    return (void*)(span->_page_id<<PAGE_SHIFT);
        //}
        //// �������(MAX_PAGE-1)ҳ���ڴ�
        //else {
        //    void* ptr = SystemAlloc(kpage);
        //    return ptr;
        //}

        PageCache::GetInstance()->_mtx.lock();
        Span* span = PageCache::GetInstance()->NewSpan(kpage);
        span->_page_block_size = size;
        PageCache::GetInstance()->_mtx.unlock();
        return (void*)(span->_page_id << PAGE_SHIFT);
    }
    // 2.���� <= 256kB���ڴ�
    else {
        if (pThreadCache == nullptr) {
            pThreadCache = new ThreadCache();
        }
        return pThreadCache->Allocate(size);
    }
};

static void ccfree(void* ptr) {
    //pThreadCache->Deallocate(ptr, (PageCache::GetInstance()->_pageid_span_map[(PAGE_ID)ptr >> PAGE_SHIFT])->_page_block_size);
    PAGE_ID page_id = (PAGE_ID)(ptr) >> PAGE_SHIFT;
    Span* span = PageCache::GetInstance()->_pageid_span_map[page_id];
    size_t size = span->_page_block_size;
    if (size > MAX_SIZE) {
        PageCache::GetInstance()->_mtx.lock();
        PageCache::GetInstance()->GetPageFromCentralCacheOrBigSpan(span);
        PageCache::GetInstance()->_mtx.unlock();
    }
    else {
        pThreadCache->Deallocate(ptr, size);
    }
}