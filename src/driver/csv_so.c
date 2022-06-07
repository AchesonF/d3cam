#include "inc_files.h"


#ifdef __cplusplus
extern "C" {
#endif


int csv_so_open (struct csv_so_t *pSO)
{


	pSO->handle = dlopen(pSO->path, RTLD_LAZY);
	if (!pSO->handle) {
		log_err("ERROR : dlopen '%s'", pSO->path);
		return -1;
	}

	dlerror();	/* clear any existing error */

	pSO->func1 = dlsym(pSO->handle, "some_func");
	if (!pSO->func1) {
		log_err("ERROR : dlsym '%s'", "some_func");
		return -1;
	}

	// todo

	return 0;
}

int csv_so_close (struct csv_so_t *pSO)
{

	dlclose(pSO->handle);

	return 0;
}

int csv_so_init (void)
{
	struct csv_so_t *pSO = &gCSV->so; 

	pSO->path = PATH_SO_FILE;
	pSO->handle = NULL;

	return csv_so_open(pSO);
}


#ifdef __cplusplus
}
#endif

