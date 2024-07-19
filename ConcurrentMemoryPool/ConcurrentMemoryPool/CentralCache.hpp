#pragma once
#include <assert.h>
#include "Common.hpp"
#include "PageCache.hpp"



class CentralCache
{
private:
	SpanList _spanlists[NUM_LIST];
	static PageCache* _pagecache;

	Span* GetOneSpan(SpanList& spanlist, size_t bytes)//bytes用于pagecache的切片
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
		spanlist._mtx.unlock(); //解桶锁，方便其他线程释放空间

		//向pagecache要空间
		_pagecache->_mtx.lock();
		Span* span=_pagecache->NewSpan(SizeMap::NumMovePage(bytes));
		_pagecache->_mtx.unlock();

		//计算span的页起始地址和总页空间
		char* start = (char*)(span->_pageid << PAGE_SHIFT);
		size_t sum_bytes = span->_n << PAGE_SHIFT;
		char* end = start + sum_bytes;
		
		//切分,采用尾插，虽然是链表连接，物理存储却是连续的
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
		size_t actualnum = 1; //实际能提供的空间
		size_t i = 0;
		
		while (i < batchnum-1 && *(void**)end != nullptr) //走到最后一块空间，不走到空
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



















//两个数比较大小时确保类型相同，隐式提升结果会不符合预期，(int)-1<(size_t)0结果是false
		//cout << ((int)-1 < (size_t)0) << endl;