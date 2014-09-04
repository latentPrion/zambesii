#ifndef _CALLBACK_H
	#define _CALLBACK_H

	#include <kernel/common/messageStream.h>

/**	EXPLANATION:
 * Simple callback continuation class framework. This enables us to create
 * polymorphic continuations. Each different sequence that requires a continuation
 * can create a custom derived class to suit its needs.
 *
 * The only disadvantage is that it requires dynamic memory allocation for
 * every continuation. But once we get past that, it's very clean.
 *
 * When the heap is in guardpaged mode however, this may end up exhausting
 * it very quickly. This problem can be alleviated a little by using a custom
 * slam cache for __kCallback objects specifically.
 **/

class Callback
{
public:
	virtual ~Callback(void) {};
	virtual void operator()(MessageStream::sHeader *message)=0;
};

template <class T>
class _Callback
: public Callback
{
protected:
	_Callback(T *fn)
	:
	function(fn)
	{}

public:
	virtual void operator()(MessageStream::sHeader *message)=0;

	virtual ~_Callback(void) { function = NULL; }

protected:
	T		*function;
};

typedef void (__kcbFn)(MessageStream::sHeader *msg);
class __kCallback
: public _Callback<__kcbFn>
{
public:
	__kCallback(__kcbFn *fn) : _Callback<__kcbFn>(fn) {}

	virtual void operator()(MessageStream::sHeader *message)
		{ function(message); }
};

#endif
