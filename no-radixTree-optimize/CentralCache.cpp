#define _CRT_SECURE_NO_WARNINGS 1
#include "CentralCache.h"
#include "PageCache.h"
CentralCache CentralCache::_instance; // 定义单例

Span* CentralCache::GetOneSpan(SpanList& span_list, size_t size) {
	// 去CentralCache对应的SpanList里面找非空Span；找到返回该Span的地址；找不到则去向PageCache申请新的Span
	Span* begin = span_list.Begin();
	Span* end = span_list.End();
	while (begin != end) {
		if (begin->_free_list != nullptr) {
			return begin;
		}
		else {
			begin = begin->_next;
		}
	}
	// 向PageCache申请以页为单位的内存，并且进行切割
	span_list._mtx.unlock(); // 暂时将桶锁解锁，以便有内存还回来的话，其他线程可以直接拿还回来的内存

	PageCache::GetInstance()->_mtx.lock();
	Span* new_span = PageCache::GetInstance()->NewSpan(SizeClass::PageBatch(size));
	//new_span->_is_used = true;
	new_span->_page_block_size = size; // 该Span里面的内存块大小位size

	PageCache::GetInstance()->_mtx.unlock();


	char* addr_begin = (char*)((new_span->_page_id) << PAGE_SHIFT);
	char* addr_end = (char*)((new_span->_page_id + new_span->_page_amount) << PAGE_SHIFT);

	new_span->_free_list = addr_begin;
	void* tail = new_span->_free_list;

	//错误写法
	//while (addr_begin < addr_end) {
	//	NextObj(tail) = (addr_begin + size); //这样写，最后一步tail会存下一页的首地址，下面的置空就会越界
	//	tail = NextObj(tail);
	//	addr_begin += size;
	//}


	addr_begin += size;
	while (addr_begin < addr_end)
	{
		NextObj(tail) = addr_begin;
		tail = NextObj(tail); // tail = start;
		addr_begin += size;
	}
	NextObj(tail) = nullptr;
	span_list._mtx.lock();

	span_list.PushFront(new_span);
	return new_span;

}
size_t CentralCache::GiveToThreadCache(void*& start, void*& end, size_t batch_size, size_t size) {
	size_t index = SizeClass::Index(size);
	_span_lists[index]._mtx.lock();
	Span* span = GetOneSpan(_span_lists[index], size);
	assert(span);
	assert(span->_free_list);
	// 至少有一个Span，也就意味着Span里面至少有一个内存块
	start = span->_free_list;
	end = start;
	size_t actual_batch_size = 1;
	if (NextObj(start) == nullptr) {
		int x = 0;
	}
	while (actual_batch_size < batch_size && NextObj(end) != nullptr) {
		actual_batch_size++;
		end = NextObj(end);
	}
	if (NextObj(start) == nullptr && actual_batch_size != 1) {
		int x = 0;
	}
	span->_free_list = NextObj(end);
	span->_used_amount += actual_batch_size;
	NextObj(end) = nullptr;

	_span_lists[index]._mtx.unlock();

	if (NextObj(start) == nullptr && actual_batch_size != 1) {
		int x = 0;
	}
	return actual_batch_size;
}

void CentralCache::GetBackFromThreadCache(void* start, size_t size) {
	// start 指向了一批连续的Pop出来的内存块
	// 从ThreadCache回收num个size大小的内存块
	size_t index = SizeClass::Index(size);
	_span_lists[index]._mtx.lock();

	// 把start对应的内存块还给指定的span
	while (start) {
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->GetSpanViaAddress(start); // 我们需要知道还给哪一个Span
		NextObj(start) = span->_free_list;
		span->_free_list = start;
		span->_used_amount--;

		// 一个Span已经全部还了回来，那么归还给PageCache
		if (span->_used_amount == 0) {
			_span_lists[index].Erase(span);
			_span_lists[index]._mtx.unlock();
			span->_free_list = nullptr;
			span->_next = nullptr;
			span->_prev = nullptr;
			//span->_is_used = false;
			PageCache::GetInstance()->_mtx.lock();
			PageCache::GetInstance()->GetPageFromCentralCacheOrBigSpan(span);
			PageCache::GetInstance()->_mtx.unlock();
			_span_lists[index]._mtx.lock();
		}
		start = next;
	}
	_span_lists[index]._mtx.unlock();
}
