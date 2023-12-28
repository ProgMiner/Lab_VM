#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>
#include <list>
#include <new>


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
    const uint8_t * code_ptr;
    std::size_t code_size;
    uint32_t string_size;
    uint32_t global_size;
    uint32_t public_size;
} __attribute__((packed));

// instruction code
namespace IC {

enum IC {

    BINOP_add       = 0x01, // BINOP +
    BINOP_sub       = 0x02, // BINOP -
    BINOP_mul       = 0x03, // BINOP *
    BINOP_div       = 0x04, // BINOP /
    BINOP_rem       = 0x05, // BINOP %
    BINOP_lt        = 0x06, // BINOP <
    BINOP_le        = 0x07, // BINOP <=
    BINOP_gt        = 0x08, // BINOP >
    BINOP_ge        = 0x09, // BINOP >=
    BINOP_eq        = 0x0A, // BINOP ==
    BINOP_ne        = 0x0B, // BINOP !=
    BINOP_and       = 0x0C, // BINOP &&
    BINOP_or        = 0x0D, // BINOP !!
    CONST           = 0x10, // CONST, int
    STRING          = 0x11, // STRING, string
    SEXP            = 0x12, // SEXP, string, int
    STI             = 0x13, // STI
    STA             = 0x14, // STA
    JMP             = 0x15, // JMP, int
    END             = 0x16, // END
    RET             = 0x17, // RET
    DROP            = 0x18, // DROP
    DUP             = 0x19, // DUP
    SWAP            = 0x1A, // SWAP
    ELEM            = 0x1B, // ELEM
    LD_G            = 0x20, // LD, G(int)
    LD_L            = 0x21, // LD, L(int)
    LD_A            = 0x22, // LD, A(int)
    LD_C            = 0x23, // LD, C(int)
    LDA_G           = 0x30, // LDA, G(int)
    LDA_L           = 0x31, // LDA, L(int)
    LDA_A           = 0x32, // LDA, A(int)
    LDA_C           = 0x33, // LDA, C(int)
    ST_G            = 0x40, // ST, G(int)
    ST_L            = 0x41, // ST, L(int)
    ST_A            = 0x42, // ST, A(int)
    ST_C            = 0x43, // ST, C(int)
    CJMPz           = 0x50, // CJMPz, int
    CJMPnz          = 0x51, // CJMPnz, int
    BEGIN           = 0x52, // BEGIN, int, int
    CBEGIN          = 0x53, // CBEGIN, int, int
    CLOSURE         = 0x54, // CLOSURE, int, n: int, [byte, int][n]
    CALLC           = 0x55, // CALLC, int
    CALL            = 0x56, // CALL, int, int
    TAG             = 0x57, // TAG, string, int
    ARRAY           = 0x58, // ARRAY, int
    FAIL            = 0x59, // FAIL, int, int
    LINE            = 0x5A, // LINE, int
    PATT_str        = 0x60, // PATT =str
    PATT_string     = 0x61, // PATT #string
    PATT_array      = 0x62, // PATT #array
    PATT_sexp       = 0x63, // PATT #sexp
    PATT_ref        = 0x64, // PATT #ref
    PATT_val        = 0x65, // PATT #val
    PATT_fun        = 0x66, // PATT #fun
    CALL_Lread      = 0x70, // CALL Lread
    CALL_Lwrite     = 0x71, // CALL Lwrite
    CALL_Llength    = 0x72, // CALL Llength
    CALL_Lstring    = 0x73, // CALL Lstring
    CALL_Barray     = 0x74, // CALL Barray, int
};

}

union value;

struct heap_object {

    enum object_kind {

        STRING = 0,
        ARRAY  = 1,
        SEXP   = 2,
    };

    uint8_t kind;
    std::size_t fields_size;

    object_kind get_kind() const {
        switch (kind) {
        case STRING:
        case ARRAY:
        case SEXP:
            return static_cast<object_kind>(kind);

        default:
            throw std::invalid_argument { "unknown object kind" };
        }
    }

