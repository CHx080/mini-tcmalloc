#include "ThreadCache.h"
void* ThreadCache::Allocate(size_t bytes)
{
	if (bytes == 0) return nullptr;
	assert(bytes > 0 && bytes <= MAX_BYTES); //��������ֽ�������С��256KB
	size_t align = SizeMap::RoundUp(bytes);
	size_t index = SizeMap::Index(align);
	
	if (!_freelists[index].IsEmpty())	//��Ӧ��С���������������ÿռ䣬ֱ�ӻ�ȡ
	{
		return _freelists[index].Pop();
	}
	else //��������Ϊ�գ�����һ��Ҫ
	{
		return FetchFromCentralCache(index, align);
	}
}

void ThreadCache::Deallocate(void* p)
{
	assert(p);
	size_t bytes = PageCache::GetInstance()->ConvertToSpanAdd(p)->_objsize;

	if (bytes > MAX_BYTES) 
	{
		PageCache::GetInstance()->BigFree(p); return;
	}

	size_t align = SizeMap::RoundUp(bytes);
	size_t index = SizeMap::Index(align);
	_freelists[index].Push(p);

	//�����ʱ_freelists[index]�����������ȴﵽmaxsize����һ���ֹ黹��centralcache
	if (_freelists[index].Size() > _freelists[index].Maxsize())
	{
		void* start = SolveListTooLong(_freelists[index], _freelists[index].Maxsize());
		CentralCache::GetInstance()->RecoverFromThreadCache(start, align);
	}
}

void* ThreadCache::SolveListTooLong(FreeList& freelist, size_t n)//n��ʶ�黹����
{
	assert(n <= freelist.Size());
	void* start = freelist.PeekHead(), * end = freelist.PeekHead();
	for (int i = 0; i < n - 1; ++i)
	{
		end = *(void**)end;
	}
	freelist.PopRange(end, n);
	*(void**)end = nullptr;
	return start;
}

void* ThreadCache::FetchFromCentralCache(size_t index, size_t bytes) //bytesΪ�����������ֽ���
{

	/*����ʼ�����㷨��С��������������ٸ�,���������������������batchnum*/
	size_t batchnum = min(_freelists[index].Maxsize(), SizeMap::NumMoveSize(bytes));

	if (_freelists[index].Maxsize() == batchnum)
	{
		_freelists[index].Maxsize() += 2;
	}

	//��ͼ�Ӵ����Ļ����н�ȡһ�οռ�
	void* start = nullptr, * end = nullptr;//��Ϊ����Ͳ���
	size_t actualnum = CentralCache::GetInstance()->FetchRangeObj(start, end, batchnum, bytes);

	assert(actualnum >= 1);
	assert(start);
	assert(end);

	if (actualnum > 1)
	{
		void* temp = *(void**)start;
		_freelists[index].PushRange(temp, end, actualnum - 1);
		//�Ѷ�Ŀռ��ݴ����߳���ӵ�е�����������
	}
	return start;
}

