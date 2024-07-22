#pragma once
#include "Common.hpp"
#include <unordered_map>
#ifdef _WIN32
#include <Windows.h>
static void* SystemAlloc(size_t k)
{
	void* p = VirtualAlloc(NULL, k << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	return p; //p�ĵ�ַһ����ҳ��С��������
}
static void SystemFree(void* p)
{
	VirtualFree(p, 0, MEM_RELEASE);
}
#endif

/*pagecache�ҵ�Ҳ��span(ÿһ��span��С��ҳ��С�ı���);ӳ�����ͬ����2�㣬��ϣ��������(ֱ�Ӷ�ֵ��)*/
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
	std::unordered_map<PAGE_ID, Span*> _idtoadd; //ҳ��-->��ַ
	/*��spanlist��ĳһ����ϣͰû��spanʱ�������������ҳ�������ڸ����ҳ��Ը�ҳ�����з֣����û������ϵͳ����*/
public:
	std::mutex _mtx;//������Ͱ��
	
	Span* ConvertToSpanAdd(void* address) //���ݵ�ַ��������span���
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
		//��spanǰ��ҳ���Խ��кϲ�,��������Ƭ
		//��ֹ�������ϲ�ҳ����128K;ǰ��ҳ����ʹ�� ;ҳ������
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

	Span* NewSpan(size_t k) //��ȡ��СΪkҳ��span
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
				//ֻ��Ҫ��¼ͷβ,nspanû�з��ظ�centralcache��ֻ��Ҫ֪����ʼλ�úͽ���λ�ø���ҳ�����Եõ�����span���ںϲ�
				_idtoadd[nspan->_pageid] = nspan;
				_idtoadd[nspan->_pageid + nspan->_n - 1] = nspan;
				_spanlist[nspan->_n].Insert(_spanlist[nspan->_n].Begin(), nspan);
				return kspan;
			}
		}

		//��ϵͳ����
		Span* bigSpan =  s_pool.New();
		void* ptr = SystemAlloc(NPAGES-1);
		bigSpan->_n = NPAGES - 1;
		bigSpan->_pageid = (PAGE_ID)(ptr) >> PAGE_SHIFT;
		_spanlist[NPAGES - 1].Insert(_spanlist[NPAGES - 1].Begin(), bigSpan);
		
		return NewSpan(k);
	}
};

PageCache* PageCache::self = new PageCache;