// Copyright (C) 2016 by rr-
//
// This file is part of arc_unpacker.
//
// arc_unpacker is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or (at
// your option) any later version.
//
// arc_unpacker is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with arc_unpacker. If not, see <http://www.gnu.org/licenses/>.

#pragma once

#include <memory>
#include "err.h"
#include "io/base_byte_stream.h"
#include "io/base_stream.h"

namespace au {
namespace io {

    class MemoryByteStream final : public BaseByteStream
    {
    public:
        MemoryByteStream();
        MemoryByteStream(const char *buffer, const size_t buffer_size);
        MemoryByteStream(const bstr &buffer);
        MemoryByteStream(BaseByteStream &other_stream, const size_t size);
        MemoryByteStream(BaseByteStream &other_stream);
        ~MemoryByteStream();

        uoff_t size() const override;
        uoff_t pos() const override;

        BaseByteStream &reserve(const uoff_t count);

        std::unique_ptr<BaseByteStream> clone() const override;

    protected:
        void read_impl(void *destination, const size_t size) override;
        void write_impl(const void *source, const size_t size) override;
        void seek_impl(const uoff_t offset) override;
        void resize_impl(const uoff_t new_size) override;

    private:
        MemoryByteStream(const std::shared_ptr<bstr> buffer);

        std::shared_ptr<bstr> buffer;
        uoff_t buffer_pos;
    };

} }
