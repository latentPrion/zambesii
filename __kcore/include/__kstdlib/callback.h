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

// Polymorphism is great.
class Callback
{
public:
	Callback(void)
	{}

	virtual void operator()(MessageStream::sHeader *message)=0;
	virtual ~Callback(void) {};
};

typedef void (MessageStreamCallbackFn)(MessageStream::sHeader *msg);

template <class FnPtrT>
class MessageStreamCallback
: public Callback
{
public:
	MessageStreamCallback(FnPtrT fnPtr = NULL)
	: function(fnPtr)
	{}

	virtual ~MessageStreamCallback(void) { function = NULL; };
	virtual void operator()(MessageStream::sHeader *message)=0;

protected:
	FnPtrT		function;
};

class MessageStreamCb
: public MessageStreamCallback<MessageStreamCallbackFn *>
{
public:
	MessageStreamCb(MessageStreamCallbackFn *fnPtr = NULL)
	: MessageStreamCallback<MessageStreamCallbackFn *>(fnPtr)
	{}

	virtual void operator()(MessageStream::sHeader *message)
		{ function(message); }
};

#endif
