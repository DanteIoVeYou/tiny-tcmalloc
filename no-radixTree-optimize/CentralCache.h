#pragma once
#include "common.h"
class CentralCache {
public:
	static CentralCache* GetInstance() {
		return &_instance;
	}
	size_t GiveToThreadCache(void*& start, void*& end, size_t batch_size, size_t size);
	Span* GetOneSpan(SpanList& span_list, size_t size);
	void GetBackFromThreadCache(void* start, size_t size);
private:
	CentralCache() {}
	CentralCache(const CentralCache&) = delete;
	CentralCache operator=(const CentralCache&) = delete;
	static CentralCache _instance;
	SpanList _span_lists[BUCKET_AMOUNT];
};