    value & get_field(std::size_t idx) noexcept;
};

union value {

    uint32_t fixnum;
    heap_object * object;
    value ** var;

    value() noexcept = default;
    explicit value(uint32_t fixnum) noexcept: fixnum(fixnum) {}
    explicit value(heap_object * object) noexcept: object(object) {}
};

// converted code
union CC {

    const void * interpreter;

    uint32_t fixnum;
    const char * string;
    const CC * code;
    value * global;
    uint32_t index;
    const uint8_t * args;
};

static_assert(sizeof(value) == sizeof(void *));
static_assert(sizeof(CC) == sizeof(void *));

struct instruction_meta {

    const CC * converted = nullptr;
    std::size_t function_idx = UINT32_MAX;
    std::size_t stack_depth = UINT32_MAX;
    std::list<CC *> forward_ptrs;
};

struct activation {

    activation * parent;
    const CC * return_ptr;
    std::size_t locals_size;

    value & local(std::size_t idx) noexcept {
        return *(
            reinterpret_cast<value *>(reinterpret_cast<uint8_t *>(this) + sizeof(activation))
                + idx
        );
    }

    static activation * create(activation * parent, const CC * return_ptr, std::size_t locals) {
        auto result = reinterpret_cast<activation *>(
            // zero-initialized
            new uint8_t[sizeof(activation) + sizeof(value) * locals] {}
        );

        result->parent = parent;
        result->return_ptr = return_ptr;
        result->locals_size = locals;
        return result;
    }

    static void drop(activation * act) {
        delete[] reinterpret_cast<uint8_t *>(act);
    }
};

static bool is_fixnum(uint32_t x) noexcept;

struct heap {

    value * global_area;
    std::size_t global_area_size;

    std::vector<value> & stack;

    activation * & current_activation;

    std::unique_ptr<uint8_t[], std::default_delete<uint8_t[]>> buffer;
    uint8_t * first_half;
    uint8_t * second_half;

    std::size_t offset = 0;
    std::size_t size;

    heap(
        value * global_area,
        std::size_t global_area_size,
        std::vector<value> & stack,
        activation * & current_activation
    )
        : global_area(global_area)
        , global_area_size(global_area_size)
        , stack(stack)
        , current_activation(current_activation)
        , buffer(new uint8_t[32])
        , first_half(buffer.get())
        , second_half(first_half + 16)
        , size(32)
    {
        if (!buffer) {
            throw std::bad_alloc {};
        }
    }

    static constexpr std::size_t align_size(std::size_t size) {
        return (size + 15) / 16 * 16;
    }

    heap_object * alloc(std::size_t object_size) {
        std::size_t real_object_size = align_size(sizeof(heap_object)) + align_size(object_size);
        request(real_object_size);

        auto * const result = reinterpret_cast<heap_object *>(first_half + offset);
        offset += real_object_size;

        std::memset(result, 0, real_object_size);
        return result;
    }

    bool is_heap_value(value val) noexcept {
        if (is_fixnum(val.fixnum)) {
            return false;
        }

        const auto * const raw_ptr = reinterpret_cast<const uint8_t *>(val.object);
        if (raw_ptr < first_half || raw_ptr >= first_half + offset) {
            return false;
        }

        return true;
    }

    void assert_heap_value(value val) {
        if (!is_heap_value(val)) {
            throw std::invalid_argument { "argument is not heap value" };
        }
    }

    void request(std::size_t object_size) {
        if (offset + object_size <= size / 2) {
            return;
        }

        gc();

        while (offset + object_size > size / 2) {
            grow();
        }
    }

