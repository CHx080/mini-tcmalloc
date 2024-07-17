#pragma once
#include <assert.h>
#include <mutex>
#include "Common.hpp"
#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#endif

struct Span //管理
{
	PAGE_ID _pageid=0; //大块内存页码
	size_t _n=0; //页的数量
	Span* _next=nullptr;
	Span* _prev=nullptr;

	size_t _useCount=0; //分配的threadcache的个数
	void* _freelist=nullptr; //自由链表
};

class SpanList
{
private:
	Span* _head=nullptr; //哨兵位
	
public:
	std::mutex _mtx; //桶锁

	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}
	
	void Insert(Span* cur, Span* newSpan)
	{
		assert(cur && newSpan);
		newSpan->_next = cur;
		newSpan->_prev = cur->_prev;
		cur->_prev->_next = newSpan;
		cur->_prev = newSpan;
	}

	void Erase(Span* cur)
	{
		assert(cur && cur != _head);
		cur->_prev = cur->_next;
		cur->_next->_prev = cur->_prev;

		//不需要delete cur，将cur交给下一层，不是系统
	}
};
class CentralCache
{
private:
	SpanList _spanlists[NUM_LIST];
	Span* GetOneSpan(SpanList& list, size_t size)
	{
		// ...
		return nullptr;
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
		size_t actualnum = 0; //实际能提供的空间
		size_t i = 0;

		while (i < batchnum - 1 && end != nullptr)
		{
			end = *(void**)end;
			++i;
			++actualnum;
		}

		_spanlists[index]._mtx.unlock();
	}
}; 