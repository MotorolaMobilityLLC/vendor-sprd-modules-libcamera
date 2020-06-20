#pragma once

#include <sys/cdefs.h>

__BEGIN_DECLS

void *get_lib_handle(const char *lib_name);
void put_lib_handle(void *handle);

__END_DECLS
