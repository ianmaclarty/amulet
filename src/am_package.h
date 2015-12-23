struct am_package {
    char *filename;
    void *handle;
};

am_package* am_open_package(const char *filename, char **errmsg);
void am_close_package(am_package *pkg);

// free returned pointer with free()
void *am_read_package_resource(am_package *pkg, const char *filename, int *len, char **errmsg);

bool am_package_resource_exists(am_package *pkg, const char *filename);
