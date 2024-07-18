#pragma once
#include "Common.hpp"

#ifdef _WIN32
#include <Windows.h>
static void* SystemAlloc(size_t k)
{
	void* p = VirtualAlloc(NULL, k << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return p; //p的地址一定是页大小的整数倍
}
#endif

/*pagecache挂的也是span(每一个span大小是页大小的倍数);映射规则不同于上2层，哈希函数更简单(直接定值法)*/
class PageCache
{
private:
	SpanList _spanlist[NPAGES];
	
	/*当spanlist中某一个哈希桶没有span时，向后检索更大的页，若存在更大的页则对该页进行切分，如果没有再向系统申请*/
public:
	std::mutex _mtx;//不能用桶锁

	Span* NewSpan(size_t k) //获取大小为k页的span
	{
		assert(k);
		if (!_spanlist[k].IsEmpty()) return _spanlist[k].Front();

		for (int i = k+1; i < NPAGES; ++i)
		{
			if (!_spanlist[i].IsEmpty())
			{
				Span* nspan = _spanlist->Begin();
				Span* kspan = new Span;

				kspan->_pageid = nspan->_pageid;
				kspan->_n = k;

				nspan->_pageid += k;
				nspan->_n -= k;

				_spanlist[nspan->_n].Insert(_spanlist->Begin(), nspan);
				return kspan;
			}
		}

		//找系统申请
		Span* bigSpan = new Span;
		bigSpan->_freelist = SystemAlloc(k);
		bigSpan->_n = NPAGES - 1;
		bigSpan->_pageid = (PAGE_ID)(bigSpan->_freelist) >> PAGE_SHIFT;
		_spanlist[NPAGES - 1].Insert(_spanlist[NPAGES - 1].Begin(), bigSpan);

		return NewSpan(k);
	}
};