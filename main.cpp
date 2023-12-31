#include <unordered_map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <cstring>
#include <cassert>
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


#ifdef DEBUG
static std::ofstream log_file { "./log.txt" };
#endif


union value;

struct heap_object {

    enum object_kind {

        STRING  = 0,
        ARRAY   = 1,
        SEXP    = 2,
        CLOSURE = 3,
    };

    uint8_t kind;
    std::size_t fields_size;
    heap_object * actual_ptr;
    heap_object * gc_next;

    heap_object() = delete;

    object_kind get_kind() const {
        switch (kind) {
        case STRING:
        case ARRAY:
        case SEXP:
        case CLOSURE:
            return static_cast<object_kind>(kind);

        default:
            throw std::invalid_argument { "unknown object kind" };
        }
    }

    value & field(std::size_t idx) noexcept;
};

union CC;

union value {

    uint32_t fixnum;
    heap_object * object;
    const char * tag;
    const CC * code;
    value * var;

    value() noexcept = default;
    explicit value(uint32_t fixnum) noexcept: fixnum(fixnum) {}
    explicit value(heap_object * object) noexcept: object(object) {}
    explicit value(value * var) noexcept: var(var) {}
};

// converted code
union CC {

    const void * interpreter;

    int32_t num;
    uint32_t fixnum;
    const char * string;
    const CC * code;
    value * global;
    uint32_t index;
    uint32_t locs;
};

static_assert(sizeof(value) == sizeof(void *));
static_assert(sizeof(CC) == sizeof(void *));

struct instruction_meta {

    CC * converted = nullptr;
    std::size_t function_idx = UINT32_MAX;
    std::size_t stack_depth = UINT32_MAX;
    std::list<std::size_t> forward_idxs;
};

struct activation {

    activation * parent;
    const CC * return_ptr;
    std::size_t locals_size;
    value closure;

