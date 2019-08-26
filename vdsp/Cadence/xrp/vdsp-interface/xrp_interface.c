#include "xrp_interface.h"
#include "xrp_api.h"
#include "xrp_host_common.h"

__attribute__ ((visibility("default"))) struct xrp_device *sprd_xrp_open_device(int idx, enum xrp_status *status)
{
	return xrp_open_device(idx , XRP_OPEN_NORMAL_TYPE , status);
}
/*!
 * Increment device reference count.
 */
__attribute__ ((visibility("default"))) void sprd_xrp_retain_device(struct xrp_device *device)
{
	return xrp_retain_device(device);
}

/*!
 * Decrement device reference count (and free associated resources once the
 * counter gets down to zero).
 */
__attribute__ ((visibility("default"))) void sprd_xrp_release_device(struct xrp_device *device)
{
	return xrp_release_device(device);
}

#ifdef USE_SPRD_MODE
__attribute__ ((visibility("default"))) struct xrp_buffer *sprd_xrp_create_buffer(struct xrp_device *device,
                                     size_t size, void *host_ptr,int32_t fd,
                                     enum xrp_status *status)
{
	return xrp_create_buffer(device,size,host_ptr,fd , status);
}
#else
__attribute__ ((visibility("default"))) struct xrp_buffer *sprd_xrp_create_buffer(struct xrp_device *device,
                                     size_t size, void *host_ptr,
                                     enum xrp_status *status)
{
        return xrp_create_buffer(device,size,host_ptr, status);
}
#endif
__attribute__ ((visibility("default"))) void sprd_xrp_retain_buffer(struct xrp_buffer *buffer)
{
	return xrp_retain_buffer(buffer);
}

__attribute__ ((visibility("default"))) void sprd_xrp_release_buffer(struct xrp_buffer *buffer)
{
	return xrp_release_buffer(buffer);
}

__attribute__ ((visibility("default"))) void *sprd_xrp_map_buffer(struct xrp_buffer *buffer, size_t offset, size_t size,
                     enum xrp_access_flags map_flags, enum xrp_status *status)
{
	return xrp_map_buffer(buffer,offset,size,map_flags,status);
}

__attribute__ ((visibility("default"))) void sprd_xrp_unmap_buffer(struct xrp_buffer *buffer, void *p,
                      enum xrp_status *status)
{
	return xrp_unmap_buffer(buffer,p,status);
}

__attribute__ ((visibility("default"))) void sprd_xrp_buffer_get_info(struct xrp_buffer *buffer, enum xrp_buffer_info info,
                         void *out, size_t out_sz, enum xrp_status *status)
{
	return xrp_buffer_get_info(buffer,info,out,out_sz,status);
}

__attribute__ ((visibility("default"))) struct xrp_buffer_group *sprd_xrp_create_buffer_group(enum xrp_status *status)
{
	return xrp_create_buffer_group(status);
}

__attribute__ ((visibility("default"))) void sprd_xrp_retain_buffer_group(struct xrp_buffer_group *group)
{
	return xrp_retain_buffer_group(group);
}

__attribute__ ((visibility("default"))) void sprd_xrp_release_buffer_group(struct xrp_buffer_group *group)
{
	return xrp_release_buffer_group(group);
}

__attribute__ ((visibility("default"))) size_t sprd_xrp_add_buffer_to_group(struct xrp_buffer_group *group,
                               struct xrp_buffer *buffer,
                               enum xrp_access_flags access_flags,
                               enum xrp_status *status)
{
	return xrp_add_buffer_to_group(group,buffer,access_flags,status);
}

__attribute__ ((visibility("default"))) void sprd_xrp_set_buffer_in_group(struct xrp_buffer_group *group,
                             size_t index,
                             struct xrp_buffer *buffer,
                             enum xrp_access_flags access_flags,
                             enum xrp_status *status)
{
	return xrp_set_buffer_in_group(group,index,buffer,access_flags,status);
}

__attribute__ ((visibility("default"))) struct xrp_buffer *sprd_xrp_get_buffer_from_group(struct xrp_buffer_group *group,
                                             size_t idx,
                                             enum xrp_status *status)
{
	return xrp_get_buffer_from_group(group,idx,status);
}
__attribute__ ((visibility("default"))) void sprd_xrp_buffer_group_get_info(struct xrp_buffer_group *group,
                               enum xrp_buffer_group_info info, size_t idx,
                               void *out, size_t out_sz,
                               enum xrp_status *status)
{
	return xrp_buffer_group_get_info(group,info,idx,out,out_sz,status);
}