    void gc() {
        std::size_t new_offset = 0;
        std::unordered_map<const heap_object *, heap_object *> moved_objects;

        for (std::size_t i = 0; i < global_area_size; ++i) {
            move_value(global_area[i], new_offset, moved_objects);
        }

        for (value & v : stack) {
            move_value(v, new_offset, moved_objects);
        }

        {
            activation * act = current_activation;

            while (act) {
                for (std::size_t i = 0; i < act->locals_size; ++i) {
                    move_value(act->local(i), new_offset, moved_objects);
                }

                act = act->parent;
            }
        }

        std::swap(first_half, second_half);
        offset = new_offset;
    }

    void move_value(
        value & val,
        std::size_t & new_offset,
        std::unordered_map<const heap_object *, heap_object *> & moved_objects
    ) {
        if (!is_heap_value(val)) {
            return;
        }

        heap_object * const heap_ptr = val.object;
        if (auto it = moved_objects.find(heap_ptr); it != moved_objects.end()) {
            val.object = it->second;
            return;
        }

        const heap_object::object_kind k = heap_ptr->get_kind();
        const std::size_t object_size = align_size(sizeof(heap_object))
            + align_size(k == heap_object::STRING ? heap_ptr->fields_size
                : sizeof(value) * heap_ptr->fields_size);

        heap_object * const new_ptr = reinterpret_cast<heap_object *>(second_half + new_offset);
        new_offset += object_size;

        std::memcpy(new_ptr, heap_ptr, object_size);

        moved_objects[heap_ptr] = new_ptr;
        val.object = new_ptr;

        if (k == heap_object::STRING) {
            return;
        }

        for (std::size_t i = 0; i < new_ptr->fields_size; ++i) {
            move_value(new_ptr->get_field(i), new_offset, moved_objects);
        }
    }

    void grow() {
        uint8_t * new_buffer = new uint8_t[size * 2];

        if (!new_buffer) {
            throw std::bad_alloc {};
        }

        second_half = new_buffer;
        gc();

        second_half = new_buffer + size;
        size *= 2;

        buffer.reset(new_buffer);
    }
};


static std::ofstream log_file { "./log.txt" };


value & heap_object::get_field(std::size_t idx) noexcept {
    return *(reinterpret_cast<value *>(reinterpret_cast<uint8_t *>(this)
        + heap::align_size(sizeof(activation))) + idx);
}

static uint32_t to_fixnum(int32_t x) noexcept {
    return static_cast<uint32_t>(x << 1) | 1;
}

static int32_t from_fixnum(uint32_t x) noexcept {
    return static_cast<int32_t>(x) >> 1;
}

static bool is_fixnum(uint32_t x) noexcept {
    return (x & 1) != 0;
}

template<typename T>
static const T & read_from_bytes(const uint8_t * buffer, std::size_t & idx) {
    const T & result = *reinterpret_cast<const T *>(buffer + idx);
    idx += sizeof(T);
    return result;
}

template<typename T>
static const T & read_from_bytes(const uint8_t * buffer) {
    return *reinterpret_cast<const T *>(buffer);
}

static std::shared_ptr<bytecode_contents> read_bytecode(const char * filename) {
    std::ifstream bytecode_file { filename, std::ios::in | std::ios::binary | std::ios::ate };

    if (!bytecode_file) {
        throw std::invalid_argument { "cannot open file" };
    }

    const std::size_t bytecode_file_size = bytecode_file.tellg();
    bytecode_file.seekg(0);

    std::shared_ptr<uint8_t[]> bytecode_buffer {
        new uint8_t[offsetof(bytecode_contents, string_size) + bytecode_file_size],
        std::default_delete<uint8_t[]> {},
    };

    auto bytecode = std::reinterpret_pointer_cast<bytecode_contents>(bytecode_buffer);

    bytecode_file.read(reinterpret_cast<char *>(&bytecode->string_size), bytecode_file_size);

    bytecode->code_size = bytecode_file_size - (
        (sizeof(bytecode_contents) - offsetof(bytecode_contents, string_size)) // size of header
        + bytecode->public_size * sizeof(public_entry) // size of public table
        + bytecode->string_size
    );

    bytecode->public_ptr = reinterpret_cast<const public_entry *>(
        bytecode_buffer.get() + sizeof(bytecode_contents)
    );

    bytecode->string_ptr = reinterpret_cast<const char *>(
        bytecode->public_ptr + bytecode->public_size
    );

    bytecode->code_ptr = reinterpret_cast<const uint8_t *>(
        bytecode->string_ptr + bytecode->string_size
    );

    return bytecode;
}