    value & local(std::size_t idx) noexcept {
        return *(reinterpret_cast<value *>(reinterpret_cast<uint8_t *>(this) + sizeof(activation))
            + idx);
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

template<typename T>
static inline constexpr T p2(T x) noexcept;

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

    static inline constexpr const std::size_t INITIAL_SIZE = 4 * 1024;
    static inline constexpr const std::size_t ALIGNMENT = 16;

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
        , buffer(new uint8_t[INITIAL_SIZE])
        , first_half(buffer.get())
        , second_half(first_half + INITIAL_SIZE / 2)
        , size(INITIAL_SIZE)
    {
        if (!buffer) {
            throw std::bad_alloc {};
        }
    }

    static constexpr std::size_t align_size(std::size_t size) noexcept {
        return (size + (ALIGNMENT - 1)) / ALIGNMENT * ALIGNMENT;
    }

    static constexpr std::size_t get_object_size(
        heap_object::object_kind kind,
        std::size_t fields
    ) {
        switch (kind) {
        case heap_object::STRING:
            return fields;

        case heap_object::ARRAY:
            return fields * sizeof(value);

        case heap_object::SEXP:
        case heap_object::CLOSURE:
            return (fields + 1) * sizeof(value);
        }

        throw std::invalid_argument { "unsupported kind" };
    }

    heap_object * alloc(heap_object::object_kind kind, std::size_t fields) {
        std::size_t object_size = align_size(sizeof(heap_object))
            + align_size(get_object_size(kind, fields));

        request(object_size);

        auto * const result = reinterpret_cast<heap_object *>(first_half + offset);
        offset += object_size;

        std::memset(result, 0, object_size);

        result->kind = kind;
        result->fields_size = fields;
        result->actual_ptr = result;
        result->gc_next = nullptr;
        return result;
    }

    bool is_heap_value(value val) const noexcept {
        if (is_fixnum(val.fixnum)) {
            return false;
        }

        const auto * const raw_ptr = reinterpret_cast<const uint8_t *>(val.object);
        if (raw_ptr < first_half || raw_ptr >= first_half + offset) {
            return false;
        }

        return true;
    }

    void assert_heap_value(value val) const {
        if (!is_heap_value(val)) {
            throw std::out_of_range { "argument is not heap value" };
        }
    }

    void request(std::size_t object_size) {
        const std::size_t min_size = (offset + object_size) * 2;

        if (min_size <= size) {
            return;
        }

        gc();
        grow(min_size);
    }

    void gc() {
        heap_object * queue_head = nullptr;
        heap_object ** queue_tail = &queue_head;
        std::size_t new_offset = 0;

#ifdef DEBUG
        log_file << "Run GC" << std::endl;
#endif

        for (std::size_t i = 0; i < global_area_size; ++i) {
            mark_value(global_area[i], new_offset, queue_tail);
        }

        for (value & v : stack) {
            mark_value(v, new_offset, queue_tail);
        }

        {
            activation * act = current_activation;

            while (act) {
                mark_value(act->closure, new_offset, queue_tail);

                for (std::size_t i = 0; i < act->locals_size; ++i) {
                    mark_value(act->local(i), new_offset, queue_tail);
                }

                act = act->parent;
            }
        }

        while (queue_head) {
            heap_object * new_queue_head = nullptr;
            heap_object ** new_queue_tail = &new_queue_head;

            for (; queue_head; queue_head = queue_head->gc_next) {
                compact_object(queue_head, new_offset, new_queue_tail);
            }

            queue_head = new_queue_head;
            // queue_tail = new_queue_tail;
        }

        std::swap(first_half, second_half);
        offset = new_offset;
    }

    void mark_value(value & val, size_t & new_offset, heap_object ** & queue_tail) {
        if (!is_heap_value(val)) {
            return;
        }

        val.object = mark_object(val.object, new_offset, queue_tail);
    }

    heap_object * mark_object(
        heap_object * heap_ptr,
        size_t & new_offset,
        heap_object ** & queue_tail
    ) {
        if (heap_ptr->actual_ptr != heap_ptr) {
            return heap_ptr->actual_ptr;
        }

#ifdef DEBUG
        assert(!heap_ptr->gc_next);
#endif

        const heap_object::object_kind k = heap_ptr->get_kind();
        const std::size_t object_size = align_size(sizeof(heap_object))
            + align_size(get_object_size(k, heap_ptr->fields_size));

        heap_object * const new_ptr = reinterpret_cast<heap_object *>(second_half + new_offset);
        new_offset += object_size;

        heap_ptr->actual_ptr = new_ptr;

        *queue_tail = heap_ptr;
        queue_tail = &heap_ptr->gc_next;
        return new_ptr;
    }

    void compact_object(heap_object * heap_ptr, size_t & new_offset, heap_object ** & queue_tail) {
        heap_object * const new_ptr = heap_ptr->actual_ptr;

#ifdef DEBUG
        assert(new_ptr != heap_ptr);
#endif

        const heap_object::object_kind k = heap_ptr->get_kind();
        const std::size_t object_size = align_size(sizeof(heap_object))
            + align_size(get_object_size(k, heap_ptr->fields_size));

        std::memcpy(new_ptr, heap_ptr, object_size);
        new_ptr->gc_next = nullptr;

#ifdef DEBUG
        log_file << "Moved object of size " << object_size
                 << ' ' << heap_ptr
                 << " -> " << new_ptr
                 << std::endl;
#endif

        if (k == heap_object::STRING) {
            return;
        }

        for (std::size_t i = 0; i < new_ptr->fields_size; ++i) {
            mark_value(new_ptr->field(i), new_offset, queue_tail);
        }
    }

    void grow(std::size_t new_size) {
        new_size = p2(new_size);

        uint8_t * new_buffer = new uint8_t[new_size];

        if (!new_buffer) {
            throw std::bad_alloc {};
        }

        second_half = new_buffer;
        gc();

        second_half = new_buffer + new_size / 2;
        size = new_size;

        buffer.reset(new_buffer);

#ifdef DEBUG
        log_file << "Heap grown to " << size
                 << " (" << reinterpret_cast<void *>(first_half)
                 << ", " << reinterpret_cast<void *>(second_half)
                 << ')' << std::endl;
#endif
    }
};


template<typename T>
static inline constexpr T p2(T x) noexcept {
    T res = 1;

    while (res < x) {
        res <<= 1;
    }

    return res;
}

value & heap_object::field(std::size_t idx) noexcept {
    return *(reinterpret_cast<value *>(reinterpret_cast<uint8_t *>(this)
        + heap::align_size(sizeof(heap_object))) + idx);
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

static void value_to_string(const struct heap & heap, value v, std::ostringstream & result) {
    if (is_fixnum(v.fixnum)) {
        result << from_fixnum(v.fixnum);
        return;
    }

    if (!heap.is_heap_value(v)) {
        result << *reinterpret_cast<void **>(&v);
    }

    heap_object * const obj = v.object;
    const std::size_t fields = obj->fields_size;

    switch (obj->get_kind()) {
    case heap_object::STRING:
        result << '"' << reinterpret_cast<const char *>(&obj->field(0)) << '"';
        break;

    case heap_object::ARRAY:
        if (fields == 0) {
            result << "[]";
        } else {
            result << '[';
            value_to_string(heap, obj->field(0), result);

            for (std::size_t i = 1; i < fields; ++i) {
                result << ", ";

                value_to_string(heap, obj->field(i), result);
            }

            result << ']';
        }

        break;

    case heap_object::SEXP:
        result << obj->field(fields).tag;

        if (fields > 0) {
            result << " (";
            value_to_string(heap, obj->field(0), result);

            for (std::size_t i = 1; i < fields; ++i) {
                result << ", ";

                value_to_string(heap, obj->field(i), result);
            }

            result << ')';
        }

        break;

    case heap_object::CLOSURE:
        result << "<closure " << obj->field(fields).code;

        for (std::size_t i = 0; i < fields; ++i) {
            result << ", ";

            value_to_string(heap, obj->field(i), result);
        }

        result << '>';
        break;
    }
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
        new CC[bytecode->code_size + 3],
    };

    std::unique_ptr<value[], std::default_delete<value[]>> global_area {
        new value[bytecode->global_size],
    };

    CC * const converted_base = converted.get();
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
        interpreter_ptrs[IC::RET] = &&I_END;
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

        std::unordered_map<std::string_view, const char *> tags;

        std::size_t current_function_idx = UINT32_MAX;
        std::size_t current_function_args = 0;
        std::size_t current_function_locals = 0;
        bool current_function_closure = false;

        std::size_t bytecode_idx = 0;
        std::size_t converted_idx = 2;
        while (bytecode_idx < bytecode->code_size) {
            const auto code = static_cast<IC::IC>(bytecode->code_ptr[bytecode_idx++]);

#ifdef DEBUG
            log_file << "0x" << std::hex << bytecode_idx - 1
                     << " (0x" << converted_idx
                     << "). 0x" << code << std::dec
                     << std::endl;
#endif

            instruction_meta & ins_meta = instructions_meta[bytecode_idx - 1];
            ins_meta.converted = converted_base + converted_idx;
            ins_meta.function_idx = current_function_idx;

            for (std::size_t forward_idx : ins_meta.forward_idxs) {
                const instruction_meta & forward_meta = instructions_meta[forward_idx];

                const std::size_t forward_function_idx = forward_meta.function_idx;
                if (
                    forward_function_idx != UINT32_MAX
                    && forward_function_idx != current_function_idx
                ) {
                    throw std::invalid_argument { "attempt to jump to another function" };
                }

                forward_meta.converted->code = ins_meta.converted;
            }

            if ((0xF0 & code) == 0xF0) {
                break;
            }

            if (code == IC::LINE) {
                bytecode_idx += 4;
                continue;
            }

            if (code == IC::BEGIN || code == IC::CBEGIN) {
                if (current_function_idx != UINT32_MAX) {
                    throw std::invalid_argument { "illegal nested function" };
                }

                current_function_idx = converted_idx;
                current_function_args = read_from_bytes<uint32_t>(bytecode->code_ptr + bytecode_idx);
                current_function_locals = read_from_bytes<uint32_t>(bytecode->code_ptr + bytecode_idx + 4);
                current_function_closure = code == IC::CBEGIN;
                ins_meta.function_idx = current_function_idx;
            } else if (code == IC::END) {
                current_function_idx = UINT32_MAX;
                current_function_args = 0;
                current_function_locals = 0;
                current_function_closure = false;
            }

            const void * interpreter_ptr = interpreter_ptrs[code];
            if (!interpreter_ptr) {
                interpreter_ptr = &&I_unsupported;
            }

            converted[converted_idx++].interpreter = interpreter_ptr;

            // TODO check is BEGIN after CALL
            // TODO check is BEGIN or CBEGIN in CLOSURE
            // TODO check CBEGIN is only in CLOSURE
            // TODO check index of C(_) loc

            // TODO check same stack depth on labels
            // TODO check negative stack depth
            // TODO check stack depth at END is 1
            // TODO check stack size > 0 at finish (first and last)

#define NAT_ARG(__var) do { \
    const int32_t _imm = read_from_bytes<int32_t>(bytecode->code_ptr, bytecode_idx); \
    if (_imm < 0) { \
        throw std::invalid_argument { "negative argument" }; \
    } \
\
    __var = _imm; \
} while (false)

#define STRING_ARG(__var) do { \
    const std::size_t _idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx); \
    if (_idx >= bytecode->string_size) { \
        throw std::invalid_argument { "pointer to string out of range" }; \
    } \
\
    __var = bytecode->string_ptr + _idx; \
} while (false)

