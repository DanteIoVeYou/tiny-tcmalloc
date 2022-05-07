#pragma once
#include "common.h"

//�����ڴ��
//1. ����Ĵ���ڴ�
//2. ������������ͷŵ��ڴ�

#ifdef _WIN32
#include <windows.h>
#endif

template<class T>
class ObjectPool {
public:
    ObjectPool() : _memory(nullptr), _free_list(nullptr), _remain_size(0) {}
    ~ObjectPool() {}
    T* New() {
        // ��ʼ��
        T* obj = nullptr;
        // ���ȴ���������ȡ�ڴ��
        if (_free_list) {
            void* next = *((void**)_free_list);
            obj = (T*)_free_list;
            _free_list = next;
        }
        // ����������û���ڴ��
        else {
            // �ڴ��û�г�ʼ�������ڴ�ش�С���������¿�����ڴ�
            if (_remain_size < sizeof(T)) {
                //                 _memory = (char*)malloc(INIT_POOL_SIZE);
                _memory = (char*)SystemAlloc(INIT_POOL_PAGE); // ֱ�ӵ���ϵͳ�ӿ�VirtualAlloc
                _remain_size = INIT_POOL_SIZE;
                // mallocʧ��
                if (!_memory) {
                    throw std::bad_alloc();
                }
            }
            // �д���ڴ�����ʹ��

            // ������32λ�¿�4���ֽڣ���64λ�¿�8���ֽڣ����ܴ����ָ��
            size_t size = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
            obj = (T*)_memory;
            _memory += size;
            _remain_size -= size;
        }
        // placement-new ��T��ʼ��
        new(obj)T;
        return obj;
    }
    void Delete(T* obj) {
        // ��ʾ����������������
        obj->~T();
        // ��һ���ڴ��ͷ�嵽����������
        *((void**)obj) = _free_list;
        _free_list = obj;
    }

private:
    size_t _remain_size; // ����ڴ�ʣ��Ĵ�С
    char* _memory; // ����ڴ����ʼ��ַ
    void* _free_list; // ��������ͷ�ڵ�ĵ�ַ
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
//    // �����ͷŵ��ִ�
//    const size_t Rounds = 3;
//    // ÿ�������ͷŶ��ٴ�
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