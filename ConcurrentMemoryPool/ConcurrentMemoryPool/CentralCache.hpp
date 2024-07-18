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
		spanlist._mtx.lock();
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
		size_t bytes = span->_n << PAGE_SHIFT;
		char* end = start + bytes;
		
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
		spanlist._mtx.unlock();
		return span;
	}
public:
	size_t FetchRangeObj(void*& start, void*& end, size_t batchnum, size_t bytes)
	{
		size_t index = SizeMap::Index(bytes);
		

		Span* span = GetOneSpan(_spanlists[index], batchnum);
		assert(span);
		assert(span->_freelist);

		start = span->_freelist;
		end = start; 
		size_t actualnum = 0; //实际能提供的空间
		size_t i = 0;

		while (i < batchnum - 1 && end != nullptr)
		{
			end = *(void**)end;
			++i;
			++actualnum;
		}

		
		return actualnum;
	}
}; 
PageCache* CentralCache::_pagecache = new PageCache;