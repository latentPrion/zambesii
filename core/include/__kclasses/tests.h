#ifndef ___KCLASSES_TESTS_H
	#define ___KCLASSES_TESTS_H

namespace tests
{

namespace __kclasses
{

namespace memBmp
{
status_t constrainedGetFrames(
	uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed);
status_t fragmentedGetFrames(
	uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed);
}

}

}

#endif
