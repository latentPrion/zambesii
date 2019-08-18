
#define UDI_VERSION 0x101
#include <udi.h>
#define UDI_PHYSIO_VERSION 0x101
#include <udi_physio.h>
#include <tests.h>
#include <__kclasses/tests.h>
#include <__kclasses/debugPipe.h>
#include <kernel/common/floodplainn/dma.h>

namespace tests
{

namespace __kclasses
{

namespace memBmp
{

TESTS_FN_MAKE_PROTOTYPE_DEFVARS(constrainedGetFrames)
{
	const int				N_ATTRS = 16;
	status_t				s;
	fplainn::dma::Constraints		c;
	fplainn::dma::constraints::Compiler	cmp;
	udi_dma_constraints_attr_spec_t		attrs[] = {
		{ UDI_DMA_SCGTH_FORMAT, UDI_SCGTH_32 | UDI_SCGTH_64 },
		{ UDI_DMA_ELEMENT_GRANULARITY_BITS, 12 }
	};

	TESTS_VARS_INIT_DEFVARS();

	c.initialize(attrs, 0);
	s = cmp.compile(&c);
	if (s == ERROR_SUCCESS)
	{
		/* SCGTH_FORMAT is required by our implementation. */
		FAILED_DEFVARS("Should fail because SCGTH_FORMAT isn't set.\n");
	} else
		{ SUCCEEDED_DEFVARS(); };

	attrs[0].attr_type = UDI_DMA_SCGTH_FORMAT;
	attrs[0].attr_value = UDI_SCGTH_32 | UDI_SCGTH_64;
	c.addOrModifyAttrs(attrs, 1);
	s = cmp.compile(&c);
	if (s != ERROR_SUCCESS)
	{
		FAILED_DEFVARS("Should have succeeded because SCGTH_FORMAT is "
			"set now.\n");
	}
	else
		{ SUCCEEDED_DEFVARS(); }

	return TESTS_RETURN_DEFVARS();
}

TESTS_FN_MAKE_PROTOTYPE_DEFVARS(fragmentedGetFrames)
{
	TESTS_VARS_INIT_DEFVARS();
	return TESTS_RETURN_DEFVARS();
}

testFn *tests[] = {
	constrainedGetFrames,
	fragmentedGetFrames,
	NULL
};

}

}

}
