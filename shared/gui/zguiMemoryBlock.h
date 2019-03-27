#ifndef __ZGUI_MEMORYBLOCK_H_
#define __ZGUI_MEMORYBLOCK_H_

namespace zgui
{

/**
    A class to hold a resizable block of raw data.
*/
class MemoryBlock
{
public:
    /** Create an uninitialised block with 0 size. */
    MemoryBlock() throw();

    /** Creates a memory block with a given initial size.

        @param initialSize          the size of block to create
        @param initialiseToZero     whether to clear the memory or just leave it uninitialised
    */
    MemoryBlock (const size_t initialSize, bool initialiseToZero = false);

    /** Creates a copy of another memory block. */
    MemoryBlock (const MemoryBlock& other);

    /** Creates a memory block using a copy of a block of data.

        @param dataToInitialiseFrom     some data to copy into this block
        @param sizeInBytes              how much space to use
    */
    MemoryBlock (const void* dataToInitialiseFrom, size_t sizeInBytes);

    /** Destructor. */
    ~MemoryBlock() throw();

    /** Copies another memory block onto this one.

        This block will be resized and copied to exactly match the other one.
    */
    MemoryBlock& operator= (const MemoryBlock& other);

    /** Compares two memory blocks.

        @returns true only if the two blocks are the same size and have identical contents.
    */
    bool operator== (const MemoryBlock& other) const throw();

    /** Compares two memory blocks.

        @returns true if the two blocks are different sizes or have different contents.
    */
    bool operator!= (const MemoryBlock& other) const throw();

    /** Returns true if the data in this MemoryBlock matches the raw bytes passed-in.
    */
    bool matches (const void* data, size_t dataSize) const throw();

    //==============================================================================
    /** Returns a void pointer to the data.

        Note that the pointer returned will probably become invalid when the
        block is resized.
    */
    void* getData() const throw() { return _data; }

    /** Returns a byte from the memory block.

        This returns a reference, so you can also use it to set a byte.
    */
    template <typename Type>
    char& operator[] (const Type offset) const throw() { return _data [offset]; }


    //==============================================================================
    /** Returns the block's current allocated size, in bytes. */
    size_t getSize() const throw() { return _size; }

    /** Resizes the memory block.

        This will try to keep as much of the block's current content as it can,
        and can optionally be made to clear any new space that gets allocated at
        the end of the block.

        @param newSize                      the new desired size for the block
        @param initialiseNewSpaceToZero     if the block gets enlarged, this determines
                                            whether to clear the new section or just leave it
                                            uninitialised
        @see ensureSize
    */
    void setSize (const size_t newSize, const bool initialiseNewSpaceToZero = false);

    /** Increases the block's size only if it's smaller than a given size.

        @param minimumSize                  if the block is already bigger than this size, no action
                                            will be taken; otherwise it will be increased to this size
        @param initialiseNewSpaceToZero     if the block gets enlarged, this determines
                                            whether to clear the new section or just leave it
                                            uninitialised
        @see setSize
    */
    void ensureSize (const size_t minimumSize, bool initialiseNewSpaceToZero = false);

    /** Fills the entire memory block with a repeated byte value.

        This is handy for clearing a block of memory to zero.
    */
    void fillWith (uint8_t valueToUse) throw();

    /** Adds another block of data to the end of this one.
        The data pointer must not be null. This block's size will be increased accordingly.
    */
    void append (const void* data, size_t numBytes);

    /** Resizes this block to the given size and fills its contents from the supplied buffer.
        The data pointer must not be null.
    */
    void replaceWith (const void* data, size_t numBytes);

    /** Inserts some data into the block.
        The dataToInsert pointer must not be null. This block's size will be increased accordingly.
        If the insert position lies outside the valid range of the block, it will be clipped to
        within the range before being used.
    */
    void insert (const void* dataToInsert, size_t numBytesToInsert, size_t insertPosition);

    /** Copies data into this MemoryBlock from a memory address.

        @param srcData              the memory location of the data to copy into this block
        @param destinationOffset    the offset in this block at which the data being copied should begin
        @param numBytes             how much to copy in (if this goes beyond the size of the memory block,
                                    it will be clipped so not to do anything nasty)
    */
    void copyFrom (const void* srcData, int destinationOffset, size_t numBytes) throw();

    /** Copies data from this MemoryBlock to a memory address.

        @param destData         the memory location to write to
        @param sourceOffset     the offset within this block from which the copied data will be read
        @param numBytes         how much to copy (if this extends beyond the limits of the memory block,
                                zeros will be used for that portion of the data)
    */
    void copyTo (void* destData, int sourceOffset, size_t numBytes) const throw();


//     bool loadFromFile(CDuiString& filePath) throw();

private:
    char* _data;
    size_t _size;
};

}

#endif // __ZGUI_MEMORYBLOCK_H_
