#ifndef __JUCE_REFERENCECOUNTEDOBJECT_JUCEHEADER__
#define __JUCE_REFERENCECOUNTEDOBJECT_JUCEHEADER__

namespace zgui
{
/**
    Adds reference-counting to an object.

    To add reference-counting to a class, derive it from this class, and
    use the ReferenceCountedObjectPtr class to point to it.

    e.g. @code
    class MyClass : public ReferenceCountedObject
    {
        void foo();

        // This is a neat way of declaring a typedef for a pointer class,
        // rather than typing out the full templated name each time..
        typedef ReferenceCountedObjectPtr<MyClass> Ptr;
    };

    MyClass::Ptr p = new MyClass();
    MyClass::Ptr p2 = p;
    p = 0;
    p2->foo();
    @endcode

    Once a new ReferenceCountedObject has been assigned to a pointer, be
    careful not to delete the object manually.

    This class uses an Atomic<int> value to hold the reference count, so that it
    the pointers can be passed between threads safely. For a faster but non-thread-safe
    version, use SingleThreadedReferenceCountedObject instead.

    @see ReferenceCountedObjectPtr, ReferenceCountedArray, SingleThreadedReferenceCountedObject
*/
class ReferenceCountedObject
{
public:
    //==============================================================================
    /** Increments the object's reference count.

        This is done automatically by the smart pointer, but is public just
        in case it's needed for nefarious purposes.
    */
    inline void incReferenceCount() throw()
    {
        ++refCount;
    }

    /** Decreases the object's reference count.

        If the count gets to zero, the object will be deleted.
    */
    inline void decReferenceCount() throw()
    {
        if (--refCount == 0) {
            delete this;
        }
    }

    /** Returns the object's current reference count. */
    inline int getReferenceCount() const throw()       { return refCount.get(); }


protected:
    //==============================================================================
    /** Creates the reference-counted object (with an initial ref count of zero). */
    ReferenceCountedObject()
    {
    }

    /** Destructor. */
    virtual ~ReferenceCountedObject()
    {
    }

    /** Resets the reference count to zero without deleting the object.
        You should probably never need to use this!
    */
    void resetReferenceCount() throw()
    {
        refCount = 0;
    }

private:
    //==============================================================================
    Atomic<int> refCount;
};

/**
    Adds reference-counting to an object.

    This is efectively a version of the ReferenceCountedObject class, but which
    uses a non-atomic counter, and so is not thread-safe (but which will be more
    efficient).
    For more details on how to use it, see the ReferenceCountedObject class notes.

    @see ReferenceCountedObject, ReferenceCountedObjectPtr, ReferenceCountedArray
*/
class SingleThreadedReferenceCountedObject
{
public:
    //==============================================================================
    /** Increments the object's reference count.

        This is done automatically by the smart pointer, but is public just
        in case it's needed for nefarious purposes.
    */
    inline void incReferenceCount() throw()
    {
        ++refCount;
    }

    /** Decreases the object's reference count.

        If the count gets to zero, the object will be deleted.
    */
    inline void decReferenceCount() throw()
    {
        if (--refCount == 0) {
            delete this;
        }
    }

    /** Returns the object's current reference count. */
    inline int getReferenceCount() const throw()       { return refCount; }


protected:
    //==============================================================================
    /** Creates the reference-counted object (with an initial ref count of zero). */
    SingleThreadedReferenceCountedObject() : refCount (0)  {}

    /** Destructor. */
    virtual ~SingleThreadedReferenceCountedObject()
    {
    }

private:
    //==============================================================================
    int refCount;
};


//==============================================================================
/**
    A smart-pointer class which points to a reference-counted object.

    The template parameter specifies the class of the object you want to point to - the easiest
    way to make a class reference-countable is to simply make it inherit from ReferenceCountedObject,
    but if you need to, you could roll your own reference-countable class by implementing a pair of
    mathods called incReferenceCount() and decReferenceCount().

    When using this class, you'll probably want to create a typedef to abbreviate the full
    templated name - e.g.
    @code typedef ReferenceCountedObjectPtr<MyClass> MyClassPtr;@endcode

    @see ReferenceCountedObject, ReferenceCountedObjectArray
*/
template <class ReferenceCountedObjectClass>
class ReferenceCountedObjectPtr
{
public:
    /** The class being referenced by this pointer. */
    typedef ReferenceCountedObjectClass ReferencedType;

    //==============================================================================
    /** Creates a pointer to a null object. */
    inline ReferenceCountedObjectPtr() throw()
        : referencedObject (0)
    {
    }

    /** Creates a pointer to an object.

        This will increment the object's reference-count if it is non-null.
    */
    inline ReferenceCountedObjectPtr (ReferenceCountedObjectClass* const refCountedObject) throw()
        : referencedObject (refCountedObject)
    {
        if (refCountedObject != 0)
            refCountedObject->incReferenceCount();
    }

