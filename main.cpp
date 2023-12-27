#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdint>
#include <memory>
#include <stack>


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

union value {

    uint32_t fixnum;
    value ** var;

    value() noexcept = default;
    explicit value(uint32_t fixnum) noexcept: fixnum(fixnum) {}
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

struct activation {

    const CC * return_ptr;

    value & local(std::size_t idx) {
        return *(
            reinterpret_cast<value *>(reinterpret_cast<uint8_t *>(this) + sizeof(activation))
                + idx
        );
    }

    static activation * create(const CC * return_ptr, std::size_t locals) {
        auto result = reinterpret_cast<activation *>(
            // zero-initialized
            new uint8_t[sizeof(activation) + sizeof(value) * locals] {}
        );

        result->return_ptr = return_ptr;
        return result;
    }

    static void drop(activation * act) {
        delete[] reinterpret_cast<uint8_t *>(act);
    }
};


static std::ofstream log_file { "./log.txt" };


static uint32_t to_fixnum(int32_t x) noexcept {
    return ((static_cast<uint32_t>(x) << 1) + 1) & UINT32_MAX;
}

static int32_t from_fixnum(uint32_t x) noexcept {
    return static_cast<int32_t>(x >> 1);
}

// static bool is_fixnum(uint32_t x) noexcept {
//     return (x & 1) != 0;
// }

static std::shared_ptr<bytecode_contents> read_bytecode(const char * filename) {
    std::ifstream bytecode_file { filename, std::ios::in | std::ios::binary | std::ios::ate };

    if (!bytecode_file) {
        throw std::invalid_argument { "Cannot open file" };
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

// TODO make static method of activation, add "drop"

static void interpret(const std::shared_ptr<bytecode_contents> & bytecode) {
    std::shared_ptr<CC[]> converted {
        new CC[bytecode->code_size],
        std::default_delete<CC[]> {},
    };

    std::shared_ptr<value[]> global_area {
        new value[bytecode->global_size],
        std::default_delete<value[]> {},
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

        std::size_t current_function_idx = UINT32_MAX;
        std::size_t current_function_args = 0;
        std::size_t current_function_locals = 0;

        std::size_t bytecode_idx = 0;
        std::size_t converted_idx = 1;
        while (bytecode_idx < bytecode->code_size) {
            const IC code = static_cast<IC>(bytecode->code_ptr[bytecode_idx++]);

            log_file << "0x" << std::hex << bytecode_idx - 1
                     << " (0x" << converted_idx
                     << "). 0x" << code << std::dec
                     << std::endl;

            if ((0xF0 & code) == 0xF0) {
                break;
            }

            if (code == IC::LINE) {
                bytecode_idx += 4;
                continue;
            }

            if (code == IC::BEGIN || code == IC::CBEGIN) {
                current_function_idx = converted_idx;
                current_function_args = *reinterpret_cast<const uint32_t *>(
                    bytecode->code_ptr + bytecode_idx
                );
                current_function_locals = *reinterpret_cast<const uint32_t *>(
                    bytecode->code_ptr + bytecode_idx + 4
                );
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
            case CONST:
            case CALLC:
            case ARRAY:
            case CALL_Barray: {
                converted[converted_idx++].fixnum = to_fixnum(
                    *reinterpret_cast<const uint32_t *>(bytecode->code_ptr + bytecode_idx)
                );

                bytecode_idx += 4;
                break;
            }

            // string arg
            case STRING:
                converted[converted_idx++].string = bytecode->string_ptr
                    + *reinterpret_cast<const uint32_t *>(bytecode->code_ptr + bytecode_idx);

                bytecode_idx += 4;
                break;

            // offset in code arg
            case JMP:
            case CJMPz:
            case CJMPnz:
                // TODO convert bytecode offset to converted offset

                converted[converted_idx++].code = converted_base
                    + *reinterpret_cast<const uint32_t *>(bytecode->code_ptr + bytecode_idx);

                bytecode_idx += 4;
                break;

            // G(int) arg
            case LD_G:
            case LDA_G:
            case ST_G:
                converted[converted_idx++].global = global_area_base
                    + *reinterpret_cast<const uint32_t *>(bytecode->code_ptr + bytecode_idx);

                bytecode_idx += 4;
                break;

            // A(int) arg
            case LD_A:
            case LDA_A:
            case ST_A: {
                if (current_function_idx == UINT32_MAX) {
                    throw std::invalid_argument { "cannot use arguments outside of function" };
                }

                const std::size_t idx = *reinterpret_cast<const uint32_t *>(
                    bytecode->code_ptr + bytecode_idx
                );

                bytecode_idx += 4;

                if (idx >= current_function_args) {
                    throw std::invalid_argument {
                        "argument index is greater than number of arguments"
                    };
                }

                converted[converted_idx++].index = idx;
                break;
            }

            // L(int) arg
            case LD_L:
            case LDA_L:
            case ST_L: {
                if (current_function_idx == UINT32_MAX) {
                    throw std::invalid_argument { "cannot use locals outside of function" };
                }

                const std::size_t idx = *reinterpret_cast<const uint32_t *>(
                    bytecode->code_ptr + bytecode_idx
                );

                bytecode_idx += 4;

                if (idx >= current_function_locals) {
                    throw std::invalid_argument {
                        "local index is greater than number of locals"
                    };
                }

                converted[converted_idx++].index = idx + current_function_args;
                break;
            }

            // TODO C(int) arg
            case LD_C:
            case LDA_C:
            case ST_C:
                bytecode_idx += 4;
                break;

            // 2 args
            case SEXP:
            case BEGIN:
            case CBEGIN:
            case CALL:
            case TAG:
            case FAIL:
                converted[converted_idx++].args = bytecode->code_ptr + bytecode_idx;
                bytecode_idx += 8;
                break;

            // CLOSURE
            case CLOSURE: {
                converted[converted_idx++].args = bytecode->code_ptr + bytecode_idx;

                const std::size_t n = *reinterpret_cast<const uint32_t *>(
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
            throw std::invalid_argument { "No <end> at end of bytecode" };
            return;
        }
    }

#define NEXT do { goto *(*ip++).interpreter; } while (false)

    activation * current_activation = nullptr;
    const CC * ip = converted_base + 1;
    const CC * rip = converted_base; // TODO set in CALL and CALLC

    std::stack<activation *> activations;
    std::stack<value> stack;
    stack.emplace();
    stack.emplace();
    stack.emplace();

    NEXT;

finish:
    return;

I_BINOP_add:
    goto I_unsupported;

I_BINOP_sub:
    goto I_unsupported;

I_BINOP_mul: {
    const int32_t b = from_fixnum(*reinterpret_cast<int32_t *>(&stack.top()));
    stack.pop();

    const int32_t a = from_fixnum(*reinterpret_cast<int32_t *>(&stack.top()));
    stack.pop();

    stack.emplace(to_fixnum(a * b));
    NEXT;
}

I_BINOP_div:
    goto I_unsupported;

I_BINOP_rem:
    goto I_unsupported;

I_BINOP_lt:
    goto I_unsupported;

I_BINOP_le:
    goto I_unsupported;

I_BINOP_gt:
    goto I_unsupported;

I_BINOP_ge:
    goto I_unsupported;

I_BINOP_eq:
    goto I_unsupported;

I_BINOP_ne:
    goto I_unsupported;

I_BINOP_and:
    goto I_unsupported;

I_BINOP_or:
    goto I_unsupported;

I_CONST: {
    const uint32_t imm = *reinterpret_cast<const uint32_t *>(ip++);
    stack.emplace(imm);
    NEXT;
}

I_STRING:
    goto I_unsupported;

I_SEXP:
    goto I_unsupported;

I_STI:
    goto I_unsupported;

I_STA:
    goto I_unsupported;

I_JMP:
    goto I_unsupported;

I_END: {
    activations.pop();
    ip = reinterpret_cast<const CC *>(current_activation->return_ptr);

    activation::drop(current_activation);

    current_activation = activations.empty() ? nullptr : activations.top();
    NEXT;
}

I_RET:
    goto I_unsupported;

I_DROP:
    stack.pop();
    NEXT;

I_DUP:
    goto I_unsupported;

I_SWAP:
    goto I_unsupported;

I_ELEM:
    goto I_unsupported;

I_LD_G: {
    auto * const imm = (ip++)->global;
    stack.push(*imm);
    NEXT;
}

I_LD_AL:
    goto I_unsupported;

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
    *imm = stack.top();
    NEXT;
}

I_ST_AL:
    goto I_unsupported;

I_ST_C:
    goto I_unsupported;

I_CJMPz:
    goto I_unsupported;

I_CJMPnz:
    goto I_unsupported;

I_BEGIN: {
    auto * const imm = reinterpret_cast<const uint32_t *>((ip++)->args);
    auto * const act = activation::create(rip, imm[0] + imm[1]);
    rip = nullptr;

    for (std::size_t i = imm[0]; i > 0; --i) {
        act->local(i - 1) = stack.top();
        stack.pop();
    }

    current_activation = act;
    activations.push(act);
    NEXT;
}

I_CBEGIN:
    goto I_unsupported;

I_CLOSURE:
    goto I_unsupported;

I_CALLC:
    goto I_unsupported;

I_CALL:
    goto I_unsupported;

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

    stack.emplace(to_fixnum(value));
    NEXT;
}

I_CALL_Lwrite: {
    const int32_t value = from_fixnum(*reinterpret_cast<int32_t *>(&stack.top()));
    stack.pop();

    std::cout << value << std::endl;
    NEXT;
}

I_CALL_Llength:
    goto I_unsupported;

I_CALL_Lstring:
    goto I_unsupported;

I_CALL_Barray:
    goto I_unsupported;

I_unsupported: // unsupported instruction
    std::ostringstream os;

    os << "Unsupported instruction at 0x" << std::hex << ip - converted_base - 1;
    throw std::invalid_argument { os.str() };

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
