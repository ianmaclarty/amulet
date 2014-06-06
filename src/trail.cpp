#include "amulet.h"

struct am_trail_entry {
    void *ptr;
    int size;
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
        memcpy(entry->ptr, &entry->value[0], entry->size);
        end -= entry->size + sizeof(am_trail_entry);
        last = entry->prev;
    }
}

void am_trail::trail(void *ptr, int size) {
    ensure_big_enough(size);
    am_trail_entry *entry = (am_trail_entry*)(base + end);
    entry->ptr = ptr;
    entry->size = size;
    entry->prev = last;
    memcpy(&entry->value[0], ptr, size);
    last = end;
    end += sizeof(am_trail_entry) + size;
}

void am_trail::ensure_big_enough(int size) {
    int required_size = end + sizeof(am_trail_entry) + size;
    if (required_size > capacity) {
        while (required_size > capacity) capacity *= 2;
        base = (char*)realloc(base, capacity);
    }
}
