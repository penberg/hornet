#ifndef HORNET_ZIP_HH
#define HORNET_ZIP_HH

#include <unordered_map>
#include <cstdint>
#include <cstddef>
#include <string>

namespace hornet {

//
// ZIP entry in memory.
//
struct zip_entry {
    zip_entry();
    ~zip_entry();

    char *filename;
    uint32_t comp_size;
    uint32_t uncomp_size;
    uint32_t lh_offset;
    uint16_t compression;
};

//
// ZIP file in memory.
//
struct zip {
    int fd;
    size_t len;
    char* mmap;
    unsigned long nr_entries;
    struct zip_entry* entries;
    std::unordered_map<std::string, zip_entry*> entry_cache;
};

struct zip *zip_open(const char *pathname);
void zip_close(struct zip *zip);
struct zip_entry *zip_entry_find(struct zip *zip, const char *filename);
struct zip_entry *zip_entry_find_class(struct zip *zip, const char *classname);
void *zip_entry_data(struct zip *zip, struct zip_entry *entry);

}

#endif
