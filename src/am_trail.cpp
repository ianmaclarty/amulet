#include "amulet.h"

struct am_trail_entry {
    void *ptr;
    int value_size;
    int entry_size;
    int prev;
    char value[];
};

am_trail::am_trail() {
    capacity = 1024;
    base = (char*)malloc(capacity);
    end = 0;
    last = 0;
}

am_trail::~am_trail() {
    free(base);
}

int am_trail::get_ticket() {
    return end;
}

void am_trail::rewind_to(int ticket) {
    while (end != ticket) {
        assert(end > ticket);
        am_trail_entry *entry = (am_trail_entry*)(base + last);
        memcpy(entry->ptr, &entry->value[0], entry->value_size);
        end = last;
        last = entry->prev;
    }
}

#define ALIGN_MASK (sizeof(void*) - 1)

void am_trail::trail(void *ptr, int size) {
    int entry_size = sizeof(am_trail_entry) + size;
    while (entry_size & ALIGN_MASK) entry_size++;

    int required_capacity = end + entry_size;
    if (required_capacity > capacity) {
        while (required_capacity > capacity) capacity *= 2;
        base = (char*)realloc(base, capacity);
    }

    am_trail_entry *entry = (am_trail_entry*)(base + end);
    entry->ptr = ptr;
    entry->value_size = size;
    entry->entry_size = entry_size;
    entry->prev = last;
    memcpy(&entry->value[0], ptr, size);
    last = end;
    end += entry_size;
}