#define CODE_PTR_ARG(__jump) do { \
    const std::size_t _old_bytecode_idx = bytecode_idx; \
\
    const std::size_t _idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx); \
    if (_idx >= bytecode->code_size) { \
        throw std::invalid_argument { "pointer to bytecode out of range" }; \
    } \
\
    auto & _idx_ins_meta = instructions_meta[_idx]; \
\
    if (_idx_ins_meta.converted) { \
        if ((__jump) && _idx_ins_meta.function_idx != current_function_idx) { \
            throw std::invalid_argument { "attempt to jump to another function" }; \
        } \
\
        converted[converted_idx++].code = _idx_ins_meta.converted; \
    } else { \
        instructions_meta[_old_bytecode_idx].converted = converted_base + converted_idx; \
        instructions_meta[_old_bytecode_idx].function_idx = (__jump) \
            ? current_function_idx : UINT32_MAX; \
\
        _idx_ins_meta.forward_idxs.emplace_back(_old_bytecode_idx); \
        converted[converted_idx++].code = converted_base + 1; \
    } \
} while (false)

#define LOC_G_ARG(__var) do { \
    const std::size_t _idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx); \
    if (_idx >= bytecode->code_size) { \
        throw std::invalid_argument { "global index out of range" }; \
    } \
\
    __var = global_area_base + _idx; \
} while (false)

