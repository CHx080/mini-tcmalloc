#include "PageCache.h"
Span* PageCache::ConvertToSpanAdd(void* address)
{
	PAGE_ID id = (PAGE_ID)address >> PAGE_SHIFT;
	auto ret = _idtoadd.find(id);

	if (ret != _idtoadd.end()) return ret->second;
	else
	{
		int i = 0;
	}
	assert(false); return nullptr;
}

void PageCache::RecoverFromCentralCache(Span* span)
{
	//对span前后页尝试进行合并,缓解外碎片
	//终止条件：合并页超出128K;前后页正在使用 ;页不存在
	while (1)
	{
		auto previd = _idtoadd.find(span->_pageid - 1);
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
	//合并结束
	_spanlist[span->_n].Insert(_spanlist[span->_n].Begin(), span);
	span->_isuse = false;
	_idtoadd[span->_pageid] = span; //1
	_idtoadd[span->_pageid + span->_n - 1] = span; //2
}

Span* PageCache::NewSpan(size_t k) //获取大小为k页的span
{
	if (!_spanlist[k].IsEmpty())
	{
		Span* kspan = _spanlist[k].Front();
		for (PAGE_ID i = 0; i < kspan->_n; ++i)
		{
			_idtoadd[kspan->_pageid + i] = kspan;
		}
		return kspan;
	}

	for (size_t i = k + 1; i < NPAGES; ++i)
	{
		if (!_spanlist[i].IsEmpty())
		{
			Span* nspan = _spanlist[i].Front();
			Span* kspan = s_pool.New();


			kspan->_pageid = nspan->_pageid;
			kspan->_n = k;
			for (PAGE_ID i = 0; i < kspan->_n; ++i)
			{
				_idtoadd[kspan->_pageid + i] = kspan; //3
			}

			nspan->_pageid += k;
			nspan->_n -= k;
			//只需要记录头尾,nspan没有返回给centralcache，只需要知道起始位置和结束位置根据页数可以得到整个span便于合并
			_idtoadd[nspan->_pageid] = nspan; //4
			_idtoadd[nspan->_pageid + nspan->_n - 1] = nspan; //5
			_spanlist[nspan->_n].Insert(_spanlist[nspan->_n].Begin(), nspan);
			return kspan;
		}
	}

	//找系统申请
	Span* bigSpan = s_pool.New();
	void* ptr = SystemAlloc(NPAGES - 1);
	bigSpan->_n = NPAGES - 1;
	bigSpan->_pageid = (PAGE_ID)(ptr) >> PAGE_SHIFT;
	_spanlist[NPAGES - 1].Insert(_spanlist[NPAGES - 1].Begin(), bigSpan);

	return NewSpan(k);
}

void* PageCache::BigAlloc(size_t bytes)
{
	void* p = SystemAlloc(bytes >> PAGE_SHIFT);
	_bigmemory.insert(p);
	return p;
}

void PageCache::BigFree(void* p)
{
	SystemFree(p);
}

PageCache* PageCache::GetInstance()
{
	return self;
}

PageCache* PageCache::self = new PageCache;