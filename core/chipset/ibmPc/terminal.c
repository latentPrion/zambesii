
#include <firmwareTrib/firmwareRivTypes.h>
#include <firmwareTrib/firmwareRivApi.h>

struct ibmPc_terminal_fbS
{
	ubit8	ch;
	ubit8	attr;
};

struct ibmPc_terminal_fbS	*static buff;

error_t ibmPc_terminal_initialize(void);
error_t ibmPc_terminal_shutdown(void);
error_t ibmPc_terminal_suspend(void);
error_t ibmPc_terminal_awake(void);

error_t ibmPc_terminal_initialize(void)
{
	buff = __KNULL;

	buff = (ibmPc_terminal_fbS *) __kvaddrSpaceStream_getPages(1);
	if (buff == __KNULL) {
		return ERROR_MEMORY_NOMEM_VIRTUAL;
	};

	
