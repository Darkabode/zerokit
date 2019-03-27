#ifndef __JUCE_ARRAYALLOCATIONBASE_JUCEHEADER__
#define __JUCE_ARRAYALLOCATIONBASE_JUCEHEADER__

/**
    Implements some basic array storage allocation functions.

    This class isn't really for public use - it's used by the other
    array classes, but might come in handy for some purposes.

    It inherits from a critical section class to allow the arrays to use
    the "empty base class optimisation" pattern to reduce their footprint.

    @see Array, OwnedArray, ReferenceCountedArray
*/
template <class ElementType>
class ArrayAllocationBase
{
public:
    //==============================================================================
    /** Creates an empty array. */
    ArrayAllocationBase() throw()
        : numAllocated (0)
    {
    }

    /** Destructor. */
    ~ArrayAllocationBase() throw()
    {
    }

    //==============================================================================
    /** Changes the amount of storage allocated.

        This will retain any data currently held in the array, and either add or
        remove extra space at the end.

        @param numElements  the number of elements that are needed
    */
    void setAllocatedSize (const int numElements)
    {
        if (numAllocated != numElements) {
            if (numElements > 0) {
                elements.realloc((size_t) numElements);
            }
            else {
                elements.free();
            }

            numAllocated = numElements;
        }
    }

    /** Increases the amount of storage allocated if it is less than a given amount.

        This will retain any data currently held in the array, but will add
        extra space at the end to make sure there it's at least as big as the size
        passed in. If it's already bigger, no action is taken.

        @param minNumElements  the minimum number of elements that are needed
    */
    void ensureAllocatedSize (const int minNumElements)
    {
        if (minNumElements > numAllocated) {
            setAllocatedSize((minNumElements + minNumElements / 2 + 8) & ~7);
        }
    }

    /** Minimises the amount of storage allocated so that it's no more than
        the given number of elements.
    */
    void shrinkToNoMoreThan (const int maxNumElements)
    {
        if (maxNumElements < numAllocated) {
            setAllocatedSize (maxNumElements);
        }
    }

    /** Swap the contents of two objects. */
    void swapWith (ArrayAllocationBase <ElementType>& other) throw()
    {
        elements.swapWith(other.elements);
        int tempNum = numAllocated;
        numAllocated = other.numAllocated;
        other.numAllocated = tempNum;
    }

    HeapBlock<ElementType> elements;
    int numAllocated;

private:
    ZGUI_DECLARE_NON_COPYABLE (ArrayAllocationBase);
};


#endif   // __JUCE_ARRAYALLOCATIONBASE_JUCEHEADER__
