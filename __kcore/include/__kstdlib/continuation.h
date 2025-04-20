#ifndef _CONTINUATION_H
	#define _CONTINUATION_H

template <class originContextT, typename localFunctionContextT>
class Continuation
{
public:
	originContextT		origCtxt;
	localFunctionContextT	localCtxt;
};

#endif /* _CONTINUATION_H */
