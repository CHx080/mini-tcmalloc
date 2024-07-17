#pragma once
#include "CentralCache.hpp"


class ThreadCache
{
private:
	FreeList _freelists[NUM_LIST];
	CentralCache* _centralcache = new CentralCache;
public:
	void* Allocate(size_t bytes)
	{
		assert(bytes <= MAX_BYTES); //所申请的字节数必须小于256KB
		size_t align = SizeMap::RoundUp(bytes);
		size_t index = SizeMap::Index(bytes);

		if (!_freelists[index].IsEmpty())
		{
			return _freelists->Pop();
		}
		else
		{
			return FetchFromCentralCache(index, align);
		}
		//如果对应的自由链表有空间直接取，不够从中心缓存中申请
	}

	void Deallocate(void* p,size_t bytes)
	{
		assert(p);
		assert(bytes <= MAX_BYTES);
		size_t index = SizeMap::Index(bytes);
		_freelists[index].Push(p);
	}

	void* FetchFromCentralCache(size_t index,size_t bytes)
	{
	
		/*慢开始调节算法，小对象多给，大对象少给
		最开始不会一次向中心缓存申请太多*/
		size_t batchnum = std::min(_freelists[index].Maxsize(), SizeMap::NumMoveSize(bytes));

		if (_freelists[index].Maxsize() == batchnum)
		{
			_freelists[index].Maxsize() += 2;
		}

		//试图从从中心缓存中截取一段空间
		void* start = nullptr, *end = nullptr;//作为输出型参数
		size_t actualnum = _centralcache->FetchRangeObj(start, end, batchnum, bytes);
		assert(actualnum > 1);
		if (actualnum == 1)
		{
			assert(end == start);
			return start;
		}
		else
		{
			void* temp = *(void**)start;
			while (temp != end)
			{
				_freelists[index].Push(temp);
				temp = *(void**)temp;
			}
			_freelists[index].Push(end); //把多的空间暂存入线程所拥有的自由链表中
			return start;
		}

	}
};

static __declspec(thread) ThreadCache* TLSthreadcache = nullptr;