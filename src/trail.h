struct am_trail {
    char *base;
    int capacity;
    int end;
    int last;

    am_trail();
    virtual ~am_trail();

    int get_ticket();
    void rewind_to(int ticket);
    void trail(void *ptr, int size);

    void ensure_big_enough(int size);
};
