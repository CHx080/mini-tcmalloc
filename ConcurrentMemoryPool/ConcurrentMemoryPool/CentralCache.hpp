#pragma once
#include <assert.h>
#include "Common.hpp"
#include "PageCache.hpp"



class CentralCache
{
private:
	SpanList _spanlists[NUM_LIST];
	static PageCache* _pagecache;

	Span* GetOneSpan(SpanList& spanlist, size_t bytes)//bytes����pagecache����Ƭ
	{
		Span* cur = spanlist.Begin();
		while (cur != spanlist.End())
		{
			if (cur->_freelist)
			{
				return cur;
			}
			cur = cur->_next;
		}
		spanlist._mtx.unlock(); //��Ͱ�������������߳��ͷſռ�

		//��pagecacheҪ�ռ�
		_pagecache->_mtx.lock();
		Span* span=_pagecache->NewSpan(SizeMap::NumMovePage(bytes));
		_pagecache->_mtx.unlock();

		//����span��ҳ��ʼ��ַ����ҳ�ռ�
		char* start = (char*)(span->_pageid << PAGE_SHIFT);
		size_t sum_bytes = span->_n << PAGE_SHIFT;
		char* end = start + sum_bytes;
		
		//�з�,����β�壬��Ȼ���������ӣ�����洢ȴ��������
		span->_freelist = (void*)start;
		start += bytes;
		void* tail = span->_freelist;
		while (start<end)
		{
			*(void**)tail = start;
			tail = *(void**)tail;
			start += bytes;
		}

		spanlist._mtx.lock();
		spanlist.Insert(spanlist.Begin(), span);

		return span;
	}
public:
	size_t FetchRangeObj(void*& start, void*& end, size_t batchnum, size_t bytes)
	{
		size_t index = SizeMap::Index(bytes);
		
		_spanlists[index]._mtx.lock();
		Span* span = GetOneSpan(_spanlists[index], bytes);
		assert(span);
		assert(span->_freelist);

		start = span->_freelist;
		end = start; 
		size_t actualnum = 1; //ʵ�����ṩ�Ŀռ�
		size_t i = 0;
		
		while (i < batchnum-1 && *(void**)end != nullptr) //�ߵ����һ��ռ䣬���ߵ���
		{
			end = *(void**)end;
			++i;
			++actualnum;
		}
		
		span->_freelist = *(void**)end;
		*(void**)end = nullptr;

		_spanlists[index]._mtx.unlock();
		
		return actualnum;
	}
}; 
PageCache* CentralCache::_pagecache = new PageCache;



















//�������Ƚϴ�Сʱȷ��������ͬ����ʽ��������᲻����Ԥ�ڣ�(int)-1<(size_t)0�����false
		//cout << ((int)-1 < (size_t)0) << endl;