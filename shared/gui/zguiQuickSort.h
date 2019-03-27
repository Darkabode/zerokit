#ifndef __ZGUI_QUICKSORT_H_
#define __ZGUI_QUICKSORT_H_

namespace zgui {

class QuickSort
{
public:
	static void sort(void *base, size_t num, size_t width, int(*comp)(void *, const void *, const void *), void *context);

private:
	static void swap(char *a, char *b, size_t width);
	static void shortsort_s(char* lo, char* hi, size_t width, int(*comp)(void*, const void*, const void*), void* context);
};

}

#endif // __ZGUI_QUICKSORT_H_
