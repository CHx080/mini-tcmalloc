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
	size_t actualnum = 1; //ʵ�����ṩ�Ŀռ�
	size_t i = 0;

	while (i < batchnum - 1 && *(void**)end != nullptr) //�ߵ����һ��ռ䣬���ߵ���
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

void CentralCache::RecoverFromThreadCache(void* start, size_t bytes) //ÿһС��ռ���Ҫ�ҵ����Ӧ��span
{
	size_t index = SizeMap::Index(bytes);
	_spanlists[index]._mtx.lock();
	//���ݵ�ַ�������Ӧ��ҳ�ź�ѿռ�����ָ����span�����
	while (start)
	{
		void* next = *(void**)start;
		Span* span = PageCache::GetInstance()->ConvertToSpanAdd(start); //��ַ--->ҳ��--->span
		*(void**)start = span->_freelist;
		span->_freelist = start;
		span->_useCount--;


		if (span->_useCount == 0) //��һ����span����pagecache
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

Span* CentralCache::GetOneSpan(SpanList& spanlist, size_t bytes)//bytes����pagecache����Ƭ
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
	PageCache::GetInstance()->_mtx.lock();
	Span* span = PageCache::GetInstance()->NewSpan(SizeMap::NumMovePage(bytes));
	span->_isuse = true;
	PageCache::GetInstance()->_mtx.unlock();

	//����span��ҳ��ʼ��ַ����ҳ�ռ�
	char* start = (char*)(span->_pageid << PAGE_SHIFT);
	size_t sum_bytes = span->_n << PAGE_SHIFT;
	char* end = start + sum_bytes;

	//�з�,����β�壬��Ȼ���������ӣ�����洢ȴ��������
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