#define LOC_A_ARG(__var) do { \
    if (current_function_idx == UINT32_MAX) { \
        throw std::invalid_argument { "cannot use arguments outside of function" }; \
    } \
\
    const std::size_t _idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx); \
    if (_idx >= current_function_args) { \
        throw std::invalid_argument { "argument index is greater than number of arguments" }; \
    } \
\
    __var = _idx; \
} while (false)

#define LOC_L_ARG(__var) do { \
    if (current_function_idx == UINT32_MAX) { \
        throw std::invalid_argument { "cannot use locals outside of function" }; \
    } \
\
    const std::size_t _idx = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx); \
    if (_idx >= current_function_locals) { \
        throw std::invalid_argument { "local index is greater than number of locals" }; \
    } \
\
    __var = _idx + current_function_args; \
} while (false)

#define LOC_C_ARG(__var) do { \
    if (!current_function_closure) { \
        throw std::invalid_argument { "cannot use closured values outside of closure" }; \
    } \
\
    __var = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx); \
} while (false)

            switch (code) {

            // fixnum arg
            case IC::CONST:
                converted[converted_idx++].fixnum =
                    to_fixnum(read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx));
                break;

            // nat arg
            case IC::CALLC:
            case IC::ARRAY:
            case IC::CALL_Barray:
                NAT_ARG(converted[converted_idx++].num);
                break;

            // string arg
            case IC::STRING:
                STRING_ARG(converted[converted_idx++].string);
                break;

            // offset in code arg
            case IC::JMP:
            case IC::CJMPz:
            case IC::CJMPnz:
                CODE_PTR_ARG(true);
                break;

            // G(int) arg
            case IC::LD_G:
            case IC::LDA_G:
            case IC::ST_G:
                LOC_G_ARG(converted[converted_idx++].global);
                break;

            // A(int) arg
            case IC::LD_A:
            case IC::LDA_A:
            case IC::ST_A:
                LOC_A_ARG(converted[converted_idx++].index);
                break;

            // L(int) arg
            case IC::LD_L:
            case IC::LDA_L:
            case IC::ST_L:
                LOC_L_ARG(converted[converted_idx++].index);
                break;

            // C(int) arg
            case IC::LD_C:
            case IC::ST_C:
                LOC_C_ARG(converted[converted_idx++].index);
                break;

            // nat, nat args
            case IC::BEGIN:
            case IC::CBEGIN:
            case IC::FAIL:
                NAT_ARG(converted[converted_idx++].num);
                NAT_ARG(converted[converted_idx++].num);
                break;

            // tag, nat args
            case IC::SEXP:
            case IC::TAG: {
                const char * tag;
                STRING_ARG(tag);

                if (auto it = tags.find(tag); it != tags.end()) {
                    tag = it->second;
                } else {
                    tags[tag] = tag;
                }

                converted[converted_idx++].string = tag;
                NAT_ARG(converted[converted_idx++].num);
                break;
            }

            case IC::LDA_C:
                // technically it is simple operation, but may cause pointers to middle of object
                // on stack, that could break the GC invariant (all pointers in heap points to
                // start of objects)
                //
                // Dmitry Boulytchev says that assignment to closured values is forbidden, but
                // is compiled by actual LaMa compiler because of historical reasons, so we assume
                // that programs that uses assignments to closured values is ill-formed, and
                // interpreter may not support that
                throw std::invalid_argument { "LDA C(_) is not supported" };

            case IC::CLOSURE: {
                CODE_PTR_ARG(false);

                const uint32_t n = read_from_bytes<uint32_t>(bytecode->code_ptr, bytecode_idx);
                if (bytecode_idx + n * 5 > bytecode->code_size) {
                    throw std::invalid_argument { "CLOSURE is out of bytecode size" };
                }

                converted[converted_idx++].index = n;

                uint8_t locs[16] {};
                std::uint32_t * locs_ptr = nullptr;
                for (std::size_t i = 0; i < n; ++i) {
                    if (i % 16 == 0) {
                        if (i > 0) {
                            std::uint32_t locs_num = 0;

                            for (int j = 15; j >= 0; --j) {
                                locs_num = (locs_num << 2) | locs[j];
                            }

                            *locs_ptr = locs_num;
                        }

                        locs_ptr = &converted[converted_idx++].locs;
                        std::memset(locs, 0, sizeof(locs));
                    }

                    const uint8_t loc = read_from_bytes<uint8_t>(bytecode->code_ptr, bytecode_idx);
                    locs[i % 16] = loc;

                    switch (loc) {
                    case 0:
                        LOC_G_ARG(converted[converted_idx++].global);
                        break;

                    case 1:
                        LOC_L_ARG(converted[converted_idx++].index);
                        break;

                    case 2:
                        LOC_A_ARG(converted[converted_idx++].index);
                        break;

                    case 3:
                        LOC_C_ARG(converted[converted_idx++].index);
                        break;

                    default:
                        throw std::invalid_argument { "unsupported loc in closure" };
                    }
                }

                if (n > 0) {
                    std::uint32_t locs_num = 0;

                    for (int j = (n - 1) % 16; j >= 0; --j) {
                        locs_num = (locs_num << 2) | locs[j];
                    }

                    *locs_ptr = locs_num;
                }

                break;
            }

            case IC::CALL:
                CODE_PTR_ARG(false);

                // ignore second arg
                bytecode_idx += 4;
                break;

            // no args
            default:
                break;
            }

