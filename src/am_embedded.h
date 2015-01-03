struct am_embedded_file_record {
    const char *filename;
    const uint8_t *data;
    size_t len;
};

extern am_embedded_file_record am_embedded_files[];

am_embedded_file_record *am_get_embedded_file(const char *filename);
