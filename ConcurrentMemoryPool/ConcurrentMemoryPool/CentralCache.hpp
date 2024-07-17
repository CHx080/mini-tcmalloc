#pragma once
#include <assert.h>
#include <mutex>
#include "Common.hpp"
#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#endif

struct Span //����
{
	PAGE_ID _pageid=0; //����ڴ�ҳ��
	size_t _n=0; //ҳ������
	Span* _next=nullptr;
	Span* _prev=nullptr;

	size_t _useCount=0; //�����threadcache�ĸ���
	void* _freelist=nullptr; //��������
};

class SpanList
{
private:
	Span* _head=nullptr; //�ڱ�λ
	
public:
	std::mutex _mtx; //Ͱ��

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

		//����Ҫdelete cur����cur������һ�㣬����ϵͳ
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
		size_t actualnum = 0; //ʵ�����ṩ�Ŀռ�
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