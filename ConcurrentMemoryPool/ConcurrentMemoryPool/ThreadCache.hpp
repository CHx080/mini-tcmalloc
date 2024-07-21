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
		assert(bytes>0 && bytes <= MAX_BYTES);
		size_t align = SizeMap::RoundUp(bytes);
		size_t index = SizeMap::Index(align);
		_freelists[index].Push(p);

		//如果此时_freelists[index]的自由链表长度达到maxsize，将一部分归还给centralcache
		if (_freelists[index].Size() > _freelists[index].Maxsize())
		{
			void* start=SolveListTooLong(_freelists[index], _freelists[index].Maxsize());
			_centralcache->RecoverFromThreadCache(start, align);
		}
	}
private:
	void* SolveListTooLong(FreeList& freelist,size_t n)//n标识归还多少
	{
		assert(n <= freelist.Size());
		void* start = freelist.PeekHead(), * end = freelist.PeekHead();
		for (int i = 0; i < n - 1; ++i)
		{
			end = *(void**)end;
		}
		freelist.PopRange(start, end, n);
		*(void**)end = nullptr;

		return start;
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
			_freelists[index].PushRange(temp, end,actualnum-1);
			//把多的空间暂存入线程所拥有的自由链表中
		}

		
		return start;
	}
	
	
};

static __declspec(thread) ThreadCache* TLSthreadcache = nullptr; //声明每一个线程独占一个threadcache
CentralCache* ThreadCache::_centralcache = new CentralCache;