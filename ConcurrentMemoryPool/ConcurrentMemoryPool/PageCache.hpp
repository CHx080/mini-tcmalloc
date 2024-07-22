#pragma once
#include "Common.hpp"
#include <unordered_map>
#ifdef _WIN32
#include <Windows.h>
static void* SystemAlloc(size_t k)
{
	void* p = VirtualAlloc(NULL, k << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return p; //p的地址一定是页大小的整数倍
}
static void SystemFree(void* p)
{
	VirtualFree(p, 0, MEM_RELEASE);
}
#endif

/*pagecache挂的也是span(每一个span大小是页大小的倍数);映射规则不同于上2层，哈希函数更简单(直接定值法)*/
class PageCache
{
private:
	PageCache()
	{}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;
public:
	static PageCache* GetInstance()
	{
		return self;
	}
private:
	ObjectPool<Span> s_pool;
	static PageCache* self;
    SpanList _spanlist[NPAGES];
	std::unordered_map<PAGE_ID, Span*> _idtoadd; //页号-->地址
	/*当spanlist中某一个哈希桶没有span时，向后检索更大的页，若存在更大的页则对该页进行切分，如果没有再向系统申请*/
public:
	std::mutex _mtx;//不能用桶锁
	
	Span* ConvertToSpanAdd(void* address) //根据地址计算所在span跨度
	{
		PAGE_ID id = (PAGE_ID)address >> PAGE_SHIFT;
		auto ret = _idtoadd.find(id);
		if (ret != _idtoadd.end()) return ret->second;
		assert(false);
	}

	void* BigAlloc(size_t bytes)
	{
		return SystemAlloc(bytes >> PAGE_SHIFT);
	}
	void BigFree(void* p)
	{
		SystemFree(p);
	}

	void RecoverFromCentralCache(Span* span)
	{
		//对span前后页尝试进行合并,缓解外碎片
		//终止条件：合并页超出128K;前后页正在使用 ;页不存在
		while (1)
		{
			auto previd = _idtoadd.find(span->_pageid-1);
			if (previd == _idtoadd.end() || previd->second->_isuse || previd->second->_n + span->_n > NPAGES - 1) break;
			Span* prev_span = previd->second;
			span->_pageid = prev_span->_pageid;
			span->_n += prev_span->_n;
			
			_spanlist[prev_span->_n].Erase(prev_span);
			s_pool.Delete(prev_span);
		}

		while (1)
		{
			auto nextid = _idtoadd.find(span->_pageid + span->_n);
			if (nextid == _idtoadd.end() || nextid->second->_isuse || nextid->second->_n + span->_n > NPAGES - 1) break;
			Span* next_span = nextid->second;
			span->_n += next_span->_n;
			_spanlist[next_span->_n].Erase(next_span);
			s_pool.Delete(next_span);
		}

		_spanlist[span->_n].Insert(_spanlist[span->_n].Begin(),span);
		span->_isuse = false;
		_idtoadd[span->_pageid] = span;
		_idtoadd[span->_pageid + span->_n - 1] = span;
	}

	Span* NewSpan(size_t k) //获取大小为k页的span
	{
		

		if (!_spanlist[k].IsEmpty()) return _spanlist[k].Front();

		for (size_t i = k+1; i < NPAGES; ++i)
		{
			if (!_spanlist[i].IsEmpty())
			{
				Span* nspan = _spanlist[i].Front();
				Span* kspan = s_pool.New();

				kspan->_pageid = nspan->_pageid;
				kspan->_n = k;
				for (PAGE_ID i = 0; i < kspan->_n; ++i)
				{
					_idtoadd[kspan->_pageid + i]=kspan;
				}

				nspan->_pageid += k;
				nspan->_n -= k;
				//只需要记录头尾,nspan没有返回给centralcache，只需要知道起始位置和结束位置根据页数可以得到整个span便于合并
				_idtoadd[nspan->_pageid] = nspan;
				_idtoadd[nspan->_pageid + nspan->_n - 1] = nspan;
				_spanlist[nspan->_n].Insert(_spanlist[nspan->_n].Begin(), nspan);
				return kspan;
			}
		}

		//找系统申请
		Span* bigSpan =  s_pool.New();
		void* ptr = SystemAlloc(NPAGES-1);
		bigSpan->_n = NPAGES - 1;
		bigSpan->_pageid = (PAGE_ID)(ptr) >> PAGE_SHIFT;
		_spanlist[NPAGES - 1].Insert(_spanlist[NPAGES - 1].Begin(), bigSpan);
		
		return NewSpan(k);
	}
};

PageCache* PageCache::self = new PageCache;