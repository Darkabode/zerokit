#ifndef __ZGUI_HEAPBLOCK_H_
#define __ZGUI_HEAPBLOCK_H_

namespace zgui
{

template <class ElementType>
class HeapBlock
{
public:
    /** Creates a HeapBlock which is initially just a null pointer.

        After creation, you can resize the array using the malloc(), calloc(),
        or realloc() methods.
    */
    HeapBlock() throw() :
    data (0)
    {
    }

    /** Creates a HeapBlock containing a number of elements.

        The contents of the block are undefined, as it will have been created by a
        malloc call.

        If you want an array of zero values, you can use the calloc() method or the
        other constructor that takes an InitialisationState parameter.
    */
    explicit HeapBlock (const size_t numElements) :
    data(static_cast<ElementType*>(fn_memalloc(numElements * sizeof(ElementType))))
    {
    }

//     /** Creates a HeapBlock containing a number of elements.
// 
//         The initialiseToZero parameter determines whether the new memory should be cleared,
//         or left uninitialised.
//     */
//     HeapBlock (const size_t numElements, const bool initialiseToZero) :
//     data(static_cast<ElementType*>(initialiseToZero ? std::calloc (numElements, sizeof (ElementType)) : std::malloc (numElements * sizeof (ElementType))))
//     {
//     }

    /** Destructor.
        This will free the data, if any has been allocated.
    */
    ~HeapBlock()
    {
        fn_memfree(data);
    }

    /** Returns a raw pointer to the allocated data.
        This may be a null pointer if the data hasn't yet been allocated, or if it has been
        freed by calling the free() method.
    */
    inline operator ElementType*() const throw() { return data; }

    /** Returns a raw pointer to the allocated data.
        This may be a null pointer if the data hasn't yet been allocated, or if it has been
        freed by calling the free() method.
    */
    inline ElementType* getData() const throw() { return data; }

    /** Returns a void pointer to the allocated data.
        This may be a null pointer if the data hasn't yet been allocated, or if it has been
        freed by calling the free() method.
    */
    inline operator void*() const throw() { return static_cast <void*> (data); }

    /** Returns a void pointer to the allocated data.
        This may be a null pointer if the data hasn't yet been allocated, or if it has been
        freed by calling the free() method.
    */
    inline operator const void*() const throw() { return static_cast <const void*> (data); }

    /** Lets you use indirect calls to the first element in the array.
        Obviously this will cause problems if the array hasn't been initialised, because it'll
        be referencing a null pointer.
    */
    inline ElementType* operator->() const  throw() { return data; }

    /** Returns a reference to one of the data elements.
        Obviously there's no bounds-checking here, as this object is just a dumb pointer and
        has no idea of the size it currently has allocated.
    */
    template <typename IndexType>
    inline ElementType& operator[] (IndexType index) const throw() { return data [index]; }

    /** Returns a pointer to a data element at an offset from the start of the array.
        This is the same as doing pointer arithmetic on the raw pointer itself.
    */
    template <typename IndexType>
    inline ElementType* operator+ (IndexType index) const throw() { return data + index; }

    /** Compares the pointer with another pointer.
        This can be handy for checking whether this is a null pointer.
    */
    inline bool operator== (const ElementType* const otherPointer) const throw() { return otherPointer == data; }

    /** Compares the pointer with another pointer.
        This can be handy for checking whether this is a null pointer.
    */
    inline bool operator!= (const ElementType* const otherPointer) const throw() { return otherPointer != data; }

    /** Allocates a specified amount of memory.

        This uses the normal malloc to allocate an amount of memory for this object.
        Any previously allocated memory will be freed by this method.

        The number of bytes allocated will be (newNumElements * elementSize). Normally
        you wouldn't need to specify the second parameter, but it can be handy if you need
        to allocate a size in bytes rather than in terms of the number of elements.

        The data that is allocated will be freed when this object is deleted, or when you
        call free() or any of the allocation methods.
    */
    void malloc(const size_t newNumElements, const size_t elementSize = sizeof (ElementType))
    {
        fn_memfree(data);
        data = static_cast <ElementType*> (fn_memalloc(newNumElements * elementSize));
    }

    /** Allocates a specified amount of memory and clears it.
        This does the same job as the malloc() method, but clears the memory that it allocates.
    */
    void calloc(const size_t newNumElements, const size_t elementSize = sizeof (ElementType))
    {
        fn_memfree(data);
        data = static_cast <ElementType*> (fn_memalloc(newNumElements * elementSize)/*::HeapAlloc(::GetProcessHeap()/ *gHeap* /, HEAP_ZERO_MEMORY, newNumElements * elementSize)*/);
    }

    /** Allocates a specified amount of memory and optionally clears it.
        This does the same job as either malloc() or calloc(), depending on the
        initialiseToZero parameter.
    */
    void allocate(const size_t newNumElements, bool initialiseToZero)
    {
        fn_memfree(data);
        data = static_cast<ElementType*>(initialiseToZero ? fn_memcalloc(newNumElements * sizeof(ElementType)) : fn_memalloc(newNumElements * sizeof(ElementType)));
    }

    /** Re-allocates a specified amount of memory.

        The semantics of this method are the same as malloc() and calloc(), but it
        uses realloc() to keep as much of the existing data as possible.
    */
    void realloc (const size_t newNumElements, const size_t elementSize = sizeof (ElementType))
    {
        data = static_cast<ElementType*>(fn_memrealloc(data, newNumElements * elementSize));
    }

    /** Frees any currently-allocated data.
        This will free the data and reset this object to be a null pointer.
    */
    void free()
    {
        fn_memfree(data);
        data = 0;
    }

    /** Swaps this object's data with the data of another HeapBlock.
        The two objects simply exchange their data pointers.
    */
    void swapWith(HeapBlock <ElementType>& other) throw()
    {
        ElementType* temp = data;
        data = other.data;
        other.data = temp;
    }

    /** This fills the block with zeros, up to the number of elements specified.
        Since the block has no way of knowing its own size, you must make sure that the number of
        elements you specify doesn't exceed the allocated size.
    */
    void clear (size_t numElements) throw()
    {
        __stosb((unsigned char*)data, 0, sizeof(ElementType) * numElements);
    }

private:
    ZGUI_DECLARE_NON_COPYABLE(HeapBlock);
    ZGUI_PREVENT_HEAP_ALLOCATION; // Creating a 'new HeapBlock' would be missing the point!

	ElementType* data;
};

}

#endif // __ZGUI_HEAPBLOCK_H_
