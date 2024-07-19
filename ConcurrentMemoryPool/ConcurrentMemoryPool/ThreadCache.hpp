#pragma once
#include "CentralCache.hpp"


class ThreadCache
{
private:
	FreeList _freelists[NUM_LIST];
	static CentralCache* _centralcache;
public:
	void* Allocate(size_t bytes)
	{
		assert(bytes>0 && bytes <= MAX_BYTES); //所申请的字节数必须小于256KB
		size_t align = SizeMap::RoundUp(bytes);
		size_t index = SizeMap::Index(align);
		/*cout << align << endl;
		cout << index << endl;*/
		if (!_freelists[index].IsEmpty())	//对应大小的自由链表中闲置空间，直接获取
		{
			return _freelists->Pop();
		}
		else //自由链表为空，找下一层要
		{
			return FetchFromCentralCache(index, align);
		}
		
	}

	void Deallocate(void* p,size_t bytes)
	{
		assert(p);
		assert(bytes <= MAX_BYTES);
		size_t index = SizeMap::Index(bytes);
		_freelists[index].Push(p);
	}

	void* FetchFromCentralCache(size_t index,size_t bytes) //bytes为经过对齐后的字节数
	{
	
		/*慢开始调节算法，小对象多给，大对象少给,随着申请次数的增多而提高batchnum*/
		size_t batchnum = min(_freelists[index].Maxsize(), SizeMap::NumMoveSize(bytes));

		if (_freelists[index].Maxsize() == batchnum)
		{
			_freelists[index].Maxsize() += 2;
		}

		//试图从从中心缓存中截取一段空间
		void* start = nullptr, *end = nullptr;//作为输出型参数
		size_t actualnum = _centralcache->FetchRangeObj(start, end, batchnum, bytes);


		assert(actualnum >= 1);
		assert(start);
		assert(end);
		
		if (actualnum > 1)
		{
			void* temp = *(void**)start;
			while (temp != nullptr)
			{
				_freelists[index].Push(temp);
				temp = *(void**)temp;
			}
			 //把多的空间暂存入线程所拥有的自由链表中
		}

		
		return start;
	}
};

static __declspec(thread) ThreadCache* TLSthreadcache = nullptr; //声明每一个线程独占一个threadcache
CentralCache* ThreadCache::_centralcache = new CentralCache;