static void interpret(const std::shared_ptr<bytecode_contents> & bytecode) {
    std::unique_ptr<CC[], std::default_delete<CC[]>> converted {
        new CC[bytecode->code_size],
    };

    std::unique_ptr<value[], std::default_delete<value[]>> global_area {
        new value[bytecode->global_size],
    };

    const CC * const converted_base = converted.get();
    value * const global_area_base = global_area.get();

    {
        const void * interpreter_ptrs[256] = {};
        interpreter_ptrs[IC::BINOP_add] = &&I_BINOP_add;
        interpreter_ptrs[IC::BINOP_sub] = &&I_BINOP_sub;
        interpreter_ptrs[IC::BINOP_mul] = &&I_BINOP_mul;
        interpreter_ptrs[IC::BINOP_div] = &&I_BINOP_div;
        interpreter_ptrs[IC::BINOP_rem] = &&I_BINOP_rem;
        interpreter_ptrs[IC::BINOP_lt] = &&I_BINOP_lt;
        interpreter_ptrs[IC::BINOP_le] = &&I_BINOP_le;
        interpreter_ptrs[IC::BINOP_gt] = &&I_BINOP_gt;
        interpreter_ptrs[IC::BINOP_ge] = &&I_BINOP_ge;
        interpreter_ptrs[IC::BINOP_eq] = &&I_BINOP_eq;
        interpreter_ptrs[IC::BINOP_ne] = &&I_BINOP_ne;
        interpreter_ptrs[IC::BINOP_and] = &&I_BINOP_and;
        interpreter_ptrs[IC::BINOP_or] = &&I_BINOP_or;
        interpreter_ptrs[IC::CONST] = &&I_CONST;
        interpreter_ptrs[IC::STRING] = &&I_STRING;
        interpreter_ptrs[IC::SEXP] = &&I_SEXP;
        interpreter_ptrs[IC::STI] = &&I_STI;
        interpreter_ptrs[IC::STA] = &&I_STA;
        interpreter_ptrs[IC::JMP] = &&I_JMP;
        interpreter_ptrs[IC::END] = &&I_END;
        interpreter_ptrs[IC::RET] = &&I_RET;
        interpreter_ptrs[IC::DROP] = &&I_DROP;
        interpreter_ptrs[IC::DUP] = &&I_DUP;
        interpreter_ptrs[IC::SWAP] = &&I_SWAP;
        interpreter_ptrs[IC::ELEM] = &&I_ELEM;
        interpreter_ptrs[IC::LD_G] = &&I_LD_G;
        interpreter_ptrs[IC::LD_L] = &&I_LD_AL;
        interpreter_ptrs[IC::LD_A] = &&I_LD_AL;
        interpreter_ptrs[IC::LD_C] = &&I_LD_C;
        interpreter_ptrs[IC::LDA_G] = &&I_LDA_G;
        interpreter_ptrs[IC::LDA_L] = &&I_LDA_AL;
        interpreter_ptrs[IC::LDA_A] = &&I_LDA_AL;
        interpreter_ptrs[IC::LDA_C] = &&I_LDA_C;
        interpreter_ptrs[IC::ST_G] = &&I_ST_G;
        interpreter_ptrs[IC::ST_L] = &&I_ST_AL;
        interpreter_ptrs[IC::ST_A] = &&I_ST_AL;
        interpreter_ptrs[IC::ST_C] = &&I_ST_C;
        interpreter_ptrs[IC::CJMPz] = &&I_CJMPz;
        interpreter_ptrs[IC::CJMPnz] = &&I_CJMPnz;
        interpreter_ptrs[IC::BEGIN] = &&I_BEGIN;
        interpreter_ptrs[IC::CBEGIN] = &&I_CBEGIN;
        interpreter_ptrs[IC::CLOSURE] = &&I_CLOSURE;
        interpreter_ptrs[IC::CALLC] = &&I_CALLC;
        interpreter_ptrs[IC::CALL] = &&I_CALL;
        interpreter_ptrs[IC::TAG] = &&I_TAG;
        interpreter_ptrs[IC::ARRAY] = &&I_ARRAY;
        interpreter_ptrs[IC::FAIL] = &&I_FAIL;
        interpreter_ptrs[IC::PATT_str] = &&I_PATT_str;
        interpreter_ptrs[IC::PATT_string] = &&I_PATT_string;
        interpreter_ptrs[IC::PATT_array] = &&I_PATT_array;
        interpreter_ptrs[IC::PATT_sexp] = &&I_PATT_sexp;
        interpreter_ptrs[IC::PATT_ref] = &&I_PATT_ref;
        interpreter_ptrs[IC::PATT_val] = &&I_PATT_val;
        interpreter_ptrs[IC::PATT_fun] = &&I_PATT_fun;
        interpreter_ptrs[IC::CALL_Lread] = &&I_CALL_Lread;
        interpreter_ptrs[IC::CALL_Lwrite] = &&I_CALL_Lwrite;
        interpreter_ptrs[IC::CALL_Llength] = &&I_CALL_Llength;
        interpreter_ptrs[IC::CALL_Lstring] = &&I_CALL_Lstring;
        interpreter_ptrs[IC::CALL_Barray] = &&I_CALL_Barray;

        converted[0].interpreter = &&finish;
        converted[1].interpreter = &&bad_jump;

        // array must be initialized
        std::unique_ptr<
            instruction_meta[],
            std::default_delete<instruction_meta[]>
        > instructions_meta { new instruction_meta[bytecode->code_size] {} };

        std::size_t current_function_idx = UINT32_MAX;
        std::size_t current_function_args = 0;
        std::size_t current_function_locals = 0;

        std::size_t bytecode_idx = 0;
        std::size_t converted_idx = 2;
        while (bytecode_idx < bytecode->code_size) {
            const auto code = static_cast<IC::IC>(bytecode->code_ptr[bytecode_idx++]);

            log_file << "0x" << std::hex << bytecode_idx - 1
                     << " (0x" << converted_idx
                     << "). 0x" << code << std::dec
                     << std::endl;

            instruction_meta & ins_meta = instructions_meta[bytecode_idx - 1];
            ins_meta.converted = converted_base + converted_idx;

            for (CC * forward : ins_meta.forward_ptrs) {
                forward->code = ins_meta.converted;
            }

            if ((0xF0 & code) == 0xF0) {
                break;
            }

            if (code == IC::LINE) {
                bytecode_idx += 4;
                continue;
            }

            if (code == IC::BEGIN || code == IC::CBEGIN) {
                current_function_idx = converted_idx;
                current_function_args = read_from_bytes<uint32_t>(bytecode->code_ptr + bytecode_idx);
                current_function_locals = read_from_bytes<uint32_t>(bytecode->code_ptr + bytecode_idx + 4);
            } else if (code == IC::END) {
                current_function_idx = UINT32_MAX;
                current_function_args = 0;
                current_function_locals = 0;
            }

            const void * interpreter_ptr = interpreter_ptrs[code];
            if (!interpreter_ptr) {
                interpreter_ptr = &&I_unsupported;
            }

            converted[converted_idx++].interpreter = interpreter_ptr;

            // TODO check same stack depth on labels
            // TODO check negative stack depth
            // TODO check is BEGIN after CALL
            // TODO check is CBEGIN in CLOSURE
            // TODO check labels accessed from same function
            // TODO check stack depth at END

            switch (code) {

            // plain int arg
            case IC::CONST:
            case IC::CALLC:
            case IC::ARRAY:
            case IC::CALL_Barray:
                converted[converted_idx++].fixnum =
                    to_fixnum(read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx));
                break;

            // string arg
            case IC::STRING: {
                const std::size_t idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx);
                if (idx >= bytecode->string_size) {
                    throw std::invalid_argument { "pointer to string out of range" };
                }

                converted[converted_idx++].string = bytecode->string_ptr + idx;
                break;
            }

            // offset in code arg
            case IC::JMP:
            case IC::CJMPz:
            case IC::CJMPnz:
            case IC::CALL: {
                const std::size_t idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx);
                if (idx >= bytecode->code_size) {
                    throw std::invalid_argument { "pointer to bytecode out of range" };
                }

                auto & idx_ins_meta = instructions_meta[idx];

                if (idx_ins_meta.converted) {
                    converted[converted_idx++].code = idx_ins_meta.converted;
                } else {
                    idx_ins_meta.forward_ptrs.push_back(&converted[converted_idx]);
                    converted[converted_idx++].code = converted_base + 1;
                }

                if (code == IC::CALL) { // ignore second arg
                    bytecode_idx += 4;
                }

                break;
            }

            // G(int) arg
            case IC::LD_G:
            case IC::LDA_G:
            case IC::ST_G: {
                const std::size_t idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx);
                if (idx >= bytecode->code_size) {
                    throw std::invalid_argument { "global index out of range" };
                }

                converted[converted_idx++].global = global_area_base + idx;
                break;
            }

            // A(int) arg
            case IC::LD_A:
            case IC::LDA_A:
            case IC::ST_A: {
                if (current_function_idx == UINT32_MAX) {
                    throw std::invalid_argument { "cannot use arguments outside of function" };
                }

                const std::size_t idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx);
                if (idx >= current_function_args) {
                    throw std::invalid_argument {
                        "argument index is greater than number of arguments"
                    };
                }

                converted[converted_idx++].index = idx;
                break;
            }

            // L(int) arg
            case IC::LD_L:
            case IC::LDA_L:
            case IC::ST_L: {
                if (current_function_idx == UINT32_MAX) {
                    throw std::invalid_argument { "cannot use locals outside of function" };
                }

                const std::size_t idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx);
                if (idx >= current_function_locals) {
                    throw std::invalid_argument {
                        "local index is greater than number of locals"
                    };
                }

                converted[converted_idx++].index = idx + current_function_args;
                break;
            }

            // TODO C(int) arg
            case IC::LD_C:
            case IC::LDA_C:
            case IC::ST_C:
                bytecode_idx += 4;
                break;

            // 2 args
            case IC::SEXP:
            case IC::BEGIN:
            case IC::CBEGIN:
            case IC::TAG:
            case IC::FAIL:
                converted[converted_idx++].args = bytecode->code_ptr + bytecode_idx;
                bytecode_idx += 8;
                break;

            // CLOSURE
            case IC::CLOSURE: {
                converted[converted_idx++].args = bytecode->code_ptr + bytecode_idx;

                const std::size_t n = read_from_bytes<uint32_t>(
                    bytecode->code_ptr + bytecode_idx + 4
                );

                bytecode_idx += 8 + n * 5;
                break;
            }

            // no args
            default:
                break;
            }
        }

        if ((bytecode->code_ptr[bytecode_idx - 1] & 0xF0) != 0xF0) {
            throw std::invalid_argument { "no <end> at end of bytecode" };
            return;
        }
    }

