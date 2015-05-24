#include "amulet.h"

am_package* am_open_package(const char *filename, char **errmsg) {
    am_package *pkg = (am_package*)malloc(sizeof(am_package));
    pkg->handle = (mz_zip_archive*)malloc(sizeof(mz_zip_archive));
    memset(pkg->handle, 0, sizeof(mz_zip_archive));
    if (!mz_zip_reader_init_file((mz_zip_archive*)pkg->handle, filename, MZ_ZIP_FLAG_CASE_SENSITIVE)) {
        free(pkg->handle);
        free(pkg);
        *errmsg = am_format("unable to open file %s", filename);
        return NULL;
    }
    pkg->filename = (char*)malloc(strlen(filename) + 1);
    strcpy(pkg->filename, filename);
    return pkg;
}

void am_close_package(am_package *pkg) {
    mz_zip_reader_end((mz_zip_archive*)pkg->handle);
    free(pkg->filename);
    free(pkg->handle);
    free(pkg);
}

void *am_read_package_resource(am_package *pkg, const char *filename, int *len, char **errmsg) {
    size_t sz;
    void *data = mz_zip_reader_extract_file_to_heap((mz_zip_archive*)pkg->handle,
        filename, &sz, MZ_ZIP_FLAG_CASE_SENSITIVE);
    if (data == NULL) {
        *errmsg = am_format("unable to read entry %s from package %s", filename, pkg->filename);
        return NULL;
    }
    *len = (int)sz;
    return data;
}

bool am_build_package(char *dir) {
    char *cwd = getcwd(NULL, 0);
    char *cmd = am_format("cd \"%s\" && rm -f \"%s/data.pak\" && "
        "zip -r -n .png:.ogg:.jpg \"%s/data.pak\" * -x \\*.pak \\*~",
        dir, cwd, cwd);
    am_log0("%s", cmd);
    int rc = system(cmd);
    free(cwd);
    if (rc != 0) {
        am_log0("unable to build pak file from %s", dir);
        return false;
    } else {
        return true;
    }
}