#undef NAT_ARG
#undef STRING_ARG
#undef CODE_PTR_ARG
#undef LOC_G_ARG
#undef LOC_A_ARG
#undef LOC_L_ARG
#undef LOC_C_ARG
        }

        if ((bytecode->code_ptr[bytecode_idx - 1] & 0xF0) != 0xF0) {
            throw std::invalid_argument { "no <end> at end of bytecode" };
            return;
        }

        converted[converted_idx].interpreter = &&finish;
    }

#ifdef DEBUG
#define NEXT do { \
    log_file << "Current IP: 0x" << std::hex << ip - converted_base << std::dec << std::endl; \
    goto *(ip++)->interpreter; \
} while (false)
#else
#define NEXT do { goto *(ip++)->interpreter; } while (false)
#endif

    activation * current_activation = nullptr;
    const CC * ip = converted_base + 2;
    const CC * rip = converted_base;
    bool from_callc = false;

    std::vector<value> stack;
    stack.reserve(128);
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

    auto * const result = heap.alloc(heap_object::STRING, str_size);

    std::memcpy(reinterpret_cast<char *>(&result->field(0)), str, str_size);

    stack.emplace_back(result);
    NEXT;
}

I_SEXP: {
    const char * const imm1 = (ip++)->string;
    const int32_t imm2 = (ip++)->num;

    auto * const result = heap.alloc(heap_object::SEXP, imm2);

    for (int32_t i = imm2 - 1; i >= 0; --i) {
        result->field(i) = stack.back();
        stack.pop_back();
    }

    result->field(imm2).tag = imm1;
    stack.emplace_back(result);
    NEXT;
}