#define NEXT do { goto *(*ip++).interpreter; } while (false)

    activation * current_activation = nullptr;
    const CC * ip = converted_base + 2;
    const CC * rip = converted_base; // TODO set in CALL and CALLC

    std::vector<value> stack;
    stack.emplace_back(); // argc
    stack.emplace_back(); // argv
    stack.emplace_back(); // envp (failsafe)

    struct heap heap { global_area_base, bytecode->global_size, stack, current_activation };

    NEXT;

finish:
    return;

I_BINOP_add: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a + b));
    NEXT;
}

I_BINOP_sub: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a - b));
    NEXT;
}

I_BINOP_mul: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a * b));
    NEXT;
}

I_BINOP_div: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a / b));
    NEXT;
}

I_BINOP_rem: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a % b));
    NEXT;
}

I_BINOP_lt: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a < b));
    NEXT;
}

I_BINOP_le: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a <= b));
    NEXT;
}

I_BINOP_gt: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a > b));
    NEXT;
}

I_BINOP_ge: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a >= b));
    NEXT;
}

I_BINOP_eq: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a == b));
    NEXT;
}

I_BINOP_ne: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a != b));
    NEXT;
}

I_BINOP_and: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a && b));
    NEXT;
}

I_BINOP_or: {
    const int32_t b = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const int32_t a = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    stack.emplace_back(to_fixnum(a || b));
    NEXT;
}

