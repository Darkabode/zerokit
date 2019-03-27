#ifndef __JUCE_ATOMIC_JUCEHEADER__
#define __JUCE_ATOMIC_JUCEHEADER__

namespace zgui
{
/**
    Simple class to hold a primitive value and perform atomic operations on it.

    The type used must be a 32 or 64 bit primitive, like an int, pointer, etc.
    There are methods to perform most of the basic atomic operations.
*/
template <typename Type>
class Atomic
{
public:
    /** Creates a new value, initialised to zero. */
    inline Atomic() throw()
        : value (0)
    {
    }

    /** Creates a new value, with a given initial value. */
    inline Atomic (const Type initialValue) throw()
        : value (initialValue)
    {
    }

    /** Copies another value (atomically). */
    inline Atomic (const Atomic& other) throw()
        : value (other.get())
    {
    }

    /** Destructor. */
    inline ~Atomic() throw()
    {
        // This class can only be used for types which are 32 or 64 bits in size.
        //static_jassert (sizeof (Type) == 4 || sizeof (Type) == 8);
    }

    /** Atomically reads and returns the current value. */
    Type get() const throw();

    /** Copies another value onto this one (atomically). */
    inline Atomic& operator= (const Atomic& other) throw()         { exchange (other.get()); return *this; }

    /** Copies another value onto this one (atomically). */
    inline Atomic& operator= (const Type newValue) throw()         { exchange (newValue); return *this; }

    /** Atomically sets the current value. */
    void set (Type newValue) throw()                               { exchange (newValue); }

    /** Atomically sets the current value, returning the value that was replaced. */
    Type exchange (Type value) throw();

    /** Atomically adds a number to this value, returning the new value. */
    Type operator+= (Type amountToAdd) throw();

    /** Atomically subtracts a number from this value, returning the new value. */
    Type operator-= (Type amountToSubtract) throw();

    /** Atomically increments this value, returning the new value. */
    Type operator++() throw();

    /** Atomically decrements this value, returning the new value. */
    Type operator--() throw();

    /** Atomically compares this value with a target value, and if it is equal, sets
        this to be equal to a new value.

        This operation is the atomic equivalent of doing this:
        @code
        bool compareAndSetBool (Type newValue, Type valueToCompare)
        {
            if (get() == valueToCompare)
            {
                set (newValue);
                return true;
            }

            return false;
        }
        @endcode

        @returns true if the comparison was true and the value was replaced; false if
                 the comparison failed and the value was left unchanged.
        @see compareAndSetValue
    */
    bool compareAndSetBool (Type newValue, Type valueToCompare) throw();

    /** Atomically compares this value with a target value, and if it is equal, sets
        this to be equal to a new value.

        This operation is the atomic equivalent of doing this:
        @code
        Type compareAndSetValue (Type newValue, Type valueToCompare)
        {
            Type oldValue = get();
            if (oldValue == valueToCompare)
                set (newValue);

            return oldValue;
        }
        @endcode

        @returns the old value before it was changed.
        @see compareAndSetBool
    */
    Type compareAndSetValue (Type newValue, Type valueToCompare) throw();

    /** Implements a memory read/write barrier. */
    static void memoryBarrier() throw();

    //==============================================================================
   #if _WIN64
    __declspec (align (8))
   #else
    __declspec (align (4))
   #endif

    /** The raw value that this class operates on.
        This is exposed publically in case you need to manipulate it directly
        for performance reasons.
    */
    volatile Type value;

private:
    static inline Type castFrom32Bit (int32_t value) throw()   { return *(Type*) &value; }
    static inline Type castFrom64Bit (int64_t value) throw()   { return *(Type*) &value; }
    static inline int32_t castTo32Bit (Type value) throw()     { return *(int32_t*) &value; }
    static inline int64_t castTo64Bit (Type value) throw()     { return *(int64_t*) &value; }

    Type operator++ (int); // better to just use pre-increment with atomics..
    Type operator-- (int);

    /** This templated negate function will negate pointers as well as integers */
    template <typename ValueType>
    inline ValueType negateValue (ValueType n) throw()
    {
        return sizeof (ValueType) == 1 ? (ValueType) -(signed char) n
            : (sizeof (ValueType) == 2 ? (ValueType) -(short) n
            : (sizeof (ValueType) == 4 ? (ValueType) -(int) n
            : ((ValueType) -(int64_t) n)));
    }

