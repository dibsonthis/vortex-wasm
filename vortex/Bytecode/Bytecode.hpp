#pragma once
#include <variant>
#include <functional>
#include <unordered_map>
#include <iomanip>
#include <cmath>
#include <string>
#include "../Node/Node.hpp"

#define value_ptr std::shared_ptr<Value>

uint8_t *int_to_bytes(int &integer);

int bytes_to_int(uint8_t a, uint8_t b, uint8_t c, uint8_t d);

enum OpCode
{
    OP_RETURN,
    OP_YIELD,
    OP_LOAD_CONST,
    OP_LOAD_THIS,
    OP_NEGATE,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_MOD,
    OP_POW,
    OP_AND,
    OP_OR,
    OP_NOT,
    OP_EQ_EQ,
    OP_NOT_EQ,
    OP_LT_EQ,
    OP_GT_EQ,
    OP_LT,
    OP_GT,
    OP_RANGE,
    OP_DOT,
    OP_STORE_VAR,
    OP_LOAD,
    OP_LOAD_GLOBAL,
    OP_LOAD_CLOSURE,
    OP_SET,
    OP_SET_FORCE,
    OP_SET_PROPERTY,
    OP_SET_CLOSURE,
    OP_MAKE_CLOSURE,
    OP_MAKE_TYPE,
    OP_MAKE_TYPED,
    OP_MAKE_OBJECT,
    OP_MAKE_FUNCTION,
    OP_MAKE_CONST,
    OP_MAKE_NON_CONST,
    OP_TYPE_DEFAULTS,
    OP_POP,
    OP_POP_CLOSE,
    OP_JUMP_IF_FALSE,
    OP_JUMP_IF_TRUE,
    OP_POP_JUMP_IF_FALSE,
    OP_POP_JUMP_IF_TRUE,
    OP_JUMP,
    OP_JUMP_BACK,
    OP_EXIT,
    OP_BREAK,
    OP_CONTINUE,
    OP_BUILD_LIST,
    OP_ACCESSOR,
    OP_LEN,
    OP_CALL,
    OP_CALL_METHOD,
    OP_IMPORT,
    OP_UNPACK,
    OP_REMOVE_PUSH,
    OP_SWAP_TOS,
    OP_LOOP,
    OP_LOOP_END,
    OP_ITER,
    OP_HOOK_ONCHANGE,
    OP_HOOK_CLOSURE_ONCHANGE,
    OP_HOOK_ONACCESS,
    OP_HOOK_CLOSURE_ONACCESS,
    OP_TRY_BEGIN,
    OP_TRY_END,
    OP_CATCH_BEGIN
};

enum ValueType
{
    Number,
    String,
    Boolean,
    List,
    Type,
    Object,
    Function,
    Native,
    Pointer,
    None
};

struct Value;
struct Closure;

std::string toString(Value value);

struct Chunk
{
    std::vector<uint8_t> code;
    std::vector<int> lines;
    std::vector<Value> constants;
    std::vector<std::string> variables;
    std::vector<std::string> public_variables;
    std::string import_path;
};

struct ClosedVar
{
    std::string name;
    int index;
    bool is_local;
};

struct FunctionObj
{
    std::string name;
    int arity;
    int defaults;
    Chunk chunk;
    std::vector<int> instruction_offsets;
    std::vector<ClosedVar> closed_var_indexes;
    std::vector<std::shared_ptr<Closure>> closed_vars;
    std::vector<Value> default_values;
    std::shared_ptr<Value> object;
    std::vector<std::string> params;
    bool is_generator = false;
    bool generator_init = false;
    bool generator_done = false;
    bool is_type_generator = false;
    std::string import_path;
};

struct TypeObj
{
    std::string name;
    std::unordered_map<std::string, Value> types;
    std::unordered_map<std::string, Value> defaults;
};

struct ObjectObj
{
    std::shared_ptr<TypeObj> type;
    std::unordered_map<std::string, Value> values;
    std::vector<std::string> keys;
    std::string type_name;
};

struct PointerObj
{
    void *value;
};

typedef Value (*NativeFunction)(std::vector<Value> &args);

struct NativeFunctionObj
{
    std::string name;
    NativeFunction function = nullptr;
};

struct Meta
{
    bool unpack = false;
    bool packer = false;
    bool is_const = false;
    bool temp_non_const = false;
};

struct ValueHooks
{
    std::shared_ptr<Value> onChangeHook = nullptr;
    std::string onChangeHookName;
    std::shared_ptr<Value> onAccessHook = nullptr;
    std::string onAccessHookName;
};

std::string toString(Value value);

static long long int v_counter = 0;

