#ifndef __ZGUI_H_
#define __ZGUI_H_

#ifdef _DEBUG
  #define zgui_assertfalse { if (fn_IsDebuggerPresent()) __debugbreak(); }
  #define zgui_assert(expression) { if (! (expression)) zgui_assertfalse; }

#else
  #define zgui_assertfalse { juce_LogCurrentAssertion }
  #define zgui_assert(a) {}
#endif

#ifndef __FILET__
#define __DUILIB_STR2WSTR(str)	L##str
#define _DUILIB_STR2WSTR(str)	__DUILIB_STR2WSTR(str)
#ifdef _UNICODE
#define __FILET__	_DUILIB_STR2WSTR(__FILE__)
#define __FUNCTIONT__	_DUILIB_STR2WSTR(__FUNCTION__)
#else
#define __FILET__	__FILE__
#define __FUNCTIONT__	__FUNCTION__
#endif
#endif

#define _CRT_SECURE_NO_DEPRECATE

// Remove pointless warning messages
#pragma warning (disable : 4511) // copy operator could not be generated
#pragma warning (disable : 4512) // assignment operator could not be generated
#pragma warning (disable : 4702) // unreachable code (bugs in Microsoft's STL)
#pragma warning (disable : 4786) // identifier was truncated
#pragma warning (disable : 4996) // function or variable may be unsafe (deprecated)
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS // eliminate deprecation warnings for VS2005
#endif

#define ZGUI_DECLARE_NON_COPYABLE(className) \
    className (const className&); \
    className& operator= (const className&)

#define ZGUI_PREVENT_HEAP_ALLOCATION \
    private: \
        static void* operator new (size_t); \
        static void operator delete (void*);

void* __cdecl operator new(size_t, void* p);

namespace zgui {
	/** This namespace contains a few template classes for helping work out class type variations.
	*/
	namespace TypeHelpers
	{
		/** The ParameterType struct is used to find the best type to use when passing some kind
			of object as a parameter.

			Of course, this is only likely to be useful in certain esoteric template situations.

			Because "typename TypeHelpers::ParameterType<SomeClass>::type" is a bit of a mouthful, there's
			a PARAMETER_TYPE(SomeClass) macro that you can use to get the same effect.

			E.g. "myFunction (PARAMETER_TYPE (int), PARAMETER_TYPE (MyObject))"
			would evaluate to "myfunction (int, const MyObject&)", keeping any primitive types as
			pass-by-value, but passing objects as a const reference, to avoid copying.
			*/
		template <typename Type> struct ParameterType                   { typedef const Type& type; };

		template <typename Type> struct ParameterType <Type&>           { typedef Type& type; };
		template <typename Type> struct ParameterType <Type*>           { typedef Type* type; };
		template <>              struct ParameterType <char>            { typedef char type; };
		template <>              struct ParameterType <unsigned char>   { typedef unsigned char type; };
		template <>              struct ParameterType <short>           { typedef short type; };
		template <>              struct ParameterType <unsigned short>  { typedef unsigned short type; };
		template <>              struct ParameterType <int>             { typedef int type; };
		template <>              struct ParameterType <uint32_t>    { typedef uint32_t type; };
		template <>              struct ParameterType <long>            { typedef long type; };
		template <>              struct ParameterType <unsigned long>   { typedef unsigned long type; };
		template <>              struct ParameterType <int64_t>         { typedef int64_t type; };
		template <>              struct ParameterType <uint64_t>        { typedef uint64_t type; };
		template <>              struct ParameterType <bool>            { typedef bool type; };
		template <>              struct ParameterType <float>           { typedef float type; };
		template <>              struct ParameterType <double>          { typedef double type; };

		/** A helpful macro to simplify the use of the ParameterType template.
			@see ParameterType
			*/
#define PARAMETER_TYPE(a)    typename TypeHelpers::ParameterType<a>::type