    /** This templated negate function will negate pointers as well as integers */
    template <typename PointerType>
    inline PointerType* negateValue (PointerType* n) throw()
    {
        return reinterpret_cast <PointerType*> (-reinterpret_cast <puint_t>(n));
    }
};


//==============================================================================
/*
    The following code is in the header so that the atomics can be inlined where possible...
*/
#define JUCE_ATOMICS_WINDOWS 1    // Windows with intrinsics

#ifndef __INTEL_COMPILER
//  #pragma intrinsic (_InterlockedExchange, _InterlockedIncrement, _InterlockedDecrement, _InterlockedCompareExchange, \
//                     _InterlockedCompareExchange64, _InterlockedExchangeAdd, _ReadWriteBarrier)
#endif
#define juce_InterlockedExchange(a, b)              _InterlockedExchange(a, b)
#define juce_InterlockedIncrement(a)                _InterlockedIncrement(a)
#define juce_InterlockedDecrement(a)                _InterlockedDecrement(a)
#define juce_InterlockedExchangeAdd(a, b)           _InterlockedExchangeAdd(a, b)
#define juce_InterlockedCompareExchange(a, b, c)    _InterlockedCompareExchange(a, b, c)
#define juce_InterlockedCompareExchange64(a, b, c)  _InterlockedCompareExchange64(a, b, c)
#define juce_MemoryBarrier _ReadWriteBarrier

#if _WIN64
#ifndef __INTEL_COMPILER
 #pragma intrinsic (_InterlockedExchangeAdd64, _InterlockedExchange64, _InterlockedIncrement64, _InterlockedDecrement64)
#endif
#define juce_InterlockedExchangeAdd64(a, b)     _InterlockedExchangeAdd64(a, b)
#define juce_InterlockedExchange64(a, b)        _InterlockedExchange64(a, b)
#define juce_InterlockedIncrement64(a)          _InterlockedIncrement64(a)
#define juce_InterlockedDecrement64(a)          _InterlockedDecrement64(a)
#else
// None of these atomics are available in a 32-bit Windows build!!
template <typename Type> static Type juce_InterlockedExchangeAdd64 (volatile Type* a, Type b) throw()  { /*jassertfalse; */Type old = *a; *a += b; return old; }
template <typename Type> static Type juce_InterlockedExchange64 (volatile Type* a, Type b) throw()     { /*jassertfalse; */Type old = *a; *a = b; return old; }
template <typename Type> static Type juce_InterlockedIncrement64 (volatile Type* a) throw()            { /*jassertfalse; */return ++*a; }
template <typename Type> static Type juce_InterlockedDecrement64 (volatile Type* a) throw()            { /*jassertfalse; */return --*a; }
#define JUCE_64BIT_ATOMICS_UNAVAILABLE 1
#endif

#pragma warning (push)
#pragma warning (disable: 4311)  // (truncation warning)

template <typename Type>
inline Type Atomic<Type>::get() const throw()
{
    return sizeof (Type) == 4 ? castFrom32Bit ((int32_t)juce_InterlockedExchangeAdd ((volatile long*) &value, (long) 0))
                              : castFrom64Bit ((int64_t)juce_InterlockedExchangeAdd64 ((volatile __int64*) &value, (__int64) 0));
}

template <typename Type>
inline Type Atomic<Type>::exchange (const Type newValue) throw()
{
    return sizeof (Type) == 4 ? castFrom32Bit ((int32_t)juce_InterlockedExchange ((volatile long*) &value, (long) castTo32Bit (newValue)))
                              : castFrom64Bit ((int64_t)juce_InterlockedExchange64 ((volatile __int64*) &value, (__int64) castTo64Bit (newValue)));
}

template <typename Type>
inline Type Atomic<Type>::operator+= (const Type amountToAdd) throw()
{
    return sizeof (Type) == 4 ? (Type) (juce_InterlockedExchangeAdd ((volatile long*) &value, (long) amountToAdd) + (long) amountToAdd)
                              : (Type) (juce_InterlockedExchangeAdd64 ((volatile __int64*) &value, (__int64) amountToAdd) + (__int64) amountToAdd);
}

template <typename Type>
inline Type Atomic<Type>::operator-= (const Type amountToSubtract) throw()
{
    return operator+= (negateValue (amountToSubtract));
}

template <typename Type>
inline Type Atomic<Type>::operator++() throw()
{
    return sizeof (Type) == 4 ? (Type) juce_InterlockedIncrement ((volatile long*) &value)
                              : (Type) juce_InterlockedIncrement64 ((volatile __int64*) &value);
}

template <typename Type>
inline Type Atomic<Type>::operator--() throw()
{
    return sizeof (Type) == 4 ? (Type) juce_InterlockedDecrement ((volatile long*) &value)
                              : (Type) juce_InterlockedDecrement64 ((volatile __int64*) &value);
}

template <typename Type>
inline bool Atomic<Type>::compareAndSetBool (const Type newValue, const Type valueToCompare) throw()
{
    return compareAndSetValue (newValue, valueToCompare) == valueToCompare;
}

template <typename Type>
inline Type Atomic<Type>::compareAndSetValue (const Type newValue, const Type valueToCompare) throw()
{
    return sizeof (Type) == 4 ? castFrom32Bit ((int32_t) juce_InterlockedCompareExchange ((volatile long*) &value, (long) castTo32Bit (newValue), (long) castTo32Bit (valueToCompare)))
                              : castFrom64Bit ((int64_t) juce_InterlockedCompareExchange64 ((volatile __int64*) &value, (__int64) castTo64Bit (newValue), (__int64) castTo64Bit (valueToCompare)));
}

template <typename Type>
inline void Atomic<Type>::memoryBarrier() throw()
{
    juce_MemoryBarrier();
}

#pragma warning (pop)

}

#endif   // __JUCE_ATOMIC_JUCEHEADER__
