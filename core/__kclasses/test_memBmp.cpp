
#include <__kclasses/tests.h>

namespace tests
{

namespace __kclasses
{

namespace memBmp
{
status_t constrainedGetFrames(
	uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed
	)
{
	const int				N_ATTRS = 16;
	status_t				s;
	fplainn::Zudi::DmaConstraints		c;
	udi_dma_constraints_attr_spec_t		attrs[] = {
		{ UDI_DMA_SCGTH_FORMAT, UDI_SCGTH_32 | UDI_SCGTH_64 },
		{ UDI_DMA_ELEMENT_GRANULARITY_BITS, 12 }

	};

	*nTotal = 1;
	*nSucceeded = *nFailed = 0;

	c.initialize();
	c.addOrModifyAttrs(attrs, 0);

	s = c.compiler.compile();
	if (s == ERROR_SUCCESS)
	{
		/* SCGTH_FORMAT is required by our implementation. */
		printf(ERROR"Should fail because SCGTH_FORMAT isn't set.\n");
		*nFailed += 1;
	} else { *nSucceeded += 1; };

	attrs[0].attr_type = UDI_DMA_SCGTH_FORMAT;
	attrs[0].attr_value = UDI_SCGTH_32 | UDI_SCGTH_64;
	c.addOrModifyAttrs(attrs, 1);
	s = s.compiler.compile();
	if (s != ERROR_SUCCESS)
	{
		printf(ERROR"Should have succeeded because SCGTH_FORMAT is "
			"set now.\n");
		*nFailed += 1;
	}
	else { *nSucceeded += 1; }
}

status_t fragmentedGetFrames(
	uarch_t *nTotal, uarch_t *nSucceeded, uarch_t *nFailed
	)
{
}

}

}

}
