#pragma once
#include "common.h"

//定长内存池
//1. 申请的大块内存
//2. 自由链表回收释放的内存

#ifdef _WIN32
#include <windows.h>
#endif

template<class T>
class ObjectPool {
public:
    ObjectPool() : _memory(nullptr), _free_list(nullptr), _remain_size(0) {}
    ~ObjectPool() {}
    T* New() {
        // 初始化
        T* obj = nullptr;
        // 优先从自由链表取内存块
        if (_free_list) {
            void* next = *((void**)_free_list);
            obj = (T*)_free_list;
            _free_list = next;
        }
        // 自由链表上没挂内存块
        else {
            // 内存池没有初始化或者内存池大小不够，就新开大块内存
            if (_remain_size < sizeof(T)) {
                //                 _memory = (char*)malloc(INIT_POOL_SIZE);
                _memory = (char*)SystemAlloc(INIT_POOL_PAGE); // 直接调用系统接口VirtualAlloc
                _remain_size = INIT_POOL_SIZE;
                // malloc失败
                if (!_memory) {
                    throw std::bad_alloc();
                }
            }
            // 有大块内存来供使用

            // 至少在32位下开4个字节，在64位下开8个字节，才能存的下指针
            size_t size = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
            obj = (T*)_memory;
            _memory += size;
            _remain_size -= size;
        }
        // placement-new 对T初始化
        new(obj)T;
        return obj;
    }
    void Delete(T* obj) {
        // 显示调用析构函数清理
        obj->~T();
        // 将一个内存块头插到自由链表上
        *((void**)obj) = _free_list;
        _free_list = obj;
    }

private:
    size_t _remain_size; // 大块内存剩余的大小
    char* _memory; // 大块内存的起始地址
    void* _free_list; // 自由链表头节点的地址
};

//struct TreeNode {
//    int _val;
//    TreeNode* _left;
//    TreeNode* _right;
//    TreeNode() : _val(0), _left(nullptr), _right(nullptr) {}
//    ~TreeNode() {}
//};
//
//void Test1() {
//    size_t cnt = 100000;
//    std::vector<TreeNode*> v1;
//    v1.reserve(3 * cnt);
//    size_t begin1 = clock();
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < cnt; j++) {
//            v1.push_back(new TreeNode);
//        }
//    }
//    for (int i = 0; i < v1.size(); i++) {
//        delete v1[i];
//    }
//    size_t end1 = clock();
//    std::cout << "new time: " << end1 - begin1 << std::endl;
//
//    std::vector<TreeNode*> v2;
//
//    v2.reserve(3 * cnt);
//    ObjectPool<TreeNode> pool;
//    size_t begin2 = clock();
//    for (int i = 0; i < 3; i++) {
//        for (int j = 0; j < cnt; j++) {
//            v2.push_back(pool.New());
//        }
//    }
//    for (int i = 0; i < v2.size(); i++) {
//        pool.Delete(v2[i]);
//    }
//    size_t end2 = clock();
//    std::cout << "ObjPool time: " << end2 - begin2 << std::endl;
//}
//void TestObjectPool()
//{
//    // 申请释放的轮次
//    const size_t Rounds = 3;
//    // 每轮申请释放多少次
//    const size_t N = 10000;
//    size_t begin1 = clock();
//    std::vector<TreeNode*> v1;
//    v1.reserve(N);
//    for (size_t j = 0; j < Rounds; ++j)
//    {
//        for (int i = 0; i < N; ++i)
//        {
//            v1.push_back(new TreeNode);
//        }
//        for (int i = 0; i < N; ++i)
//        {
//            delete v1[i];
//        }
//        v1.clear();
//    }
//    size_t end1 = clock();
//    ObjectPool<TreeNode> TNPool;
//    size_t begin2 = clock();
//    std::vector<TreeNode*> v2;
//    v2.reserve(N);
//    for (size_t j = 0; j < Rounds; ++j) {
//        for (int i = 0; i < N; ++i)
//        {
//            v2.push_back(TNPool.New());
//        }
//        for (int i = 0; i < N; ++i)
//        {
//            TNPool.Delete(v2[i]);
//        }
//        v2.clear();
//    }
//    size_t end2 = clock();
//    std::cout << "new cost time:" << end1 - begin1 << std::endl;
//    std::cout << "object pool cost time:" << end2 - begin2 << std::endl;
//}