#pragma once
#include "Common.hpp"

constexpr size_t NUM_FREELIST = 208;

class ThreadCache
{
private:
	FreeList _freelists[NUM_FREELIST];
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

	void* FetchFromCentralCache(size_t index,size_t align)
	{
		return nullptr;
	}
};

static __declspec(thread) ThreadCache* TLSthreadcache = nullptr;