#include <sys/types.h>
#include <utils/Log.h>
#include <dlfcn.h>
#include <stdio.h>
#include <cutils/properties.h>

int main(int argc, char **argv)
{
	void *handle = NULL;
	int (*read_sensor) (void);

	ALOGE("srid E\n");

	handle = dlopen("libcamsensor.so", RTLD_NOW);
	if (!handle) {
		fprintf(stderr, "%s\n", dlerror());
		ALOGE("srid dlopen err %s\n", dlerror());
	}
	read_sensor = (int *)dlsym(handle, "sensor_rid_read_sensor");
	if (dlerror() != NULL) {
		fprintf(stderr, "%s\n", dlerror());
		ALOGE("srid dlsym err %s\n", dlerror());
	}
	if(read_sensor)
		read_sensor();

	dlclose(handle);

	ALOGE("srid X\n");

	return 0;
}