I_STI:
    goto I_unsupported;

I_STA: {
    const value x = stack.back();
    stack.pop_back();

    const value index_value = stack.back();
    stack.pop_back();

    if (is_fixnum(index_value.fixnum)) {
        const int32_t index = from_fixnum(index_value.fixnum);

        const value xs = stack.back();
        stack.pop_back();

        heap.assert_heap_value(xs);

        heap_object * const obj = xs.object;

        if (index < 0 || static_cast<std::size_t>(index) >= obj->fields_size) {
            throw std::out_of_range { "index is out of range" };
        }

        switch (obj->get_kind()) {
        case heap_object::STRING:
            reinterpret_cast<int8_t *>(&obj->field(0))[index] = from_fixnum(x.fixnum);
            break;

        case heap_object::ARRAY:
        case heap_object::SEXP:
            obj->field(index) = x;
            break;

        case heap_object::CLOSURE:
            throw std::out_of_range { "cannot assign value in closure" };
        }
    } else {
        if (heap.is_heap_value(index_value)) {
            throw std::out_of_range { "invalid assignment target" };
        }

        *index_value.var = x;
    }

    stack.emplace_back(x);
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

I_DROP:
    stack.pop_back();
    NEXT;

I_DUP:
    stack.emplace_back(stack.back());
    NEXT;

I_SWAP: {
    const std::size_t i = stack.size() - 2;

    std::swap(stack[i], stack[i + 1]);
    NEXT;
}

I_ELEM: {
    const int32_t index = from_fixnum(stack.back().fixnum);
    stack.pop_back();

    const value x = stack.back();
    stack.pop_back();

    heap.assert_heap_value(x);

    heap_object * const obj = x.object;

    if (index < 0 || static_cast<std::size_t>(index) >= obj->fields_size) {
        throw std::out_of_range { "index is out of range" };
    }

    value result;
    switch (obj->get_kind()) {
    case heap_object::STRING:
        result = value {
            to_fixnum(reinterpret_cast<int8_t *>(&obj->field(0))[index]),
        };
        break;

    case heap_object::ARRAY:
    case heap_object::SEXP:
        result = obj->field(index);
        break;

    case heap_object::CLOSURE:
        throw std::out_of_range { "cannot index closure" };
    }

    stack.emplace_back(result);
    NEXT;
}

I_LD_G: {
    auto * const imm = (ip++)->global;
    stack.emplace_back(*imm);
    NEXT;
}

I_LD_AL: {
    const std::size_t imm = (ip++)->index;
    stack.emplace_back(current_activation->local(imm));
    NEXT;
}

I_LD_C: {
    const std::size_t imm = (ip++)->index;
    stack.emplace_back(current_activation->closure.object->field(imm));
    NEXT;
}

I_LDA_G: {
    auto * const imm = (ip++)->global;
    stack.emplace_back(imm);
    NEXT;
}

I_LDA_AL: {
    const std::size_t imm = (ip++)->index;
    stack.emplace_back(&current_activation->local(imm));
    NEXT;
}

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

I_ST_C: {
    const std::size_t imm = (ip++)->index;
    current_activation->closure.object->field(imm) = stack.back();
    NEXT;
}

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
    const int32_t imm1 = (ip++)->num; // args
    const int32_t imm2 = (ip++)->num; // locals
    auto * const act = activation::create(current_activation, rip, imm1 + imm2);
    rip = nullptr;

    for (std::size_t i = imm1; i > 0; --i) {
        act->local(i - 1) = stack.back();
        stack.pop_back();
    }

    if (from_callc) {
        stack.pop_back();
        from_callc = false;
    }

    current_activation = act;
    NEXT;
}

