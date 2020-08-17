struct debug_tag {
    struct debug_tag *prev, *next;
    cmr_u32 st[2];
};
static struct debug_tag debug_head = {
    .prev = &debug_head,
    .next = &debug_head,
    .st[0] = 0,
    .st[1] = 1,
};

void * __wrap_malloc(size_t size)
{
    void *result;
    struct debug_tag *pdebug;

    result = malloc(size + sizeof(struct debug_tag));
    if (!result)
        return result;
    pdebug = (struct debug_tag *)result;
    pdebug->next = &debug_head;
    pdebug->prev = debug_head.prev;
    debug_head.prev->next = pdebug;
    debug_head.st[0]++;
    //printf pdebug, size
    CMR_LOGI("lucid, %p %d", pdebug, size);

    result = ((unsigned char *)result + (int)sizeof(struct debug_tag));


    return result;
}

void __wrap_free(void *p)
{
    struct debug_tag *pdebug;

    if ((unsigned long)p <= sizeof(struct debug_tag))
        return;
    p = (unsigned char *)p + (int)sizeof(struct debug_tag);
    pdebug = (struct debug_tag *)p;
    pdebug->prev->next = pdebug->next;
    pdebug->next->prev = pdebug->prev;
    CMR_LOGI("lucid, %p", pdebug);
    free(p);
}

void show_to_log()
{
    struct debug_tag *p = debug_head.next;

    while (p != &debug_head && p) {
        //printf p
        p = p->next;
    }
}






