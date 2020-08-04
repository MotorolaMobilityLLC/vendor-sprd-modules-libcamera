#include "sprd_camalg_assist_log.h"
#include "sprd_camalg_assist.h"
#include "sprd_camalg_assist_common.h"
#include <ui/GraphicBuffer.h>
#include "graphics-base.h"

using namespace android;

typedef struct {
    sp<GraphicBuffer> gb;
} GraphicBufferHandle;

JNIEXPORT void *GraphicBuffer_new(uint32_t width, uint32_t height, int format)
{
    int gformat;
    if (FORMAT_RGB_888 == format)
        gformat = HAL_PIXEL_FORMAT_RGB_888;
    else if (FORMAT_YCRCB_420_SP == format)
        gformat = HAL_PIXEL_FORMAT_YCRCB_420_SP;
    else {
        CAA_LOGE("unsupport format");
        return NULL;
    }
    GraphicBufferHandle *h_gb = (GraphicBufferHandle *)calloc(1, sizeof(GraphicBufferHandle));
    if (NULL == h_gb) {
        CAA_LOGE("GraphicBufferHandle malloc failed");
        return NULL;
    }
    int usage = GraphicBuffer::USAGE_HW_TEXTURE | GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN;
    h_gb->gb = new GraphicBuffer(width, height, gformat, usage);
    return h_gb;
}

JNIEXPORT void GraphicBuffer_delete(void *h_graphic_buffer)
{
    GraphicBufferHandle *h_gb = (GraphicBufferHandle *)h_graphic_buffer;
    if (NULL == h_gb) {
        CAA_LOGE("null h_graphic_buffer");
        return;
    }
    h_gb->gb.clear();
    h_gb->gb = NULL;
    free(h_gb);
}

JNIEXPORT void *GraphicBuffer_lock(void *h_graphic_buffer)
{
    GraphicBufferHandle *h_gb = (GraphicBufferHandle *)h_graphic_buffer;
    if (NULL == h_gb) {
        CAA_LOGE("null h_graphic_buffer");
        return NULL;
    }
    void *buf = NULL;
    h_gb->gb->lock(GraphicBuffer::USAGE_SW_READ_OFTEN | GraphicBuffer::USAGE_SW_WRITE_OFTEN, &buf);
    CAA_LOGD("GraphicBuffer_lock: %p", buf);
    return buf;
}

JNIEXPORT void GraphicBuffer_unlock(void *h_graphic_buffer)
{
    GraphicBufferHandle *h_gb = (GraphicBufferHandle *)h_graphic_buffer;
    if (NULL == h_gb) {
        CAA_LOGE("null h_graphic_buffer");
        return;
    }
    h_gb->gb->unlock();
}

JNIEXPORT void *GraphicBuffer_getNativeBuffer(void *h_graphic_buffer)
{
    GraphicBufferHandle *h_gb = (GraphicBufferHandle *)h_graphic_buffer;
    if (NULL == h_gb) {
        CAA_LOGE("null h_graphic_buffer");
        return NULL;
    }
    return h_gb->gb->getNativeBuffer();
}