I_CBEGIN: {
    const int32_t imm1 = (ip++)->num; // args
    const int32_t imm2 = (ip++)->num; // locals
    auto * const act = activation::create(current_activation, rip, imm1 + imm2);
    rip = nullptr;

    for (std::size_t i = imm1; i > 0; --i) {
        act->local(i - 1) = stack.back();
        stack.pop_back();
    }

    act->closure = value { stack.back() };
    stack.pop_back();

    current_activation = act;
    from_callc = false;
    NEXT;
}

I_CLOSURE: {
    auto * const imm1 = (ip++)->code;
    const std::size_t imm2 = (ip++)->index;

    auto * const result = heap.alloc(heap_object::CLOSURE, imm2);
    result->field(imm2).code = imm1;

    uint32_t locs = 0;
    for (std::size_t i = 0; i < imm2; ++i) {
        if (i % 16 == 0) {
            locs = (ip++)->locs;
        }

        const uint8_t loc = locs & 3;
        locs >>= 2;

        switch (loc) {
        case 0: // G(x)
            result->field(i) = *(ip++)->global;
            break;

        case 1: // L(x)
        case 2: // A(x)
            result->field(i) = current_activation->local((ip++)->index);
            break;

        case 3: // C(x)
            result->field(i) = current_activation->closure.object->field((ip++)->index);
            break;
        }
    }

    stack.emplace_back(result);
    NEXT;
}

I_CALLC: {
    const int32_t imm = (ip++)->num;

    const value x = stack[stack.size() - 1 - imm];

    heap.assert_heap_value(x);

    heap_object * const obj = x.object;

    if (obj->get_kind() != heap_object::CLOSURE) {
        throw std::out_of_range { "cannot call value as closure" };
    }

    rip = ip;
    ip = obj->field(obj->fields_size).code;
    from_callc = true;
    NEXT;
}

I_CALL: {
    auto * const imm = (ip++)->code;
    rip = ip;
    ip = imm;
    NEXT;
}

I_TAG: {
    const char * const imm1 = (ip++)->string;
    const int32_t imm2 = (ip++)->num;

    const value x = stack.back();
    stack.pop_back();

    bool result = false;
    if (heap.is_heap_value(x)) {
        heap_object * const obj = x.object;

        if (obj->get_kind() == heap_object::SEXP) {
            result = obj->fields_size == static_cast<uint32_t>(imm2)
                && obj->field(imm2).tag == imm1;
        }
    }

    stack.emplace_back(to_fixnum(result));
    NEXT;
}

