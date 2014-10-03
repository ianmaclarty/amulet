// all objects managed by lua inherit from this
struct am_userdata {
    int num_refs; // number of references from this object to other
                  // lua objects. Use am
    am_userdata();
};
