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




















//�������Ƚϴ�Сʱȷ��������ͬ����ʽ��������᲻����Ԥ�ڣ�(int)-1<(size_t)0�����false
		//cout << ((int)-1 < (size_t)0) << endl;