struct Value
{
    ValueType type;
    Meta meta;
    ValueHooks hooks;
    long long int id;
    std::variant<
        double,
        std::string,
        bool,
        std::shared_ptr<std::vector<Value>>,
        std::shared_ptr<FunctionObj>,
        std::shared_ptr<TypeObj>,
        std::shared_ptr<ObjectObj>,
        std::shared_ptr<NativeFunctionObj>,
        std::shared_ptr<PointerObj>>
        value;

    Value() : type(None)
    {
        id = ++v_counter;
    }
    Value(ValueType type) : type(type)
    {
        id = ++v_counter;
        switch (type)
        {
        case Number:
            value = 0.0f;
            break;
        case String:
            value = "";
            break;
        case Boolean:
            value = false;
            break;
        case List:
            value = std::make_shared<std::vector<Value>>();
            break;
        case Type:
            value = std::make_shared<TypeObj>();
            break;
        case Object:
            value = std::make_shared<ObjectObj>();
            break;
        case Function:
            value = std::make_shared<FunctionObj>();
            break;
        case Native:
            value = std::make_shared<NativeFunctionObj>();
            break;
        case Pointer:
            value = std::make_shared<PointerObj>();
            break;
        default:
            break;
        }
    }

    double &get_number()
    {
        return std::get<double>(this->value);
    }
    std::string &get_string()
    {
        return std::get<std::string>(this->value);
    }
    bool &get_boolean()
    {
        return std::get<bool>(this->value);
    }

    std::shared_ptr<std::vector<Value>> &get_list()
    {
        return std::get<std::shared_ptr<std::vector<Value>>>(this->value);
    }

    std::shared_ptr<TypeObj> &get_type()
    {
        return std::get<std::shared_ptr<TypeObj>>(this->value);
    }

    std::shared_ptr<ObjectObj> &get_object()
    {
        return std::get<std::shared_ptr<ObjectObj>>(this->value);
    }

    std::shared_ptr<FunctionObj> &get_function()
    {
        return std::get<std::shared_ptr<FunctionObj>>(this->value);
    }

    std::shared_ptr<NativeFunctionObj> &get_native()
    {
        return std::get<std::shared_ptr<NativeFunctionObj>>(this->value);
    }

    std::shared_ptr<PointerObj> &get_pointer()
    {
        return std::get<std::shared_ptr<PointerObj>>(this->value);
    }

    bool is_number()
    {
        return type == Number;
    }
    bool is_string()
    {
        return type == String;
    }
    bool is_boolean()
    {
        return type == Boolean;
    }
    bool is_list()
    {
        return type == List;
    }
    bool is_type()
    {
        return type == Type;
    }
    bool is_object()
    {
        return type == Object;
    }
    bool is_function()
    {
        return type == Function;
    }
    bool is_native()
    {
        return type == Native;
    }
    bool is_pointer()
    {
        return type == Pointer;
    }
    bool is_none()
    {
        return type == None;
    }
    std::string type_repr()
    {
        switch (type)
        {
        case Number:
            return "Number";
        case String:
            return "String";
        case Boolean:
            return "Boolean";
        case List:
            return "List";
        case Object:
            return "Object";
        case Function:
            return "Function";
        case Native:
            return "Native";
        case Pointer:
            return "Pointer";
        case Type:
            return "Type";
        case None:
            return "None";
        default:
            return "Unknown";
        }
    }

    std::string value_repr()
    {
        return toString(*this);
    }
};

struct Closure
{
    std::string name;
    std::string frame_name;
    bool is_local;
    int index;
    Value *location;
    Value closed;
    Value *initial_location;
};

Value new_val();
Value number_val(double value);
Value string_val(std::string value);
Value boolean_val(bool value);
Value list_val();
Value type_val(std::string name);
Value object_val();
Value function_val();
Value native_val();
Value pointer_val();
Value none_val();

void printValue(Value value);

void add_code(Chunk &chunk, uint8_t code, int line = 0);

void add_opcode(Chunk &chunk, uint8_t op, int operand, int line = 0);

int add_constant(Chunk &chunk, Value value);

void add_constant_code(Chunk &chunk, Value value, int line = 0);

void add_bytes(Chunk &chunk, Value value, uint8_t op, int line = 0);

void patch_bytes(Chunk &chunk, int offset, uint8_t *bytes);

static int simple_instruction(std::string name, int offset);

static int constant_instruction(std::string name, Chunk &chunk, int offset);
static int op_code_instruction(std::string name, Chunk &chunk, int offset);

int disassemble_instruction(Chunk &chunk, int offset);

void disassemble_chunk(Chunk &chunk, std::string name);

int advance(Chunk &chunk, int offset);

std::vector<int> instruction_offsets(Chunk &chunk);