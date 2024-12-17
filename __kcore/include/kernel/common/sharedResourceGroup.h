#ifndef _SHARED_RESOURCE_GROUP_H
	#define _SHARED_RESOURCE_GROUP_H

	/**	EXPLANATION:
	 * A resource group is a class which encapsulates the shared resource
	 * locking for any set of parallel resources. In order to disambiguate
	 * the locks for any set of resources, the resources and their lock are
	 * grouped together. The template allows for strong type checking on
	 * the lock.
	 *
	 * Use of this class within the kernel is *mandatory*.
	 **/

template <class lockType, class resourceType>
class SharedResourceGroup
{
public:
	lockType	lock;
	resourceType	rsrc;
};

#endif

