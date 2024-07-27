#pragma once
#include <assert.h>
#include "Common.hpp"
#include "PageCache.h"



class CentralCache
{
private:
	CentralCache() {}
	CentralCache(const CentralCache&) = delete;
	CentralCache& operator=(const CentralCache&) = delete;
private:
	SpanList _spanlists[NUM_LIST];
	
	static CentralCache* self;

	Span* GetOneSpan(SpanList& spanlist, size_t bytes);
public:
	static CentralCache* GetInstance();

	size_t FetchRangeObj(void*& start, void*& end, size_t batchnum, size_t bytes);

	void RecoverFromThreadCache(void* start, size_t bytes);
}; 




















//两个数比较大小时确保类型相同，隐式提升结果会不符合预期，(int)-1<(size_t)0结果是false
		//cout << ((int)-1 < (size_t)0) << endl;