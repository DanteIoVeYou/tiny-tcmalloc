#define _CRT_SECURE_NO_WARNINGS 1
#include "PageCache.h"
PageCache PageCache::_instance;

Span* PageCache::NewSpan(size_t kpage) {
	// 0.kpage���ڵ���MAX_PAGEҳ��ʱ��ֱ���������
	if (kpage >= MAX_PAGE) {
		Span* new_span = ObjPool.New();
		void* ptr = SystemAlloc(kpage);
		new_span->_page_amount = kpage;
		new_span->_page_id = ((PAGE_ID)ptr >> PAGE_SHIFT);
		_pageid_span_map[new_span->_page_id] = new_span;
		return new_span;
	}
	// ��PageCache��ȡkpage��Span
	// 1. PageCache��kpage�±��Ͱ�������ֳɵ�Span��ֱ�ӷ���
	if (!_span_lists[kpage].Empty()) {
		Span* kspan = _span_lists[kpage].PopFront();
		for (size_t i = 0; i < kspan->_page_amount; i++) {
			_pageid_span_map[kspan->_page_id + i] = kspan;
		}
		kspan->_is_used = true;
		return kspan;
	}
	// 2. PageCache��kpage�±��Ͱ����û���ֳɵ�Span����������
	for (size_t i = kpage + 1; i < MAX_PAGE; i++) {
		if (!_span_lists[i].Empty()) {
			// �ҵ��˷ǿյ�Ͱ�������з֣���iҳ�зֳ� kpage + i-kpage
			Span* nspan = _span_lists[i].PopFront(); // �ҵ����ֳɵ�Span
			Span* kspan = ObjPool.New(); // �г������ص�Span
			kspan->_page_id = nspan->_page_id;
			kspan->_page_amount = kpage;
			nspan->_page_id += kpage;
			nspan->_page_amount -= kpage;
			_pageid_span_map[nspan->_page_id] = nspan;
			_pageid_span_map[nspan->_page_id + nspan->_page_amount - 1] = nspan;
			for (size_t i = 0; i < kspan->_page_amount; i++) {
				_pageid_span_map[kspan->_page_id + i] = kspan;
			}
			_span_lists[nspan->_page_amount].PushFront(nspan);
			kspan->_is_used = true;
			return kspan;
		}
	}
	// 3. �Ҳ����ֳɵ�Span����ϵͳ����MAX_PAGE��С��Span
	void* ptr = SystemAlloc(MAX_PAGE - 1);
	PAGE_ID page_id = ((PAGE_ID)ptr >> PAGE_SHIFT);
	Span* kspan = ObjPool.New();
	kspan->_page_id = page_id;
	kspan->_page_amount = MAX_PAGE - 1;
	_span_lists[MAX_PAGE - 1].PushFront(kspan);

	return NewSpan(kpage);
}

Span* PageCache::GetSpanViaAddress(void* start) {
	PAGE_ID page_id = ((PAGE_ID)start >> PAGE_SHIFT);
	Span* span = _pageid_span_map[page_id];
	return span;
}

void PageCache::GetPageFromCentralCacheOrBigSpan(Span* span) {
	// ����128ҳֱ�ӻ�����
	if (span->_page_amount > MAX_PAGE - 1) {
		void* ptr = (void*)(span->_page_id << PAGE_SHIFT);
		SystemFree(ptr);
		ObjPool.Delete(span);
		return;
	}

	// ǰ��ҳ�ϲ�
	while (1) {
		PAGE_ID prev_page_id = span->_page_id - 1;
		auto ret = _pageid_span_map.find(prev_page_id);
		// 1. û���ҵ�ҳ�ţ����ϲ�
		if (ret == _pageid_span_map.end()) {
			break;
		}
		Span* prev_span = _pageid_span_map[prev_page_id];
		// 2. ǰ��ϲ�����128ҳ�����ϲ�
		if (span->_page_amount + prev_span->_page_amount > 128) {
			break;
		}
		// 3. ǰ��һ��span����ʹ�ã����ϲ�
		if (prev_span->_is_used == true) {
			break;
		}
		// 4. ǰ��ϲ�С�ڵ�����128ҳ���ϲ�
		span->_page_amount += prev_span->_page_amount;
		span->_page_id = prev_span->_page_id;

		// �ɵ�ǰһ��span
		_span_lists[prev_span->_page_amount].Erase(prev_span);
		ObjPool.Delete(prev_span);
	}
	while (1) {
		PAGE_ID next_page_id = span->_page_id + span->_page_amount;
		auto ret = _pageid_span_map.find(next_page_id);
		if (ret == _pageid_span_map.end()) {
			break;
		}
		Span* next_span = _pageid_span_map[next_page_id];
		if (span->_page_amount + next_span->_page_amount > 128) {
			break;
		}
		if (next_span->_is_used == true) {
			break;
		}
		span->_page_amount += next_span->_page_amount;

		_span_lists[next_span->_page_amount].Erase(next_span);
		ObjPool.Delete(next_span);
	}

	// �Ѻϲ���ҳ���뵽PageCache�Ĺ�ϣͰ����
	_span_lists[span->_page_amount - 1].PushFront(span);
	span->_is_used = false;

	_pageid_span_map[span->_page_id] = span;
	_pageid_span_map[span->_page_id + span->_page_amount - 1] = span;
}
