#include "fmt/leaf/ar10_group/cz10_image_archive_decoder.h"
#include <array>
#include <boost/filesystem/path.hpp>
#include "err.h"
#include "fmt/naming_strategies.h"
#include "io/memory_stream.h"
#include "util/file_from_grid.h"
#include "util/format.h"
#include "util/pack/zlib.h"
#include "util/range.h"

using namespace au;
using namespace au::fmt::leaf;
namespace fs = boost::filesystem;

static const bstr magic = "cz10"_b;

namespace
{
    struct ArchiveEntryImpl final : fmt::ArchiveEntry
    {
        size_t offset, size;
        size_t width, height, channels;
    };
}

static bstr decompress(
    const bstr &input,
    const size_t width,
    const size_t height,
    const size_t channels)
{
    const auto stride = width * channels;

    bstr last_line(stride);
    bstr output;
    output.reserve(stride * height);

    io::MemoryStream input_stream(input);
    for (const auto y : util::range(height))
    {
        const auto control = input_stream.read_u8();
        bstr line;

        if (control == 0)
        {
            line = input_stream.read(stride);
        }
        else if (control == 1)
        {
            const auto chunk_size = input_stream.read_u16_be();
            const auto chunk = input_stream.read(chunk_size);
            line = util::pack::zlib_inflate(chunk);
        }
        else if (control == 2)
        {
            const auto chunk_size = input_stream.read_u16_be();
            const auto chunk = input_stream.read(chunk_size);
            line = util::pack::zlib_inflate(chunk);
            for (const auto i : util::range(1, stride))
                line[i] = line[i - 1] - line[i];
        }
        else if (control == 3)
        {
            const auto chunk_size = input_stream.read_u16_be();
            const auto chunk = input_stream.read(chunk_size);
            line = util::pack::zlib_inflate(chunk);
            for (const auto i : util::range(stride))
                line[i] = last_line[i] - line[i];
        }
        else
            throw err::CorruptDataError("Unexpected control byte");

        output += line;
        last_line = line;
    }

    return output;
}

bool Cz10ImageArchiveDecoder::is_recognized_impl(io::File &input_file) const
{
    return input_file.stream.read(magic.size()) == magic;
}

std::unique_ptr<fmt::ArchiveMeta>
    Cz10ImageArchiveDecoder::read_meta_impl(io::File &input_file) const
{
    auto meta = std::make_unique<ArchiveMeta>();

    input_file.stream.seek(magic.size());
    const auto image_count = input_file.stream.read_u32_le();
    auto current_offset = input_file.stream.tell() + image_count * 16;
    for (const auto i : util::range(image_count))
    {
        auto entry = std::make_unique<ArchiveEntryImpl>();
        entry->width = input_file.stream.read_u16_le();
        entry->height = input_file.stream.read_u16_le();
        input_file.stream.skip(4);
        entry->size = input_file.stream.read_u32_le();
        entry->channels = input_file.stream.read_u32_le();
        entry->offset = current_offset;
        current_offset += entry->size;
        meta->entries.push_back(std::move(entry));
    }

    const auto base_name = fs::path(input_file.name).stem().string();
    for (const auto i : util::range(meta->entries.size()))
        meta->entries[i]->name = meta->entries.size() > 1
            ? util::format("%s_%03d", base_name.c_str(), i)
            : base_name;

    return meta;
}

std::unique_ptr<io::File> Cz10ImageArchiveDecoder::read_file_impl(
    io::File &input_file, const ArchiveMeta &m, const ArchiveEntry &e) const
{
    const auto entry = static_cast<const ArchiveEntryImpl*>(&e);
    const auto data = decompress(
        input_file.stream.seek(entry->offset).read(entry->size),
        entry->width,
        entry->height,
        entry->channels);

    if (entry->channels != 4)
        throw err::UnsupportedChannelCountError(entry->channels);

    pix::Grid image(entry->width, entry->height);
    const auto *data_ptr = data.get<const u8>();
    for (const auto y : util::range(entry->height))
    for (const auto c : util::range(entry->channels))
    for (const auto x : util::range(entry->width))
    {
        image.at(x, y)[c] = *data_ptr++;
    }
    return util::file_from_grid(image, entry->name);
}

std::unique_ptr<fmt::INamingStrategy>
    Cz10ImageArchiveDecoder::naming_strategy() const
{
    return std::make_unique<SiblingNamingStrategy>();
}

static auto dummy = fmt::register_fmt<Cz10ImageArchiveDecoder>("leaf/cz10");