I_ARRAY: {
    const int32_t imm = (ip++)->num;

    const value x = stack.back();
    stack.pop_back();

    bool result = false;
    if (heap.is_heap_value(x)) {
        heap_object * const obj = x.object;

        if (obj->get_kind() == heap_object::ARRAY) {
            result = obj->fields_size == static_cast<uint32_t>(imm);
        }
    }

    stack.emplace_back(to_fixnum(result));
    NEXT;
}

I_FAIL: {
    const int32_t imm1 = (ip++)->num; // line
    const int32_t imm2 = (ip++)->num; // column

    const value x = stack.back();
    stack.pop_back();

    std::ostringstream os;
    os << "match failure at " << imm1 << ':' << imm2 << ", value: ";
    value_to_string(heap, x, os);

    throw std::out_of_range { os.str() };
    goto I_unsupported;
}

I_PATT_str: {
    const value x = stack.back();
    stack.pop_back();

    const value y = stack.back();
    stack.pop_back();

    bool result = false;
    if (heap.is_heap_value(y)) {
        heap_object * const y_obj = y.object;

        if (y_obj->get_kind() == heap_object::STRING) {
            heap.assert_heap_value(x);

            heap_object * const x_obj = x.object;
            if (x_obj->get_kind() != heap_object::STRING) {
                throw std::out_of_range { "argument of PATT =str must be string" };
            }

            const std::size_t str_size = x_obj->fields_size;
            result = y_obj->fields_size == str_size
                && std::memcmp(&y_obj->field(0), &x_obj->field(0), str_size) == 0;
        }
    }

    stack.emplace_back(to_fixnum(result));
    NEXT;
}

I_PATT_string: {
    const value x = stack.back();
    stack.pop_back();

    const bool result = heap.is_heap_value(x) && x.object->kind == heap_object::STRING;
    stack.emplace_back(to_fixnum(result));
    NEXT;
}

I_PATT_array: {
    const value x = stack.back();
    stack.pop_back();

    const bool result = heap.is_heap_value(x) && x.object->kind == heap_object::ARRAY;
    stack.emplace_back(to_fixnum(result));
    NEXT;
}

I_PATT_sexp: {
    const value x = stack.back();
    stack.pop_back();

    const bool result = heap.is_heap_value(x) && x.object->kind == heap_object::SEXP;
    stack.emplace_back(to_fixnum(result));
    NEXT;
}

I_PATT_ref: {
    const value x = stack.back();
    stack.pop_back();

    stack.emplace_back(to_fixnum(heap.is_heap_value(x)));
    NEXT;
}

I_PATT_val: {
    const value x = stack.back();
    stack.pop_back();

    stack.emplace_back(to_fixnum(is_fixnum(x.fixnum)));
    NEXT;
}

I_PATT_fun: {
    const value x = stack.back();
    stack.pop_back();

    const bool result = heap.is_heap_value(x) && x.object->kind == heap_object::CLOSURE;
    stack.emplace_back(to_fixnum(result));
    NEXT;
}

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

I_CALL_Lstring: {
    const value x = stack.back();

    {
        std::ostringstream os;
        value_to_string(heap, x, os);

        const std::string str = std::move(os).str();

#ifdef DEBUG
        log_file << "Lstring: " << str << std::endl;
#endif

        auto * const result = heap.alloc(heap_object::STRING, str.size());

        std::memcpy(reinterpret_cast<char *>(&result->field(0)), str.data(), str.size());

        stack.emplace_back(result);
    }

    NEXT;
}

I_CALL_Barray: {
    const int32_t imm = (ip++)->num;

    auto * const result = heap.alloc(heap_object::ARRAY, imm);

    for (int32_t i = imm - 1; i >= 0; --i) {
        result->field(i) = stack.back();
        stack.pop_back();
    }

    stack.emplace_back(result);
    NEXT;
}

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

#ifdef DEBUG
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
#endif

    try {
        interpret(bytecode);
    } catch (const std::invalid_argument & e) {
        std::cerr << "Ill-formed bytecode: " << e.what() << std::endl;
    } catch (const std::out_of_range & e) {
        std::cerr << "Runtime failure: " << e.what() << std::endl;
    }
}
