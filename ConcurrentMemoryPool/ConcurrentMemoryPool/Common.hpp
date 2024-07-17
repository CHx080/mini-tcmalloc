#pragma once
#include <assert.h>
constexpr size_t MAX_BYTES = 256 * 1024;  //小于256KB的找ThreadCache要
constexpr size_t NUM_LIST = 208;

class FreeList //管理切分好的块空间
{
public:
	void Push(void* obj)//头插
	{
		assert(obj);//不能为空
		*(void**)obj = _freelist;
		_freelist = obj;
		++_size;
	}

	void* Pop()//头删
	{
		void* obj = _freelist;
		_freelist = *(void**)obj;
		--_size;
		return obj;
	}

	bool IsEmpty()
	{
		return _freelist == nullptr;
	}

	size_t& Maxsize()
	{
		return _maxsize;
	}
private:
	void* _freelist = nullptr; //自由链表的头指针
	size_t _size = 0; //自由链表的长度
	size_t _maxsize = 1; //与慢开始算法相关
};



class SizeMap
{
	// [1,128]				8byte对⻬ freelist[0,16)
	// [128+1,1024]			16byte对⻬ freelist[16,72)
	// [1024+1,81024]		128byte对⻬ freelist[72,128)
	// [8*1024+1,641024]	1024byte对⻬ freelist[128,184)
	// [64*1024+1,256*1024] 8*1024byte对⻬ freelist[184,208)
	// 所申请的空间大小必须是相应区间内对齐数的倍数
	// 每一个对齐数所对应的自由链表桶存放存放其倍数大小的空间
	//例如freelist[0]表示每一个节点为指向8字节空间指针的链表，freelist[1]则为16字节空间
private:
	static size_t _RoundUp(size_t bytes, size_t align)
	{
		return ((bytes + align - 1) & ~(align - 1));
	}
	static size_t _Index(size_t bytes, size_t align_shift)
	{
		return ((bytes + ((size_t)1 << align_shift) - 1) >> align_shift) - 1;
	}
public:
	static size_t RoundUp(size_t bytes)//计算对齐数
	{
		if (bytes <= 128) return _RoundUp(bytes, 8);
		else if (bytes <= 1024) return _RoundUp(bytes, 16);
		else if (bytes <= 81024) return _RoundUp(bytes, 128);
		else if (bytes <= 64 * 1024) return _RoundUp(bytes, 1024);
		else if (bytes <= 256 * 1024) return _RoundUp(bytes, 8 * 1024);
		else assert(false);
	}

	static size_t Index(size_t bytes)
	{
		static int group_array[4] = { 16, 56, 56, 56 }; //每一个区间第一个桶的下标
		if (bytes <= 128) {
			return _Index(bytes, 3);
		}
		else if (bytes <= 1024) {
			return _Index(bytes - 128, 4) + group_array[0];
		}
		else if (bytes <= 81024) {
			return _Index(bytes - 1024, 7) + group_array[1] + group_array[0];
		}
		else if (bytes <= 64 * 1024) {
			return _Index(bytes - 8 * 1024, 10) + group_array[2] +
				group_array[1] + group_array[0];
		}
		else if (bytes <= 256 * 1024) {
			return _Index(bytes - 64 * 1024, 13) + group_array[3] +
				group_array[2] + group_array[1] + group_array[0];
		}
		else {
			assert(false);
		}
	}

	static size_t NumMoveSize(size_t size)
	{	
		if (size == 0)
			return 0;
		// [2, 512]，⼀次批量移动多少个对象的(慢启动)上下限值
		// ⼩对象⼀次批量上限⾼
		// ⼩对象⼀次批量上限低
		size_t num = MAX_BYTES / size;
		if (num < 2)
			num = 2;
		if (num > 512)
			num = 512;
		return num;
	}
};