I_CONST: {
    stack.emplace_back((ip++)->fixnum);
    NEXT;
}

I_STRING: {
    const char * const str = (ip++)->string;
    const std::size_t str_size = std::strlen(str);

    auto * const result = heap.alloc(str_size);
    result->kind = heap_object::STRING;
    result->fields_size = str_size;

    std::memcpy(reinterpret_cast<uint8_t *>(result) + sizeof(heap_object), str, str_size);

    stack.emplace_back(result);
    NEXT;
}

I_SEXP:
    goto I_unsupported;

I_STI:
    goto I_unsupported;

I_STA: {
    const value x = stack.back();
    stack.pop_back();

    const int32_t index = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const value xs = stack.back();
    stack.pop_back();

    heap.assert_heap_value(xs);

    heap_object * const obj = xs.object;

    if (index < 0 || static_cast<std::size_t>(index) >= obj->fields_size) {
        throw std::invalid_argument { "index is out of range" };
    }

    switch (obj->get_kind()) {
    case heap_object::STRING:
        if (!is_fixnum(x.fixnum)) {
            throw std::invalid_argument { "cannon assign non-integral value in string" };
        }

        reinterpret_cast<uint8_t *>(obj)[sizeof(heap_object) + index] = from_fixnum(x.fixnum);
        break;

    case heap_object::ARRAY:
    case heap_object::SEXP:
        obj->get_field(index) = x;
        break;
    }

    stack.push_back(x);
    NEXT;
}

