#pragma once
#include <assert.h>
#include <mutex>
#include "ObjectPool.hpp"
#include <iostream>

#ifdef _WIN64
typedef unsigned long long PAGE_ID;
#elif _WIN32
typedef size_t PAGE_ID;
#endif

constexpr  size_t MAX_BYTES = 256 * 1024;  //小于256KB的找ThreadCache要
constexpr  size_t NUM_LIST = 208;
constexpr  size_t NPAGES = 129; //1024kb 129-1
constexpr  size_t PAGE_SHIFT = 13; //1页大小为8K 


class FreeList //管理切分好的块空间
{
public:
	void Push(void* obj)//头插
	{
		assert(obj);//不能为空
		*(void**)obj = _freelist;
		_freelist = obj;
		++_size;
	}
	void PushRange(void* start, void* end,size_t n)
	{
		*(void**)end = _freelist;
		_freelist = start; //控制头尾即可
		_size += n;
	}
	void* Pop()//头删
	{
		void* obj = _freelist;
		_freelist = *(void**)obj;
		--_size;
		return obj;
	}

	void PopRange(void* end, size_t n)
	{
		_freelist = *(void**)end;
		_size -= n;
	}

	bool IsEmpty()
	{
		return _freelist == nullptr;
	}

	size_t& Maxsize()
	{
		return _maxsize;
	}

	size_t Size()
	{
		return _size;
	}

	void* PeekHead()
	{
		return _freelist;
	}
private:
	void* _freelist = nullptr; //自由链表的头指针
	size_t _size = 0; //自由链表的长度
	size_t _maxsize = 1; //与慢开始算法相关
};

struct Span //管理
{
	PAGE_ID _pageid = 0; //大块内存页码
	size_t _n = 0; //页的数量
	Span* _next = nullptr;
	Span* _prev = nullptr;

	size_t _useCount = 0; //分配的threadcache的个数
	void* _freelist = nullptr; //自由链表

	bool _isuse = false; //是否在被使用
	size_t _objsize = 0; //切好的块大小
};

class SpanList
{
private:
	Span* _head = nullptr; //哨兵位

public:
	std::mutex _mtx; //桶锁

	bool IsEmpty()
	{
		return _head->_next == _head;
	}

	SpanList()
	{
		_head = new Span;
		_head->_next = _head;
		_head->_prev = _head;
	}
	
	Span* Begin()
	{
		return _head->_next;
	}

	Span* End()
	{
		return _head;
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
		cur->_prev->_next = cur->_next;
		cur->_next->_prev = cur->_prev;

		//不需要delete cur，将cur交给下一层，不是系统
	}

	Span* Front()
	{
		Span* span= _head->_next;
		Erase(span);
		return span;
	}
};

class SizeMap
{
	// [1,128]				8byte对⻬ freelist[0,16)
	// [128+1,1024]			16byte对⻬ freelist[16,72)
	// [1024+1,81024]		128byte对⻬ freelist[72,128)
	// [8*1024+1,641024]	1024byte对⻬ freelist[128,184)
	// [64*1024+1,256*1024] 8*1024byte对⻬ freelist[184,208)
	// 所申请的空间大小必须是相应区间内对齐数的倍数
	// 每一个对齐数所对应的自由链表桶存放存放其倍数大小的空间
	//例如freelist[0]表示每一个节点为指向8字节空间指针的链表，freelist[1]则为16字节空间
private:
	static size_t _RoundUp(size_t bytes, size_t align)
	{
		return ((bytes + align - 1) & ~(align - 1));
	}
	static size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + ((size_t)1 << align_shift) - 1) >> align_shift) - 1;
	}
public:
	static size_t RoundUp(size_t bytes)//计算对齐数
	{
		if (bytes <= 128) return _RoundUp(bytes, 8);
		else if (bytes <= 1024) return _RoundUp(bytes, 16);
		else if (bytes <= 8 * 1024) return _RoundUp(bytes, 128);
		else if (bytes <= 64 * 1024) return _RoundUp(bytes, 1024);
		else if (bytes <= 256 * 1024) return _RoundUp(bytes, 8 * 1024);
		else return _RoundUp(bytes, 1 << PAGE_SHIFT);
	}

	static size_t Index(size_t bytes)
	{
		static int group_array[4] = { 16, 56, 56, 56 }; //每一个区间第一个桶的下标
		if (bytes <= 128) {
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 8*1024) {
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8 * 1024, 10) + group_array[2] +
				group_array[1] + group_array[0];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes - 64 * 1024, 13) + group_array[3] +
				group_array[2] + group_array[1] + group_array[0];
		}
		else {
			assert(false);
			return -1;
		}
	}

	static size_t NumMoveSize(size_t bytes)
	{	
		if (bytes == 0)
			return 0;
		// [2, 512]，⼀次批量移动多少个对象的(慢启动)上下限值
		// ⼩对象⼀次批量上限⾼
	
		size_t num = MAX_BYTES / bytes;
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;
		return num;
	}

	static size_t NumMovePage(size_t bytes)
	{
		size_t npage = NumMoveSize(bytes) * bytes;
		npage >>= PAGE_SHIFT;
		if (npage == 0) npage = 1;
		return npage; 
	}
};
