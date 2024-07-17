#pragma once
#include <assert.h>
#include "Common.hpp"
#include "PageCache.hpp"



class CentralCache
{
private:
	SpanList _spanlists[NUM_LIST];
	PageCache* _pagecache = new PageCache;

	Span* GetOneSpan(SpanList& spanlist, size_t bytes)//size����pagecache����Ƭ
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

		//��pagecacheҪ�ռ�
		Span* span=_pagecache->NewSpan(SizeMap::NumMovePage(bytes));

		//����span��ҳ��ʼ��ַ����ҳ�ռ�
		char* start = (char*)(span->_pageid << PAGE_SHIFT);
		size_t bytes = span->_n << PAGE_SHIFT;
		char* end = start + bytes;
		
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

		spanlist.Insert(spanlist.Begin(), span);
		return span;
	}
public:
	size_t FetchRangeObj(void*& start, void*& end, size_t batchnum, size_t bytes)
	{
		size_t index = SizeMap::Index(bytes);
		_spanlists[index]._mtx.lock();

		Span* span = GetOneSpan(_spanlists[index], batchnum);
		assert(span);
		assert(span->_freelist);

		start = span->_freelist;
		end = start; 
		size_t actualnum = 0; //ʵ�����ṩ�Ŀռ�
		size_t i = 0;

		while (i < batchnum - 1 && end != nullptr)
		{
			end = *(void**)end;
			++i;
			++actualnum;
		}

		_spanlists[index]._mtx.unlock();
		return actualnum;
	}
}; 