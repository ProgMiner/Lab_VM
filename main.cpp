#include <iostream>
#include <fstream>
#include <cstdint>
#include <memory>


/* Bytecode file layout
 * 
 * Header:
 * +----------------------------------+
 * | String pool size in bytes        | 32-bit little endian
 * +----------------------------------+
 * | Global area size in 32-bit words | 32-bit little endian
 * +----------------------------------+
 * | Number of public symbols         | 32-bit little endian
 * +----------------------------------+
 * | Public symbols table             | Array of Public symbols (see below)
 * +----------------------------------+
 * | String pool                      | Concatenated NUL-terminated strings
 * +----------------------------------+
 * | Bytecode                         |
 * +----------------------------------+
 *
 * Public symbol:
 * +--------------------------------+
 * | Offset of name in string pool  | 32-bit little endian
 * +--------------------------------+
 * | Offset of bytecode in bytecode | 32-bit little endian
 * +--------------------------------+
 *
 */

struct public_entry {

    uint32_t name_pos;
    uint32_t code_pos;
} __attribute__((packed));

struct bytecode_contents {

    const public_entry * public_ptr;
    const char * string_ptr;
    const char * code_ptr;
    uint32_t string_size;
    uint32_t global_size;
    uint32_t public_size;
} __attribute__((packed));


static std::shared_ptr<bytecode_contents> read_bytecode(const char * filename) {
    std::ifstream bytecode_file { filename, std::ios::in | std::ios::binary | std::ios::ate };
    const std::size_t bytecode_file_size = bytecode_file.tellg();
    bytecode_file.seekg(0);

    std::shared_ptr<uint8_t> bytecode_buffer {
        new uint8_t[offsetof(bytecode_contents, string_size) + bytecode_file_size],
        std::default_delete<uint8_t[]> {},
    };

    auto bytecode = std::reinterpret_pointer_cast<bytecode_contents>(bytecode_buffer);

    bytecode_file.get(reinterpret_cast<char *>(&bytecode->string_size), bytecode_file_size);

    bytecode->public_ptr = reinterpret_cast<const public_entry *>(
        bytecode_buffer.get() + sizeof(bytecode_contents)
    );

    bytecode->string_ptr = reinterpret_cast<const char *>(
        bytecode->public_ptr + bytecode->public_size
    );

    bytecode->code_ptr = bytecode->string_ptr + bytecode->string_size;
    return bytecode;
}

int main(int argc, char * argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <bytecode file>" << std::endl;
        return 0;
    }

    auto bytecode = read_bytecode(argv[1]);

    std::cerr << "String pool size: " << bytecode->string_size << std::endl;
    std::cerr << "Global area size: " << bytecode->global_size << std::endl;

    std::cerr << "Public names:" << std::endl;
    for (std::size_t i = 0; i < bytecode->public_size; ++i) {
        auto public_entry = bytecode->public_ptr[i];

        std::cout << bytecode->string_ptr + public_entry.name_pos
                  << " -> " << reinterpret_cast<void *>(public_entry.code_pos)
                  << std::endl;
    }

    // TODO
}
