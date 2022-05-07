#pragma once

#include "common.h"
#include "ObjectPool.h"

class PageCache {
public:
	static PageCache* GetInstance() {
		return &_instance;
	}
	Span* NewSpan(size_t kpage);
	Span* GetSpanViaAddress(void* start);
	void GetPageFromCentralCacheOrBigSpan(Span* span);
	std::mutex _mtx;
	ObjectPool<Span> ObjPool;
	std::unordered_map<PAGE_ID, Span*> _pageid_span_map;
private:
	PageCache() {}
	PageCache(const PageCache&) = delete;
	PageCache operator=(const PageCache&) = delete;
	static PageCache _instance;
	SpanList _span_lists[MAX_PAGE];
};