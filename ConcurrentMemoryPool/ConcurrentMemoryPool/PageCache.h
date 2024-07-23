#pragma once
#include "Common.hpp"
#include <unordered_map>
#include <unordered_set>
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
	PageCache() {}
	PageCache(const PageCache&) = delete;
	PageCache& operator=(const PageCache&) = delete;
private:
	ObjectPool<Span> s_pool;
	static PageCache* self;
    SpanList _spanlist[NPAGES];
	std::unordered_map<PAGE_ID, Span*> _idtoadd; //页号-->地址
	/*当spanlist中某一个哈希桶没有span时，向后检索更大的页，若存在更大的页则对该页进行切分，如果没有再向系统申请*/
public:
	std::mutex _mtx;//不能用桶锁
	std::unordered_set<void*> _bigmemory;

	static PageCache* GetInstance(); //获取单例对象
	
	Span* ConvertToSpanAdd(void* address); //根据地址计算所在span跨度

	void* BigAlloc(size_t bytes); //大于256KB内存申请
	void BigFree(void* p);	//大于256KB内存释放
	
	Span* NewSpan(size_t k); //获取大小为k页的span
	
	
	void RecoverFromCentralCache(Span* span); //从centralcache中回收空间
	
};
