#include <stdlib.h>
#include <string.h>
#include "formats/arc/mbl_archive.h"
#include "assert_ex.h"
#include "logger.h"
#include "string_ex.h"

typedef struct
{
    IO *io;
    char *name;
    uint32_t offset;
    uint32_t size;
} TableEntry;

static int get_version(IO *io)
{
    uint32_t file_count = io_read_u32_le(io);
    if (!io_skip(io, file_count * (16 + 8) - 8))
        return -1;
    uint32_t last_file_offset = io_read_u32_le(io);
    uint32_t last_file_size = io_read_u32_le(io);
    io_seek(io, 0);
    return last_file_offset + last_file_size == io_size(io) ? 1 : 2;
}

static VirtualFile *read_file(void *context)
{
    VirtualFile *vf = vf_create();
    TableEntry *table_entry = (TableEntry*)context;
    io_seek(table_entry->io, table_entry->offset);
    char *data = io_read_string(table_entry->io, table_entry->size);
    assert_not_null(data);
    vf_set_data(vf, data, table_entry->size);
    vf_set_name(vf, table_entry->name);
    free(data);
    return vf;
}

static bool unpack(
    __attribute__ ((unused)) Archive *archive,
    IO *io,
    OutputFiles *output_files)
{
    char *tmp_name;
    size_t i, old_pos;
    int version = get_version(io);
    if (version == -1)
    {
        log_error("Not a MBL archive");
        return false;
    }
    log_info("Version: %d", version);

    uint32_t file_count = io_read_u32_le(io);
    uint32_t name_length = version == 2 ? io_read_u32_le(io) : 16;
    for (i = 0; i < file_count; i ++)
    {
        TableEntry *entry = (TableEntry*)malloc(sizeof(TableEntry));
        assert_not_null(entry);

        old_pos = io_tell(io);
        tmp_name = io_read_until_zero(io);
        assert_not_null(tmp_name);
        assert_that(convert(
            tmp_name, strlen(tmp_name),
            &entry->name, NULL,
            "sjis", "utf-8"));
        free(tmp_name);
        io_seek(io, old_pos + name_length);

        entry->offset = io_read_u32_le(io);
        entry->size = io_read_u32_le(io);
        if (entry->offset + entry->size > io_size(io))
        {
            log_error("Bad offset to file");
            free(entry);
            free(entry->name);
            return false;
        }
        entry->io = io;
        old_pos = io_tell(io);
        output_files_save(output_files, &read_file, (void*)entry);
        io_seek(io, old_pos);
        free(entry->name);
        free(entry);
    }

    return true;
}

Archive *mbl_archive_create()
{
    Archive *archive = archive_create();
    archive->unpack = &unpack;
    return archive;
}
