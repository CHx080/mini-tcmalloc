#include "CentralCache.h"
size_t CentralCache::FetchRangeObj(void*& start, void*& end, size_t batchnum, size_t bytes)
{
	size_t index = SizeMap::Index(bytes);

	_spanlists[index]._mtx.lock();
	Span* span = GetOneSpan(_spanlists[index], bytes);
	span->_objsize = bytes;
	assert(span);
	assert(span->_freelist);

	start = span->_freelist;
	end = start;
	size_t actualnum = 1; //实际能提供的空间
	size_t i = 0;

	while (i < batchnum - 1 && *(void**)end != nullptr) //走到最后一块空间，不走到空
	{
		end = *(void**)end;
		++i;
		++actualnum;
	}

	span->_freelist = *(void**)end;
	*(void**)end = nullptr;
	span->_useCount++;
	_spanlists[index]._mtx.unlock();
	return actualnum;
}

void CentralCache::RecoverFromThreadCache(void* start, size_t bytes) //每一小块空间需要找到其对应的span
{
	size_t index = SizeMap::Index(bytes);
	_spanlists[index]._mtx.lock();
	//根据地址计算出相应的页号后把空间块放入指定的span跨度中
	while (start)
	{
		void* next = *(void**)start;
		Span* span = PageCache::GetInstance()->ConvertToSpanAdd(start); //地址--->页号--->span
		*(void**)start = span->_freelist;
		span->_freelist = start;
		span->_useCount--;


		if (span->_useCount == 0) //将一整个span还给pagecache
		{
			span->_freelist = nullptr;
			span->_prev = nullptr;
			span->_next = nullptr;
			span->_isuse = false;
			_spanlists[index]._mtx.unlock();
			PageCache::GetInstance()->RecoverFromCentralCache(span);
			_spanlists[index]._mtx.lock();
		}

		start = next;
	}
	_spanlists[index]._mtx.unlock();
}

Span* CentralCache::GetOneSpan(SpanList& spanlist, size_t bytes)//bytes用于pagecache的切片
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
	PageCache::GetInstance()->_mtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeMap::NumMovePage(bytes));
	span->_isuse = true;
	PageCache::GetInstance()->_mtx.unlock();

	//计算span的页起始地址和总页空间
	char* start = (char*)(span->_pageid << PAGE_SHIFT);
	size_t sum_bytes = span->_n << PAGE_SHIFT;
	char* end = start + sum_bytes;

	//切分,采用尾插，虽然是链表连接，物理存储却是连续的
	span->_freelist = (void*)start;
	start += bytes;
	void* tail = span->_freelist;
	while (start < end)
	{
		*(void**)tail = start;
		tail = *(void**)tail;
		start += bytes;
	}
	*(void**)tail = nullptr;

	spanlist._mtx.lock();
	spanlist.Insert(spanlist.Begin(), span);

	return span;
}

CentralCache* CentralCache::GetInstance()
{
	return self;
}

CentralCache* CentralCache::self = new CentralCache;
