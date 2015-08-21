struct am_allocator;

am_allocator *am_new_allocator();
void *am_alloc(void *allocator, void *ptr, size_t osize, size_t nsize);
void am_destroy_allocator(am_allocator *allocator);