		/** These templates are designed to take a type, and if it's a double, they return a double
			type; for anything else, they return a float type.
			*/
		template <typename Type> struct SmallestFloatType             { typedef float  type; };
		template <>              struct SmallestFloatType <double>    { typedef double type; };
	}

	/** Handy function for getting the number of elements in a simple const C array.
		E.g.
		@code
		static int myArray[] = { 1, 2, 3 };

		int numElements = numElementsInArray (myArray) // returns 3
		@endcode
		*/
	template <typename Type, int N>
	inline int numElementsInArray(Type(&array)[N])
	{
		(void)array; // (required to avoid a spurious warning in MS compilers)
		(void) sizeof (0[array]); // This line should cause an error if you pass an object with a user-defined subscript operator
		return N;
	}

	template <typename Type>
	inline Type* addBytesToPointer(Type* pointer, int bytes) throw()  { return (Type*)(((char*)pointer) + bytes); }

	template <typename Type1, typename Type2>
	inline int getAddressDifference(Type1* pointer1, Type2* pointer2) throw()  { return (int)(((const char*)pointer1) - (const char*)pointer2); }


	template <typename Type>
	inline bool isPositiveAndBelow(Type valueToTest, Type upperLimit) throw()
	{
		return Type() <= valueToTest && valueToTest < upperLimit;
	}

	template <typename Type>
	inline void swapVariables(Type& variable1, Type& variable2)
	{
		Type temp = variable1;
		variable1 = variable2;
		variable2 = temp;
	}

	template <typename Type>
	inline Type zgui_limit(const Type lowerLimit, const Type upperLimit, const Type valueToConstrain) throw()
	{
		return (valueToConstrain < lowerLimit) ? lowerLimit : ((upperLimit < valueToConstrain) ? upperLimit : valueToConstrain);
	}

	template <typename FloatType>
	inline int roundToInt(const FloatType value)
	{
		union { int asInt[2]; double asDouble; } n;
		n.asDouble = ((double)value) + 6755399441055744.0;

		return n.asInt[0];
	}

#define sizeOfArray(x) (sizeof(x) / sizeof(x[0]))


	/** A simple COM smart pointer. */
	template <class ComClass>
	class ComSmartPtr
	{
	public:
		ComSmartPtr() throw() :
		p(0)
		{
		}

		ComSmartPtr(ComClass* const p_) :
		p(p_)
		{
			if (p_ != 0) {
				p_->AddRef();
			}
		}

		ComSmartPtr(const ComSmartPtr<ComClass>& p_) :
		p(p_.p)
		{
			if (p != 0) {
				p->AddRef();
			}
		}

		ComSmartPtr(IUnknown* lp)
		{
			if (lp != 0) {
				if (FAILED(lp->QueryInterface(__uuidof(ComClass), (void**)&p))) {
					p = NULL;
				}
			}
		}

		~ComSmartPtr()
		{
			release();
		}

		operator ComClass*() const throw()
		{
			return p;
		}

		ComClass& operator*() const throw()
		{
			return *p;
		}

		ComClass* operator->() const throw()
		{
			return p;
		}

		ComClass** address()
		{
			return &p;
		}

		ComSmartPtr& operator= (ComClass* const newP)
		{
			if (newP != 0) {
				newP->AddRef();
			}
			release();
			p = newP;
			return *this;
		}

		ComSmartPtr& operator= (const ComSmartPtr<ComClass>& newP)  { return operator= (newP.p); }

		// Releases and nullifies this pointer and returns its address
		ComClass** resetAndGetPointerAddress()
		{
			release();
			p = 0;
			return &p;
		}

		HRESULT CoCreateInstance(REFCLSID classUUID, DWORD dwClsContext = CLSCTX_INPROC_SERVER)
		{
			return fn_CoCreateInstance(classUUID, 0, dwClsContext, __uuidof (ComClass), (void**)resetAndGetPointerAddress());
		}

