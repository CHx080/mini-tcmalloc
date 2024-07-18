#pragma once
#include "Common.hpp"

#ifdef _WIN32
#include <Windows.h>
static void* SystemAlloc(size_t k)
{
	void* p = VirtualAlloc(NULL, k << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return p; //p�ĵ�ַһ����ҳ��С��������
}
#endif

/*pagecache�ҵ�Ҳ��span(ÿһ��span��С��ҳ��С�ı���);ӳ�����ͬ����2�㣬��ϣ��������(ֱ�Ӷ�ֵ��)*/
class PageCache
{
private:
	SpanList _spanlist[NPAGES];
	
	/*��spanlist��ĳһ����ϣͰû��spanʱ�������������ҳ�������ڸ����ҳ��Ը�ҳ�����з֣����û������ϵͳ����*/
public:
	std::mutex _mtx;//������Ͱ��

	Span* NewSpan(size_t k) //��ȡ��СΪkҳ��span
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

		//��ϵͳ����
		Span* bigSpan = new Span;
		bigSpan->_freelist = SystemAlloc(k);
		bigSpan->_n = NPAGES - 1;
		bigSpan->_pageid = (PAGE_ID)(bigSpan->_freelist) >> PAGE_SHIFT;
		_spanlist[NPAGES - 1].Insert(_spanlist[NPAGES - 1].Begin(), bigSpan);

		return NewSpan(k);
	}
};