I_JMP: {
    auto * const imm = (ip++)->code;
    ip = imm;
    NEXT;
}

I_END: {
    ip = current_activation->return_ptr;

    auto * const tmp = current_activation;
    current_activation = tmp->parent;
    activation::drop(tmp);

    NEXT;
}

I_RET:
    goto I_unsupported;

I_DROP:
    stack.pop_back();
    NEXT;

I_DUP:
    goto I_unsupported;

I_SWAP:
    goto I_unsupported;

I_ELEM: {
    const int32_t index = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const value x = stack.back();
    stack.pop_back();

    heap.assert_heap_value(x);

    heap_object * const obj = x.object;

    if (index < 0 || static_cast<std::size_t>(index) >= obj->fields_size) {
        throw std::invalid_argument { "index is out of range" };
    }

    value result;
    switch (obj->get_kind()) {
    case heap_object::STRING:
        result = value {
            to_fixnum(reinterpret_cast<uint8_t *>(obj)[sizeof(heap_object) + index]),
        };
        break;

    case heap_object::ARRAY:
    case heap_object::SEXP:
        result = obj->get_field(index);
        break;
    }

    stack.emplace_back(result);
    NEXT;
}

I_LD_G: {
    auto * const imm = (ip++)->global;
    stack.push_back(*imm);
    NEXT;
}

