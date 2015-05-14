#include "hornet/zip.hh"

#include "hornet/byte-order.hh"

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <zlib.h>

namespace hornet {

/*
 *	On-disk data structures
 */

/*
 * Local file header
 */

#define ZIP_LFH_SIGNATURE	0x04034b50

struct zip_lfh {
    uint32_t		signature;
    uint16_t		version;
    uint16_t		flags;
    uint16_t		compression;
    uint16_t		mod_time;
    uint16_t		mod_date;
    uint32_t		crc32;
    uint32_t		comp_size;
    uint32_t		uncomp_size;
    uint16_t		filename_len;
    uint16_t		extra_field_len;
} __attribute__((packed));

/*
 *	Central directory file header
 */

#define ZIP_CDSFH_SIGNATURE	0x02014b50

struct zip_cdfh {
    uint32_t		signature;
    uint16_t		version;
    uint16_t		version_needed;
    uint16_t		flags;
    uint16_t		compression;
    uint16_t		mod_time;
    uint16_t		mod_date;
    uint32_t		crc32;
    uint32_t		comp_size;
    uint32_t		uncomp_size;
    uint16_t		filename_len;
    uint16_t		extra_field_len;
    uint16_t		file_comm_len;
    uint16_t		disk_start;
    uint16_t		internal_attr;
    uint32_t		external_attr;
    uint32_t		lh_offset;	/* local header offset */
} __attribute__((packed));

/*
 *	End of central directory record
 */
#define ZIP_EOCDR_SIGNATURE	0x06054b50

struct zip_eocdr {
    uint32_t		signature;
    uint16_t		disk_number;
    uint16_t		cd_disk_number;
    uint16_t		disk_entries;
    uint16_t		total_entries;
    uint32_t		cd_size;	/* central directory size */
    uint32_t		offset;
    uint16_t		comment_len;
} __attribute__((packed));

zip_entry::zip_entry()
    : filename(nullptr)
{
}

zip_entry::~zip_entry()
{
    free(filename);
}

static inline const char *cdfh_filename(struct zip_cdfh *cdfh)
{
    return reinterpret_cast<char*>(cdfh) + sizeof(*cdfh);
}

void zip_close(struct zip *zip)
{
    if (!zip)
        return;

    munmap(zip->mmap, zip->len);

    close(zip->fd);

    delete [] zip->entries;

    delete zip;
}

static size_t zip_lfh_size(struct zip_lfh *lfh)
{
    return sizeof *lfh + le16_to_cpu(lfh->filename_len) + le16_to_cpu(lfh->extra_field_len);
}

static size_t zip_cdfh_size(struct zip_cdfh *cdfh)
{
    return sizeof *cdfh
           + le16_to_cpu(cdfh->filename_len)
           + le16_to_cpu(cdfh->extra_field_len)
           + le16_to_cpu(cdfh->file_comm_len);
}

static int zip_eocdr_traverse(struct zip *zip, struct zip_eocdr *eocdr)
{
    unsigned long nr_entries, nr = 0;
    unsigned int idx;
    char *p;

    nr_entries = le16_to_cpu(eocdr->total_entries);

    zip->entries = new zip_entry[nr_entries];

    p = zip->mmap + le32_to_cpu(eocdr->offset);

    for (idx = 0; idx < nr_entries; idx++) {
        struct zip_cdfh *cdfh = reinterpret_cast<zip_cdfh*>(p);
        struct zip_entry *entry;
        uint16_t filename_len;
        const char *filename;
        char *s;

        if (le32_to_cpu(cdfh->signature) != ZIP_CDSFH_SIGNATURE)
            goto error;

        filename_len = le16_to_cpu(cdfh->filename_len);

        filename = cdfh_filename(cdfh);
        if (filename[filename_len - 1] == '/')
            goto next;

        s = strndup(filename, filename_len);
        if (!s)
            goto error;

        entry = &zip->entries[nr++];

        entry->filename		= s;
        entry->comp_size	= le32_to_cpu(cdfh->comp_size);
        entry->uncomp_size	= le32_to_cpu(cdfh->uncomp_size);
        entry->lh_offset	= le32_to_cpu(cdfh->lh_offset);
        entry->compression	= le16_to_cpu(cdfh->compression);

        zip->entry_cache.emplace(s, entry);
next:
        p += zip_cdfh_size(cdfh);
    }

    zip->nr_entries = nr;

    return 0;

error:
    return -1;
}

static struct zip_eocdr *zip_eocdr_find(struct zip *zip)
{
    char *p;

