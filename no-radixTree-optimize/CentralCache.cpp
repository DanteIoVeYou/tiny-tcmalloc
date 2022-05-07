#define _CRT_SECURE_NO_WARNINGS 1
#include "CentralCache.h"
#include "PageCache.h"
CentralCache CentralCache::_instance; // ���嵥��

Span* CentralCache::GetOneSpan(SpanList& span_list, size_t size) {
	// ȥCentralCache��Ӧ��SpanList�����ҷǿ�Span���ҵ����ظ�Span�ĵ�ַ���Ҳ�����ȥ��PageCache�����µ�Span
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
	// ��PageCache������ҳΪ��λ���ڴ棬���ҽ����и�
	span_list._mtx.unlock(); // ��ʱ��Ͱ���������Ա����ڴ滹�����Ļ��������߳̿���ֱ���û��������ڴ�

	PageCache::GetInstance()->_mtx.lock();
	Span* new_span = PageCache::GetInstance()->NewSpan(SizeClass::PageBatch(size));
	//new_span->_is_used = true;
	new_span->_page_block_size = size; // ��Span������ڴ���Сλsize

	PageCache::GetInstance()->_mtx.unlock();


	char* addr_begin = (char*)((new_span->_page_id) << PAGE_SHIFT);
	char* addr_end = (char*)((new_span->_page_id + new_span->_page_amount) << PAGE_SHIFT);

	new_span->_free_list = addr_begin;
	void* tail = new_span->_free_list;

	//����д��
	//while (addr_begin < addr_end) {
	//	NextObj(tail) = (addr_begin + size); //����д�����һ��tail�����һҳ���׵�ַ��������ÿվͻ�Խ��
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
	// ������һ��Span��Ҳ����ζ��Span����������һ���ڴ��
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
	// start ָ����һ��������Pop�������ڴ��
	// ��ThreadCache����num��size��С���ڴ��
	size_t index = SizeClass::Index(size);
	_span_lists[index]._mtx.lock();

	// ��start��Ӧ���ڴ�黹��ָ����span
	while (start) {
		void* next = NextObj(start);
		Span* span = PageCache::GetInstance()->GetSpanViaAddress(start); // ������Ҫ֪��������һ��Span
		NextObj(start) = span->_free_list;
		span->_free_list = start;
		span->_used_amount--;

		// һ��Span�Ѿ�ȫ�����˻�������ô�黹��PageCache
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
