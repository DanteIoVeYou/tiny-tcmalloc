#define _CRT_SECURE_NO_WARNINGS 1

#include "ccmalloc.h"
void BenchmarkMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
    std::vector<std::thread> vthread(nworks);
    std::atomic<size_t> malloc_costtime = 0;
    std::atomic<size_t> free_costtime = 0;
    for (size_t k = 0; k < nworks; ++k)
    {
        vthread[k] = std::thread([&, k]() {
            std::vector<void*> v;
            v.reserve(ntimes);
            for (size_t j = 0; j < rounds; ++j)
            {
                size_t begin1 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    v.push_back(malloc(16));
                    //v.push_back(malloc((16 + i) % 8192 + 1));
                }
                size_t end1 = clock();
                size_t begin2 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    free(v[i]);
                }
                size_t end2 = clock();
                v.clear();
                malloc_costtime += (end1 - begin1);
                free_costtime += (end2 - begin2);
            }
            });
    }
    for (auto& t : vthread)
    {
        t.join();
    }
    //printf("%u个线程并发执行%u轮次，每轮次malloc %u次: 花费：%u ms\n",
    //    nworks, rounds, ntimes, malloc_costtime);
    //printf("%u个线程并发执行%u轮次，每轮次free %u次: 花费：%u ms\n",
    //    nworks, rounds, ntimes, free_costtime);
    //printf("%u个线程并发malloc&free %u次，总计花费：%u ms\n",
    //    nworks, nworks * rounds * ntimes, malloc_costtime + free_costtime);

    printf("%u个线程并发执行%u轮次，每轮次malloc %u次:",
        nworks, rounds, ntimes);
    std::cout << " 花费：" << malloc_costtime << "ms" << std::endl;
    printf("%u个线程并发执行%u轮次，每轮次malloc %u次:",
        nworks, rounds, ntimes);
    std::cout << " 花费：" << free_costtime << "ms" << std::endl;
    printf("%u个线程并发执行%u轮次，每轮次malloc %u次:",
        nworks, rounds, ntimes);
    std::cout << " 花费：" << malloc_costtime + free_costtime << "ms" << std::endl;
}

void BenchmarkConcurrentMalloc(size_t ntimes, size_t nworks, size_t rounds)
{
    std::vector<std::thread> vthread(nworks);
    std::atomic<size_t> malloc_costtime = 0;
    std::atomic<size_t> free_costtime = 0;
    for (size_t k = 0; k < nworks; ++k)
    {
        vthread[k] = std::thread([&]() {
            std::vector<void*> v;
            v.reserve(ntimes);
            for (size_t j = 0; j < rounds; ++j)
            {
                size_t begin1 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    v.push_back(ccmalloc(16));
                    //v.push_back(ConcurrentAlloc((16 + i) % 8192 + 1));
                }
                size_t end1 = clock();
                size_t begin2 = clock();
                for (size_t i = 0; i < ntimes; i++)
                {
                    ccfree(v[i]);
                }
                size_t end2 = clock();
                v.clear();
                malloc_costtime += (end1 - begin1);
                free_costtime += (end2 - begin2);
            }
            });
    }
    for (auto& t : vthread)
    {
        t.join();
    }
    //printf("%u个线程并发执行%u轮次，每轮次concurrent alloc %u次: 花费：%u ms\n",
    //    nworks, rounds, ntimes, malloc_costtime);
    //printf("%u个线程并发执行%u轮次，每轮次concurrent dealloc %u次: 花费：%u ms\n",
    //    nworks, rounds, ntimes, free_costtime);
    //printf("%u个线程并发concurrent alloc&dealloc %u次，总计花费：%u ms\n",
    //    nworks, nworks * rounds * ntimes, malloc_costtime + free_costtime);

    printf("%u个线程并发执行%u轮次，每轮次malloc %u次:",
        nworks, rounds, ntimes);
    std::cout << " 花费：" << malloc_costtime << "ms" << std::endl;
    printf("%u个线程并发执行%u轮次，每轮次malloc %u次:",
        nworks, rounds, ntimes);
    std::cout << " 花费：" << free_costtime << "ms" << std::endl;
    printf("%u个线程并发执行%u轮次，每轮次malloc %u次:",
        nworks, rounds, ntimes);
    std::cout << " 花费：" << malloc_costtime + free_costtime << "ms" << std::endl;
}

void TLSTest() {

    std::thread t1([]() {
        for (int i = 0; i < 5; i++) {
            void* ptr = ccmalloc(8);
            std::cout << std::this_thread::get_id() << "  " << pThreadCache << std::endl;
        }
        });

    //std::thread t2([]() {
    //    for (int i = 0; i < 5; i++) {
    //        void* ptr = ccmalloc(8);
    //        std::cout << std::this_thread::get_id() << "  "<< pThreadCache << std::endl;
    //    }
    //    });

    t1.join();
    //t2.join();
}


void TestAlloc1() {
    
    void* p1 = ccmalloc(6);
    Span* sp = PageCache::GetInstance()->_pageid_span_map[(PAGE_ID)p1 >> 13];
    void* p2 = ccmalloc(6);
    void* p3 = ccmalloc(6);
    void* p4 = ccmalloc(6);
    void* p5 = ccmalloc(6);
    ccfree(p1);
    ccfree(p2);
    ccfree(p3);
    ccfree(p4);
    ccfree(p5);
}

void TestBigAlloc() {
    void* p1 = ccmalloc(257 * 1024);
    void* p11 = ccmalloc(257 * 1024);
    void* p12 = ccmalloc(257 * 1024);
    void* p2 = ccmalloc(129 * 8 * 1024);
    void* p3 = ccmalloc(129 * 8 * 1024);
    void* p4 = ccmalloc(129 * 8 * 1024);
    ccfree(p1);
    ccfree(p11);
    ccfree(p12);
    ccfree(p2);
    ccfree(p3);
    ccfree(p4);
}
//int main()
//{
//    //TLSTest();
//    TestAlloc1();
//    return 0;
//}

//int main()
//{
//    TestBigAlloc();
//}
int main()
{
    size_t n = 10000;
    std::cout << "==========================================================" << std::endl;
    BenchmarkConcurrentMalloc(n, 10, 20);
    std::cout << std::endl << std::endl;
    BenchmarkMalloc(n, 10, 20);
    std::cout << "==========================================================" << std::endl;
    return 0;
}