__attribute__ ((visibility("default"))) struct xrp_queue *sprd_xrp_create_queue(struct xrp_device *device,
                                   enum xrp_status *status)
{
	return xrp_create_queue(device,status);
}

__attribute__ ((visibility("default"))) struct xrp_queue *sprd_xrp_create_ns_queue(struct xrp_device *device,
                                      const void *nsid,
                                      enum xrp_status *status)
{
	return xrp_create_ns_queue(device,nsid,status);
}
__attribute__ ((visibility("default"))) void sprd_xrp_retain_queue(struct xrp_queue *queue)
{
	return xrp_retain_queue(queue);
}

__attribute__ ((visibility("default"))) void sprd_xrp_release_queue(struct xrp_queue *queue)
{
	return xrp_release_queue(queue);
}

__attribute__ ((visibility("default"))) void sprd_xrp_retain_event(struct xrp_event *event)
{
	return xrp_retain_event(event);
}

__attribute__ ((visibility("default"))) void sprd_xrp_release_event(struct xrp_event *event)
{
	return xrp_release_event(event);
}

__attribute__ ((visibility("default"))) void sprd_xrp_event_status(struct xrp_event *event, enum xrp_status *status)
{
	return xrp_event_status(event,status);
}
#ifdef USE_SPRD_MODE
__attribute__ ((visibility("default"))) void sprd_xrp_run_command_sync(struct xrp_queue *queue,
			const void *in_data, int in_data_fd , size_t in_data_size,
			void *out_data, int out_data_fd , size_t out_data_size,
			struct xrp_buffer_group *buffer_group,
			enum xrp_status *status)
{
        return xrp_run_command_sync(queue,in_data,in_data_fd , in_data_size, out_data, out_data_fd , out_data_size,buffer_group,status);
}
#else
__attribute__ ((visibility("default"))) void sprd_xrp_run_command_sync(struct xrp_queue *queue,
                          const void *in_data, size_t in_data_size,
                          void *out_data, size_t out_data_size,
                          struct xrp_buffer_group *buffer_group,
                          enum xrp_status *status)
{
	return xrp_run_command_sync(queue,in_data,in_data_size,out_data,out_data_size,buffer_group,status);
}
#endif
#ifdef USE_SPRD_MODE
__attribute__ ((visibility("default"))) void sprd_xrp_enqueue_command(struct xrp_queue *queue,
                         const void *in_data, int in_data_fd , size_t in_data_size,
                         void *out_data, int out_data_fd , size_t out_data_size,
                         struct xrp_buffer_group *buffer_group,
                         struct xrp_event **event,
                         enum xrp_status *status)
{
        return xrp_enqueue_command(queue,in_data,in_data_fd , in_data_size,out_data, out_data_fd , out_data_size,buffer_group,event,status);
}
#else
__attribute__ ((visibility("default"))) void sprd_xrp_enqueue_command(struct xrp_queue *queue,
                         const void *in_data, size_t in_data_size,
                         void *out_data, size_t out_data_size,
                         struct xrp_buffer_group *buffer_group,
                         struct xrp_event **event,
                         enum xrp_status *status)
{
	return xrp_enqueue_command(queue,in_data,in_data_size,out_data,out_data_size,buffer_group,event,status);
}
#endif
__attribute__ ((visibility("default"))) void sprd_xrp_wait(struct xrp_event *event, enum xrp_status *status)
{
	return xrp_wait(event,status);
}

__attribute__ ((visibility("default"))) struct xrp_queue *sprd_xrp_create_nsp_queue(struct xrp_device *device,
                                       const void *nsid,
                                       int priority,
                                       enum xrp_status *status)
{
	return xrp_create_nsp_queue(device,nsid,priority,status);
}
/*!
 * set xrp_buffer domain apptr
 */
__attribute__ ((visibility("default"))) void sprd_xrp_set_apptr(struct xrp_buffer *buffer,void * apptr)
{
	if(apptr!= NULL)
	{
		buffer->apptr = apptr;
	}
	return;
}
