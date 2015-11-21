#pragma once

#include "fmt/archive_decoder.h"

namespace au {
namespace fmt {
namespace leaf {

    class Pak1ArchiveDecoder final : public ArchiveDecoder
    {
    public:
        Pak1ArchiveDecoder();
        ~Pak1ArchiveDecoder();
        void register_cli_options(ArgParser &arg_parser) const;
        void parse_cli_options(const ArgParser &arg_parser);
        void set_version(const int version);
        std::vector<std::string> get_linked_formats() const override;

    protected:
        bool is_recognized_impl(io::File &input_file) const override;

        std::unique_ptr<ArchiveMeta> read_meta_impl(
            io::File &input_file) const override;

        std::unique_ptr<io::File> read_file_impl(
            io::File &input_file,
            const ArchiveMeta &m,
            const ArchiveEntry &e) const override;

        void preprocess(
            io::File &input_file,
            ArchiveMeta &m,
            const FileSaver &file_saver) const override;

    private:
        struct Priv;
        std::unique_ptr<Priv> p;
    };

} } }