I_LD_AL: {
    const std::size_t imm = (ip++)->index;
    stack.push_back(current_activation->local(imm));
    NEXT;
}

I_LD_C:
    goto I_unsupported;

I_LDA_G:
    goto I_unsupported;

I_LDA_AL:
    goto I_unsupported;

I_LDA_C:
    goto I_unsupported;

I_ST_G: {
    auto * const imm = (ip++)->global;
    *imm = stack.back();
    NEXT;
}

I_ST_AL: {
    const std::size_t imm = (ip++)->index;
    current_activation->local(imm) = stack.back();
    NEXT;
}

I_ST_C:
    goto I_unsupported;

I_CJMPz: {
    auto * const imm = (ip++)->code;
    const int32_t x = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    if (x == 0) {
        ip = imm;
    }

    NEXT;
}

I_CJMPnz: {
    auto * const imm = (ip++)->code;
    const int32_t x = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    if (x != 0) {
        ip = imm;
    }

    NEXT;
}

I_BEGIN: {
    auto imm = read_from_bytes<uint32_t[2]>((ip++)->args);
    auto * const act = activation::create(current_activation, rip, imm[0] + imm[1]);
    rip = nullptr;

    for (std::size_t i = imm[0]; i > 0; --i) {
        act->local(i - 1) = stack.back();
        stack.pop_back();
    }

    current_activation = act;
    NEXT;
}

I_CBEGIN:
    goto I_unsupported;

I_CLOSURE:
    goto I_unsupported;

I_CALLC:
    goto I_unsupported;

I_CALL: {
    auto * const imm = (ip++)->code;
    rip = ip;
    ip = imm;
    NEXT;
}

I_TAG:
    goto I_unsupported;

I_ARRAY:
    goto I_unsupported;

I_FAIL:
    goto I_unsupported;

I_PATT_str:
    goto I_unsupported;

I_PATT_string:
    goto I_unsupported;

I_PATT_array:
    goto I_unsupported;

I_PATT_sexp:
    goto I_unsupported;

I_PATT_ref:
    goto I_unsupported;

I_PATT_val:
    goto I_unsupported;

I_PATT_fun:
    goto I_unsupported;

I_CALL_Lread: {
    std::cout << "> ";
    std::cout.flush();

    int32_t value;
    std::cin >> value;

    stack.emplace_back(to_fixnum(value));
    NEXT;
}

I_CALL_Lwrite: {
    const int32_t value = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    std::cout << value << std::endl;

    stack.emplace_back();
    NEXT;
}

I_CALL_Llength: {
    const value x = stack.back();
    stack.pop_back();

    heap.assert_heap_value(x);
    stack.emplace_back(to_fixnum(x.object->fields_size));
    NEXT;
}

I_CALL_Lstring:
    goto I_unsupported;

I_CALL_Barray:
    goto I_unsupported;

I_unsupported: // unsupported instruction
    std::ostringstream os;

    os << "Unsupported instruction at 0x" << std::hex << ip - converted_base - 1;
    throw std::invalid_argument { os.str() };

bad_jump: // unresolved jump
    throw std::invalid_argument { "unresolved jump happen" };

#undef NEXT
}

int main(int argc, char * argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " <bytecode file>" << std::endl;
        return 0;
    }

    auto bytecode = read_bytecode(argv[1]);

    log_file << "String pool size: " << bytecode->string_size << std::endl;
    log_file << "Global area size: " << bytecode->global_size << std::endl;
    log_file << "Code size: " << bytecode->code_size << std::endl;

    log_file << "Public names:" << std::endl;
    for (std::size_t i = 0; i < bytecode->public_size; ++i) {
        auto public_entry = bytecode->public_ptr[i];

        log_file << bytecode->string_ptr + public_entry.name_pos
                 << " -> 0x" << std::hex << public_entry.code_pos << std::dec
                 << std::endl;
    }

    interpret(bytecode);
}
