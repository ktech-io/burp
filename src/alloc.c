#include "burp.h"
#include "alloc.h"
#include "log.h"

#ifdef UTEST
int alloc_errors=0;
uint64_t alloc_count=0;
uint64_t free_count=0;
void alloc_counters_reset(void)
{
	alloc_count=0;
	free_count=0;
}

static char *errored(const char *func)
{
	log_oom_w(__func__, func);
	return NULL;
}
#endif

char *strdup_w(const char *s, const char *func)
{
	char *ret;
#ifdef UTEST
	if(alloc_errors) return errored(func);
#endif
	if(!(ret=strdup(s))) log_oom_w(__func__, func);
#ifdef UTEST
	else alloc_count++;
#endif
	return ret;
}

void *realloc_w(void *ptr, size_t size, const char *func)
{
	void *ret;
#ifdef UTEST
	int already_alloced=0;
	if(alloc_errors) return errored(func);
	if(ptr) already_alloced=1;
#endif
	if(!(ret=realloc(ptr, size))) log_oom_w(__func__, func);
#ifdef UTEST
	else if(!already_alloced) alloc_count++;
#endif
	return ret;
}

void *malloc_w(size_t size, const char *func)
{
	void *ret;
#ifdef UTEST
	if(alloc_errors) return errored(func);
#endif
	if(!(ret=malloc(size))) log_oom_w(__func__, func);
#ifdef UTEST
	else alloc_count++;
#endif
	return ret;
}

void *calloc_w(size_t nmem, size_t size, const char *func)
{
	void *ret;
#ifdef UTEST
	if(alloc_errors) return errored(func);
#endif
	if(!(ret=calloc(nmem, size))) log_oom_w(__func__, func);
#ifdef UTEST
	else alloc_count++;
#endif
	return ret;
}

void free_v(void **ptr)
{
	if(!ptr || !*ptr) return;
	free(*ptr);
	*ptr=NULL;
#ifdef UTEST
	free_count++;
#endif
}

void free_w(char **str)
{
	free_v((void **)str);
}