    /** Copies another pointer.
        This will increment the object's reference-count (if it is non-null).
    */
    inline ReferenceCountedObjectPtr (const ReferenceCountedObjectPtr& other) throw() :
    referencedObject(other.referencedObject)
    {
        if (referencedObject != 0) {
            referencedObject->incReferenceCount();
        }
    }

    /** Copies another pointer.
        This will increment the object's reference-count (if it is non-null).
    */
    template <class DerivedClass>
    inline ReferenceCountedObjectPtr (const ReferenceCountedObjectPtr<DerivedClass>& other) throw()
        : referencedObject (static_cast <ReferenceCountedObjectClass*> (other.get()))
    {
        if (referencedObject != 0)
            referencedObject->incReferenceCount();
    }

    /** Changes this pointer to point at a different object.

        The reference count of the old object is decremented, and it might be
        deleted if it hits zero. The new object's count is incremented.
    */
    ReferenceCountedObjectPtr& operator= (const ReferenceCountedObjectPtr& other)
    {
        return operator= (other.referencedObject);
    }

    /** Changes this pointer to point at a different object.

        The reference count of the old object is decremented, and it might be
        deleted if it hits zero. The new object's count is incremented.
    */
    template <class DerivedClass>
    ReferenceCountedObjectPtr& operator= (const ReferenceCountedObjectPtr<DerivedClass>& other)
    {
        return operator= (static_cast <ReferenceCountedObjectClass*> (other.get()));
    }

    /** Changes this pointer to point at a different object.

        The reference count of the old object is decremented, and it might be
        deleted if it hits zero. The new object's count is incremented.
    */
    ReferenceCountedObjectPtr& operator= (ReferenceCountedObjectClass* const newObject)
    {
        if (referencedObject != newObject)
        {
            if (newObject != 0)
                newObject->incReferenceCount();

            ReferenceCountedObjectClass* const oldObject = referencedObject;
            referencedObject = newObject;

            if (oldObject != 0)
                oldObject->decReferenceCount();
        }

        return *this;
    }

    /** Destructor.

        This will decrement the object's reference-count, and may delete it if it
        gets to zero.
    */
    inline ~ReferenceCountedObjectPtr()
    {
        if (referencedObject != 0)
            referencedObject->decReferenceCount();
    }

    /** Returns the object that this pointer references.
        The pointer returned may be zero, of course.
    */
    inline operator ReferenceCountedObjectClass*() const throw()
    {
        return referencedObject;
    }

    // the -> operator is called on the referenced object
    inline ReferenceCountedObjectClass* operator->() const throw()
    {
        return referencedObject;
    }

    /** Returns the object that this pointer references.
        The pointer returned may be zero, of course.
    */
    inline ReferenceCountedObjectClass* get() const throw()
    {
        return referencedObject;
    }

    /** Returns the object that this pointer references.
        The pointer returned may be zero, of course.
    */
    inline ReferenceCountedObjectClass* getObject() const throw()
    {
        return referencedObject;
    }

private:
    //==============================================================================
    ReferenceCountedObjectClass* referencedObject;
};


/** Compares two ReferenceCountedObjectPointers. */
template <class ReferenceCountedObjectClass>
bool operator== (const ReferenceCountedObjectPtr<ReferenceCountedObjectClass>& object1, ReferenceCountedObjectClass* const object2) throw()
{
    return object1.get() == object2;
}

/** Compares two ReferenceCountedObjectPointers. */
template <class ReferenceCountedObjectClass>
bool operator== (const ReferenceCountedObjectPtr<ReferenceCountedObjectClass>& object1, const ReferenceCountedObjectPtr<ReferenceCountedObjectClass>& object2) throw()
{
    return object1.get() == object2.get();
}

/** Compares two ReferenceCountedObjectPointers. */
template <class ReferenceCountedObjectClass>
bool operator== (ReferenceCountedObjectClass* object1, ReferenceCountedObjectPtr<ReferenceCountedObjectClass>& object2) throw()
{
    return object1 == object2.get();
}

/** Compares two ReferenceCountedObjectPointers. */
template <class ReferenceCountedObjectClass>
bool operator!= (const ReferenceCountedObjectPtr<ReferenceCountedObjectClass>& object1, const ReferenceCountedObjectClass* object2) throw()
{
    return object1.get() != object2;
}

/** Compares two ReferenceCountedObjectPointers. */
template <class ReferenceCountedObjectClass>
bool operator!= (const ReferenceCountedObjectPtr<ReferenceCountedObjectClass>& object1, ReferenceCountedObjectPtr<ReferenceCountedObjectClass>& object2) throw()
{
    return object1.get() != object2.get();
}

/** Compares two ReferenceCountedObjectPointers. */
template <class ReferenceCountedObjectClass>
bool operator!= (ReferenceCountedObjectClass* object1, ReferenceCountedObjectPtr<ReferenceCountedObjectClass>& object2) throw()
{
    return object1 != object2.get();
}

}

#endif   // __JUCE_REFERENCECOUNTEDOBJECT_JUCEHEADER__
