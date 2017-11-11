//
//  src/mach-o/container.cc
//  tbd
//
//  Created by inoahdev on 4/24/17.
//  Copyright © 2017 inoahdev. All rights reserved.
//

#include <cstdio>
#include <cstdlib>

#include "headers/symbol_table.h"
#include "container.h"

namespace macho {
    container::container(const container &container) noexcept :
    stream(container.stream), base(container.base), size(container.size), header(container.header) {}

    container::container(container &&container) noexcept :
    stream(container.stream), base(container.base), size(container.size), header(container.header),
    cached_load_commands_(container.cached_load_commands_), cached_symbol_table_(container.cached_symbol_table_), cached_string_table_(container.cached_string_table_) {
        container.cached_load_commands_ = nullptr;
        container.cached_symbol_table_ = nullptr;
        container.cached_string_table_ = nullptr;
    }

    container &container::operator=(const container &container) noexcept {
        stream = container.stream;

        base = container.base;
        size = container.size;

        header = container.header;
        return *this;
    }

    container &container::operator=(container &&container) noexcept {
        stream = std::move(container.stream);

        base = container.base;
        size = container.size;

        header = container.header;

        cached_load_commands_ = container.cached_load_commands_;
        cached_symbol_table_ = container.cached_symbol_table_;
        cached_string_table_ = container.cached_string_table_;

        container.cached_load_commands_ = nullptr;
        container.cached_symbol_table_ = nullptr;
        container.cached_string_table_ = nullptr;
        
        return *this;
    }

    container::open_result container::open(const stream::file::shared &stream, long base, size_t size) noexcept {
        this->stream = stream;

        this->base = base;
        this->size = size;

        return validate_and_load_data();
    }

    container::open_result container::open_from_library(const stream::file::shared &stream, long base, size_t size) noexcept {
        this->stream = stream;

        this->base = base;
        this->size = size;

        return validate_and_load_data<validation_type::as_library>();
    }

    container::open_result container::open_from_dynamic_library(const stream::file::shared &stream, long base, size_t size) noexcept {
        this->stream = stream;

        this->base = base;
        this->size = size;

        return validate_and_load_data<validation_type::as_dynamic_library>();
    }

    container::open_result container::open_copy(const container &container) noexcept {
        stream = container.stream;

        base = container.base;
        size = container.size;

        return validate_and_load_data();
    }

    container::~container() noexcept {
        if (cached_load_commands_ != nullptr) {
            delete[] cached_load_commands_;
        }

        if (cached_symbol_table_ != nullptr) {
            delete[] cached_symbol_table_;
        }

        if (cached_string_table_ != nullptr) {
            delete[] cached_string_table_;
        }
    }

    size_t container::file_size(container::open_result &result) noexcept {
        const auto position = stream.position();
        if (!stream.seek(0, stream::file::seek_type::end)) {
            result = open_result::stream_seek_error;
            return 0;
        }

        auto size = static_cast<size_t>(stream.position());
        if (size < base) {
            result = open_result::invalid_range;
            return size;
        }

        if (!stream.seek(position, stream::file::seek_type::beginning)) {
            result = open_result::stream_seek_error;
            return size;
        }

        result = open_result::ok;
        return size;
    }

    struct load_command *container::find_first_of_load_command(load_commands cmd, container::load_command_iteration_result *result) {
        const auto magic_is_big_endian = is_big_endian();

        const auto &ncmds = header.ncmds;
        const auto &sizeofcmds = header.sizeofcmds;

        if (!ncmds || !sizeofcmds) {
            if (result != nullptr) {
                *result = load_command_iteration_result::no_load_commands;
            }

            return nullptr;
        }

        auto &cached_load_commands = cached_load_commands_;
        const auto created_cached_load_commands = !cached_load_commands;

        auto load_command_base = base + sizeof(header);
        auto magic_is_64_bit = is_64_bit();

        if (magic_is_64_bit) {
            load_command_base += sizeof(uint32_t);
        }

        if (!cached_load_commands) {
            const auto position = stream.position();

            if (!stream.seek(load_command_base, stream::file::seek_type::beginning)) {
                if (result != nullptr) {
                    *result = load_command_iteration_result::stream_seek_error;
                }

                return nullptr;
            }

            cached_load_commands = new uint8_t[sizeofcmds];

            if (!stream.read(cached_load_commands, sizeofcmds)) {
                delete[] cached_load_commands;
                cached_load_commands = nullptr;

                if (result != nullptr) {
                    *result = load_command_iteration_result::stream_read_error;
                }

                return nullptr;
            }

            if (!stream.seek(position, stream::file::seek_type::beginning)) {
                if (result != nullptr) {
                    *result = load_command_iteration_result::stream_seek_error;
                }

                return nullptr;
            }
        }

        auto size_used = uint32_t();
        for (auto i = uint32_t(), cached_load_commands_index = uint32_t(); i < ncmds; i++) {
            auto load_command = (struct load_command *)&cached_load_commands[cached_load_commands_index];
            auto swapped_load_command = *load_command;

            if (magic_is_big_endian) {
                swap_load_command(swapped_load_command);
            }

            if (created_cached_load_commands) {
                if (swapped_load_command.cmdsize < sizeof(struct load_command)) {
                    if (result != nullptr) {
                        *result = load_command_iteration_result::load_command_is_too_small;
                    }

                    return nullptr;
                }

                size_used += swapped_load_command.cmdsize;
                if (size_used > sizeofcmds || (size_used == sizeofcmds && i != ncmds - 1)) {
                    if (result != nullptr) {
                        *result = load_command_iteration_result::load_command_is_too_large;
                    }

                    return nullptr;
                }
            }

            if (swapped_load_command.cmd == cmd) {
                return load_command;
            }

            cached_load_commands_index += swapped_load_command.cmdsize;
        }

        return nullptr;
    }
}