		template <class OtherComClass>
		HRESULT QueryInterface(REFCLSID classUUID, ComSmartPtr<OtherComClass>& destObject) const
		{
			if (p == 0) {
				return E_POINTER;
			}

			return p->QueryInterface(classUUID, (void**)destObject.resetAndGetPointerAddress());
		}

		template <class OtherComClass>
		HRESULT QueryInterface(ComSmartPtr<OtherComClass>& destObject) const
		{
			return this->QueryInterface(__uuidof (OtherComClass), destObject);
		}

	private:
		ComClass* p;

		void release()
		{
			if (p != 0) {
				p->Release();
			}
		}

		ComClass** operator&() throw(); // private to avoid it being used accidentally
	};


	class Variant : public VARIANT
	{
	public:
		Variant() 
		{ 
			fn_VariantInit(this);
		}
		Variant(int i)
		{
			fn_VariantInit(this);
			this->vt = VT_I4;
			this->intVal = i;
		}
		Variant(float f)
		{
			fn_VariantInit(this);
			this->vt = VT_R4;
			this->fltVal = f;
		}
		Variant(LPOLESTR s)
		{
			fn_VariantInit(this);
			this->vt = VT_BSTR;
			this->bstrVal = s;
		}
		Variant(IDispatch *disp)
		{
			fn_VariantInit(this);
			this->vt = VT_DISPATCH;
			this->pdispVal = disp;
		}

		~Variant()
		{
			fn_VariantClear(this);
		}
	};
}

//extern HANDLE ::GetProcessHeap()/*gHeap*/;

#if defined _M_IX86
	#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_IA64
	#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
	#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
	#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif

#include "zguiQuickSort.h"
#include "zguiAtomic.h"
#include "zguiHeapBlock.h"
#include "zguiString.h"
#include "zguiArray.h"
#include "zguiStringArray.h"
#include "zguiStringPairArray.h"
#include "zguiStringPtrArray.h"
#include "zguiUtils.h"
#include "zguiScopedLock.h"
#include "zguiReferenceCountedObject.h"
#include "zguiCriticalSection.h"
#include "zguiSingleton.h"
#include "zguiWaitableEvent.h"
#include "zguiSpinLock.h"
#include "zguiThread.h"
#include "zguiStopwatch.h"
#include "zguiTimespan.h"
#include "zguiDirectoryIterator.h"

#include "zguiHttpClient.h"

#include "zguiMemoryBlock.h"
#include "zguiLzmaArchReader.h"
#include "zguiResources.h"
#include "zguiPngLoad.h"

#include "zguiDelegate.h"
#include "zguiDefine.h"
#include "zguiLanguage.h"
#include "zguiBase.h"
#include "zguiAnimation.h"
#include "zguiPaintManager.h"
#include "zguiControl.h"
#include "zguiContainer.h"

#include "zguiMarkup.h"
#include "zguiGuiBuilder.h"
#include "zguiRender.h"
#include "zguiWinImplBase.h"

#include "zguiSystemTrayIcon.h"

#include "zguiVerticalLayout.h"
#include "zguiHorizontalLayout.h"
#include "zguiTileLayout.h"
#include "zguiTabLayout.h"
#include "zguiChildLayout.h"

#include "zguiList.h"
#include "zguiCombo.h"
#include "zguiScrollBar.h"

#include "zguiLabel.h"
#include "zguiText.h"
#include "zguiEdit.h"

#include "zguiButton.h"

#include "zguiTreeView.h"

#include "zguiProgress.h"
#include "zguiSlider.h"

#include "zguiRichEdit.h"
#include "zguiDateTime.h"

#include "zguiCamera.h"

#include "zguiActiveX.h"
#include "zguiWebBrowser.h"


#define lengthof(x) (sizeof(x)/sizeof(*x))
#define MAX max
#define MIN min
#define CLAMP(x,a,b) (MIN(b,MAX(a,x)))

#endif // __ZGUI_H_