    for (p = zip->mmap + zip->len - sizeof(uint32_t); p >= zip->mmap; p--) {
        uint32_t *sig = reinterpret_cast<uint32_t*>(p);

        if (le32_to_cpu(*sig) == ZIP_EOCDR_SIGNATURE)
            return reinterpret_cast<zip_eocdr*>(p);
    }

    return nullptr;
}

static struct zip *zip_do_open(const char *pathname)
{
    struct stat st;

    auto zp = new zip;

    zp->fd = open(pathname, O_RDONLY);
    if (zp->fd < 0)
        goto error_free;

    if (fstat(zp->fd, &st) < 0)
        goto error_free;

    zp->len = st.st_size;

    zp->mmap = static_cast<char*>(mmap(nullptr, zp->len, PROT_READ, MAP_SHARED, zp->fd, 0));
    if (zp->mmap == MAP_FAILED)
        goto error_close;

    return zp;

error_close:
    close(zp->fd);

error_free:
    delete zp;

    return nullptr;
}

struct zip *zip_open(const char *pathname)
{
    struct zip_eocdr *eocdr;
    uint32_t *lfh_sig;
    struct zip *zip;

    zip = zip_do_open(pathname);
    if (!zip)
        return nullptr;

    lfh_sig = reinterpret_cast<uint32_t*>(zip->mmap);

    if (le32_to_cpu(*lfh_sig) != ZIP_LFH_SIGNATURE)
        goto error;

    eocdr = zip_eocdr_find(zip);
    if (!eocdr)
        goto error;

    if (zip_eocdr_traverse(zip, eocdr) < 0)
        goto error;

    return zip;

error:
    zip_close(zip);

    return nullptr;
}

void *zip_entry_data(struct zip *zip, struct zip_entry *entry)
{
    struct zip_lfh *lfh;
    void *output;
    void *input;

    lfh = reinterpret_cast<zip_lfh*>(zip->mmap + entry->lh_offset);

    input = zip->mmap + entry->lh_offset + zip_lfh_size(lfh);

    output = malloc(entry->uncomp_size);
    if (!output)
        return nullptr;

    switch (entry->compression) {
    case Z_DEFLATED: {
        z_stream zs;
        int err;

        zs.next_in	= static_cast<Bytef*>(input);
        zs.avail_in	= entry->comp_size;
        zs.next_out	= static_cast<Bytef*>(output);
        zs.avail_out	= entry->uncomp_size;

        zs.zalloc	= Z_NULL;
        zs.zfree	= Z_NULL;
        zs.opaque	= Z_NULL;

        if (inflateInit2(&zs, -MAX_WBITS) != Z_OK)
            goto error;

        err = inflate(&zs, Z_SYNC_FLUSH);
        if ((err != Z_STREAM_END) && (err != Z_OK))
            goto error;

        if (inflateEnd(&zs) != Z_OK)
            goto error;

        break;
    }
    case 0: {
        memcpy(output, input, entry->comp_size);
        break;
    }
    default:
        goto error;
    }

    return output;
error:
    free(output);
    return nullptr;
}

struct zip_entry *zip_entry_find(struct zip *zip, const char *pathname)
{
    auto it = zip->entry_cache.find(pathname);
    if (it != zip->entry_cache.end())
        return it->second;
    return nullptr;
}

struct zip_entry *zip_entry_find_class(struct zip *zip, const char *classname)
{
    std::string entry_name = std::string{classname} + ".class";
    auto it = zip->entry_cache.find(entry_name);
    if (it != zip->entry_cache.end())
        return it->second;
    return nullptr;
}

}
