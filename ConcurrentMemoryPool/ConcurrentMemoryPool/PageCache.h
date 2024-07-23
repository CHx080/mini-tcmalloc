#pragma once
#include "Common.hpp"
#include <unordered_map>
#include <unordered_set>
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
	PageCache() {}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;
private:
	ObjectPool<Span> s_pool;
	static PageCache* self;
    SpanList _spanlist[NPAGES];
	std::unordered_map<PAGE_ID, Span*> _idtoadd; //ҳ��-->��ַ
	/*��spanlist��ĳһ����ϣͰû��spanʱ�������������ҳ�������ڸ����ҳ��Ը�ҳ�����з֣����û������ϵͳ����*/
public:
	std::mutex _mtx;//������Ͱ��
	std::unordered_set<void*> _bigmemory;

	static PageCache* GetInstance(); //��ȡ��������
	
	Span* ConvertToSpanAdd(void* address); //���ݵ�ַ��������span���

	void* BigAlloc(size_t bytes); //����256KB�ڴ�����
	void BigFree(void* p);	//����256KB�ڴ��ͷ�
	
	Span* NewSpan(size_t k); //��ȡ��СΪkҳ��span
	
	
	void RecoverFromCentralCache(Span* span); //��centralcache�л��տռ�
	
};
