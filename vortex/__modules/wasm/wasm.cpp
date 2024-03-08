#include <functional>
#include "../../Node/Node.hpp"
#include "../../Lexer/Lexer.hpp"
#include "../../Parser/Parser.hpp"
#include "../../Bytecode/Bytecode.hpp"
#include "../../Bytecode/Generator.hpp"
#include "../../VirtualMachine/VirtualMachine.hpp"

#include <emscripten.h>
#include <emscripten/fetch.h>
#include <emscripten/websocket.h>

extern "C" Value run_script(std::vector<Value> &args)
{
    int num_required_args = 1;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'run_script' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value script = args[0];

    if (!script.is_string())
    {
        return error_object("Parameter 'script' must be a string");
    }

    emscripten_run_script(script.get_string().c_str());

    return none_val();
}

extern "C" Value run_script_number(std::vector<Value> &args)
{
    int num_required_args = 1;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'run_script_number' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value script = args[0];

    if (!script.is_string())
    {
        return error_object("Parameter 'script' must be a string");
    }

    int res = emscripten_run_script_int(script.get_string().c_str());

    return number_val(res);
}

extern "C" Value run_script_string(std::vector<Value> &args)
{
    int num_required_args = 1;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'run_script_string' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value script = args[0];

    if (!script.is_string())
    {
        return error_object("Parameter 'script' must be a string");
    }

    std::string res = emscripten_run_script_string(script.get_string().c_str());

    return string_val(res);
}

extern "C" Value get_ptr(std::vector<Value> &args)
{
    int num_required_args = 1;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'get_ptr' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    auto ptr = new Value(args[0]);

    auto ptr_num = reinterpret_cast<uintptr_t>(ptr);

    return number_val(ptr_num);
}

extern "C" Value make_closure(std::vector<Value> &args)
{
    int num_required_args = 2;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'make_closure' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value vm_ptr = args[1];

    if (!vm_ptr.is_pointer())
    {
        return error_object("Parameter 'vm_ptr' must be a pointer");
    }

    VM *vm = (VM *)vm_ptr.get_pointer()->value;

    Value *hoisted = new Value(args[0]);

    auto ptr_num = reinterpret_cast<uintptr_t>(hoisted);

    vm->hoisted[ptr_num] = {&args[0], hoisted};

    return number_val(ptr_num);
}

extern "C" Value delete_closure(std::vector<Value> &args)
{
    int num_required_args = 2;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'delete_closure' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value ptr_num = args[0];
    Value vm_ptr = args[1];

    if (!ptr_num.is_number())
    {
        return error_object("Parameter 'ptr_num' must be a number");
    }

    if (!vm_ptr.is_pointer())
    {
        return error_object("Parameter 'vm_ptr' must be a pointer");
    }

    VM *vm = (VM *)vm_ptr.get_pointer()->value;

    delete (vm->hoisted[ptr_num.get_number()].second);

    vm->hoisted.erase(ptr_num.get_number());

    return none_val();
}

void callbackWrapper(void *arg)
{
    // Cast the argument back to the shared pointer type
    auto func = *static_cast<std::shared_ptr<Value> *>(arg);

    VM func_vm;
    std::shared_ptr<FunctionObj> main = std::make_shared<FunctionObj>();
    main->name = "";
    main->arity = 0;
    main->chunk = Chunk();
    CallFrame main_frame;
    main_frame.function = main;
    main_frame.sp = 0;
    main_frame.ip = main->chunk.code.data();
    main_frame.frame_start = 0;
    func_vm.frames.push_back(main_frame);

    add_constant(main->chunk, *func);
    add_opcode(main->chunk, OP_LOAD_CONST, 0, 0);
    add_opcode(main->chunk, OP_CALL, 0, 0);

    add_code(main->chunk, OP_EXIT, 0);

    auto offsets = instruction_offsets(main_frame.function->chunk);
    main_frame.function->instruction_offsets = offsets;

    evaluate(func_vm);
}

extern "C" Value set_main_loop(std::vector<Value> &args)
{
    int num_required_args = 2;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'set_main_loop' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value func = args[0];
    Value loop = args[1];

    if (!func.is_function())
    {
        return error_object("Function 'set_main_loop' expects arg 'func' to be a function");
    }

    if (!loop.is_boolean())
    {
        return error_object("Function 'set_main_loop' expects arg 'simulate_infinite_loop' to be a boolean");
    }

    auto func_ptr = std::make_shared<Value>(func);

    emscripten_set_main_loop_arg(callbackWrapper, &func_ptr, 0, (int)loop.get_boolean());

    return none_val();
}

/* FETCH API */

void onSuccessWrapper(struct emscripten_fetch_t *fetch)
{
    auto unpacked_data = static_cast<std::vector<Value *> *>(fetch->userData);
    auto func = (*unpacked_data)[0];
    std::string data(fetch->data, fetch->numBytes);

    VM func_vm;
    std::shared_ptr<FunctionObj> main = std::make_shared<FunctionObj>();
    main->name = "";
    main->arity = 0;
    main->chunk = Chunk();
    CallFrame main_frame;
    main_frame.function = main;
    main_frame.sp = 0;
    main_frame.ip = main->chunk.code.data();
    main_frame.frame_start = 0;
    func_vm.frames.push_back(main_frame);

    add_constant(main->chunk, *func);
    add_constant(main->chunk, string_val(data));
    add_opcode(main->chunk, OP_LOAD_CONST, 1, 0);
    add_opcode(main->chunk, OP_LOAD_CONST, 0, 0);
    add_opcode(main->chunk, OP_CALL, 1, 0);

    add_code(main->chunk, OP_EXIT, 0);

    auto offsets = instruction_offsets(main_frame.function->chunk);
    main_frame.function->instruction_offsets = offsets;

    evaluate(func_vm);

    emscripten_fetch_close(fetch); // Free resources
    free(func);
}

void onFailWrapper(struct emscripten_fetch_t *fetch)
{
    auto unpacked_data = static_cast<std::vector<Value *> *>(fetch->userData);
    auto func = (*unpacked_data)[1];
    std::string data(fetch->data, fetch->numBytes);

    VM func_vm;
    std::shared_ptr<FunctionObj> main = std::make_shared<FunctionObj>();
    main->name = "";
    main->arity = 0;
    main->chunk = Chunk();
    CallFrame main_frame;
    main_frame.function = main;
    main_frame.sp = 0;
    main_frame.ip = main->chunk.code.data();
    main_frame.frame_start = 0;
    func_vm.frames.push_back(main_frame);

    add_constant(main->chunk, *func);
    add_constant(main->chunk, string_val(data));
    add_opcode(main->chunk, OP_LOAD_CONST, 1, 0);
    add_opcode(main->chunk, OP_LOAD_CONST, 0, 0);
    add_opcode(main->chunk, OP_CALL, 1, 0);

    add_code(main->chunk, OP_EXIT, 0);

    auto offsets = instruction_offsets(main_frame.function->chunk);
    main_frame.function->instruction_offsets = offsets;

    evaluate(func_vm);

    emscripten_fetch_close(fetch); // Free resources
    free(func);
}

extern "C" Value fetch_get(std::vector<Value> &args)
{
    int num_required_args = 4;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'fetch_get' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value url = args[0];
    Value success_callback = args[1];
    Value fail_callback = args[2];
    Value headers = args[3];

    if (!url.is_string())
    {
        return error_object("Function 'fetch_get' expects arg 'url' to be a string");
    }

    if (!success_callback.is_function())
    {
        return error_object("Function 'fetch_get' expects arg 'func' to be a function");
    }

    if (!fail_callback.is_function())
    {
        return error_object("Function 'fetch_get' expects arg 'fail_callback' to be a function");
    }

    if (!headers.is_object())
    {
        return error_object("Function 'fetch_get' expects argument 'headers' to be a object");
    }

    auto success_callback_ptr = new Value(success_callback);
    auto fail_callback_ptr = new Value(fail_callback);

    void *data = new std::vector<Value *>{success_callback_ptr, fail_callback_ptr};

    std::vector<std::string> headers_;

    for (auto key : headers.get_object()->keys)
    {
        auto value = headers.get_object()->values[key];
        if (!value.is_string())
        {
            continue;
        }
        headers_.push_back(key);
        headers_.push_back(value.get_string());
    }

    // Allocate memory for an array of const char pointers
    const char **headersArray = new const char *[headers_.size() + 1]; // +1 for NULL terminator

    // Copy each string from the vector to the array
    for (size_t i = 0; i < headers_.size(); ++i)
    {
        headersArray[i] = strdup(headers_[i].c_str());
    }

    // Add a NULL terminator at the end
    headersArray[headers_.size()] = NULL;

    emscripten_fetch_attr_t attr;
    emscripten_fetch_attr_init(&attr);
    strcpy(attr.requestMethod, "GET");
    attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;
    attr.onsuccess = onSuccessWrapper;
    attr.onerror = onFailWrapper;
    attr.userData = data;
    attr.requestHeaders = headersArray;
    emscripten_fetch(&attr, url.get_string().c_str());

    return none_val();
}

/* Websockets */

EM_BOOL socketOpenWrapper(int eventType, const EmscriptenWebSocketOpenEvent *websocketEvent, void *userData)
{
    auto func = static_cast<Value *>(userData);

    VM func_vm;
    std::shared_ptr<FunctionObj> main = std::make_shared<FunctionObj>();
    main->name = "";
    main->arity = 0;
    main->chunk = Chunk();
    CallFrame main_frame;
    main_frame.function = main;
    main_frame.sp = 0;
    main_frame.ip = main->chunk.code.data();
    main_frame.frame_start = 0;
    func_vm.frames.push_back(main_frame);

    add_constant(main->chunk, *func);
    add_opcode(main->chunk, OP_LOAD_CONST, 0, 0);
    add_opcode(main->chunk, OP_CALL, 0, 0);

    add_code(main->chunk, OP_EXIT, 0);

    auto offsets = instruction_offsets(main_frame.function->chunk);
    main_frame.function->instruction_offsets = offsets;

    evaluate(func_vm);

    return EM_TRUE;
}

EM_BOOL socketErrorWrapper(int eventType, const EmscriptenWebSocketErrorEvent *websocketEvent, void *userData)
{
    auto func = static_cast<Value *>(userData);

    VM func_vm;
    std::shared_ptr<FunctionObj> main = std::make_shared<FunctionObj>();
    main->name = "";
    main->arity = 0;
    main->chunk = Chunk();
    CallFrame main_frame;
    main_frame.function = main;
    main_frame.sp = 0;
    main_frame.ip = main->chunk.code.data();
    main_frame.frame_start = 0;
    func_vm.frames.push_back(main_frame);

    add_constant(main->chunk, *func);
    add_opcode(main->chunk, OP_LOAD_CONST, 0, 0);
    add_opcode(main->chunk, OP_CALL, 0, 0);

    add_code(main->chunk, OP_EXIT, 0);

    auto offsets = instruction_offsets(main_frame.function->chunk);
    main_frame.function->instruction_offsets = offsets;

    evaluate(func_vm);

    return EM_TRUE;
}

EM_BOOL socketCloseWrapper(int eventType, const EmscriptenWebSocketCloseEvent *websocketEvent, void *userData)
{
    auto func = static_cast<Value *>(userData);

    int code = websocketEvent->code;
    std::string reason = websocketEvent->reason;

    auto data = object_val();
    data.get_object()->keys = {"code", "reason"};
    data.get_object()->values["code"] = number_val(code);
    data.get_object()->values["reason"] = string_val(reason);

    VM func_vm;
    std::shared_ptr<FunctionObj> main = std::make_shared<FunctionObj>();
    main->name = "";
    main->arity = 0;
    main->chunk = Chunk();
    CallFrame main_frame;
    main_frame.function = main;
    main_frame.sp = 0;
    main_frame.ip = main->chunk.code.data();
    main_frame.frame_start = 0;
    func_vm.frames.push_back(main_frame);

    add_constant(main->chunk, *func);
    add_constant(main->chunk, data);
    add_opcode(main->chunk, OP_LOAD_CONST, 1, 0);
    add_opcode(main->chunk, OP_LOAD_CONST, 0, 0);
    add_opcode(main->chunk, OP_CALL, 1, 0);

    add_code(main->chunk, OP_EXIT, 0);

    auto offsets = instruction_offsets(main_frame.function->chunk);
    main_frame.function->instruction_offsets = offsets;

    evaluate(func_vm);

    return EM_TRUE;
}

EM_BOOL socketMessageWrapper(int eventType, const EmscriptenWebSocketMessageEvent *websocketEvent, void *userData)
{
    auto func = static_cast<Value *>(userData);

    std::string message;

    if (websocketEvent->isText)
    {
        message = reinterpret_cast<char *>(websocketEvent->data);
    }

    auto msg = string_val(message);

    VM func_vm;
    std::shared_ptr<FunctionObj> main = std::make_shared<FunctionObj>();
    main->name = "";
    main->arity = 0;
    main->chunk = Chunk();
    CallFrame main_frame;
    main_frame.function = main;
    main_frame.sp = 0;
    main_frame.ip = main->chunk.code.data();
    main_frame.frame_start = 0;
    func_vm.frames.push_back(main_frame);

    add_constant(main->chunk, *func);
    add_constant(main->chunk, msg);
    add_opcode(main->chunk, OP_LOAD_CONST, 1, 0);
    add_opcode(main->chunk, OP_LOAD_CONST, 0, 0);
    add_opcode(main->chunk, OP_CALL, 1, 0);

    add_code(main->chunk, OP_EXIT, 0);

    auto offsets = instruction_offsets(main_frame.function->chunk);
    main_frame.function->instruction_offsets = offsets;

    evaluate(func_vm);

    return EM_TRUE;
}

extern "C" Value socket_connect(std::vector<Value> &args)
{
    int num_required_args = 1;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'socket_connect' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value url = args[0];

    if (!url.is_string())
    {
        return error_object("Function 'socket_connect' expects arg 'url' to be a string");
    }

    EmscriptenWebSocketCreateAttributes ws_attrs = {
        url.get_string().c_str(),
        NULL,
        false};

    EMSCRIPTEN_WEBSOCKET_T ws = emscripten_websocket_new(&ws_attrs);

    return number_val(ws);
}

extern "C" Value socket_on_open(std::vector<Value> &args)
{
    int num_required_args = 2;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'socket_on_open' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value ws = args[0];
    Value func = args[1];

    auto func_ptr = new Value(func);

    if (!ws.is_number())
    {
        return error_object("Function 'socket_on_open' expects arg 'ws' to be a number");
    }

    if (!func.is_function())
    {
        return error_object("Function 'socket_on_open' expects arg 'func' to be a function");
    }

    emscripten_websocket_set_onopen_callback(ws.get_number(), func_ptr, socketOpenWrapper);

    return none_val();
}

extern "C" Value socket_on_error(std::vector<Value> &args)
{
    int num_required_args = 2;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'socket_on_error' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value ws = args[0];
    Value func = args[1];

    auto func_ptr = new Value(func);

    if (!ws.is_number())
    {
        return error_object("Function 'socket_on_error' expects arg 'ws' to be a number");
    }

    if (!func.is_function())
    {
        return error_object("Function 'socket_on_error' expects arg 'func' to be a function");
    }

    emscripten_websocket_set_onerror_callback(ws.get_number(), func_ptr, socketErrorWrapper);

    return none_val();
}

extern "C" Value socket_on_close(std::vector<Value> &args)
{
    int num_required_args = 2;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'socket_on_close' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value ws = args[0];
    Value func = args[1];

    auto func_ptr = new Value(func);

    if (!ws.is_number())
    {
        return error_object("Function 'socket_on_close' expects arg 'ws' to be a number");
    }

    if (!func.is_function())
    {
        return error_object("Function 'socket_on_close' expects arg 'func' to be a function");
    }

    emscripten_websocket_set_onclose_callback(ws.get_number(), func_ptr, socketCloseWrapper);

    return none_val();
}

extern "C" Value socket_on_message(std::vector<Value> &args)
{
    int num_required_args = 2;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'socket_on_message' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value ws = args[0];
    Value func = args[1];

    auto func_ptr = new Value(func);

    if (!ws.is_number())
    {
        return error_object("Function 'socket_on_message' expects arg 'ws' to be a number");
    }

    if (!func.is_function())
    {
        return error_object("Function 'socket_on_message' expects arg 'func' to be a function");
    }

    emscripten_websocket_set_onmessage_callback(ws.get_number(), func_ptr, socketMessageWrapper);

    return none_val();
}

extern "C" Value socket_send(std::vector<Value> &args)
{
    int num_required_args = 2;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'socket_send' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value ws = args[0];
    Value message = args[1];

    if (!ws.is_number())
    {
        return error_object("Function 'socket_send' expects arg 'ws' to be a number");
    }

    if (!message.is_string())
    {
        return error_object("Function 'socket_send' expects arg 'message' to be a string");
    }

    return number_val(emscripten_websocket_send_utf8_text(ws.get_number(), message.get_string().c_str()));
}

extern "C" Value socket_close(std::vector<Value> &args)
{
    int num_required_args = 3;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'socket_close' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value ws = args[0];
    Value code = args[1];
    Value reason = args[2];

    if (!ws.is_number())
    {
        return error_object("Function 'socket_close' expects arg 'ws' to be a number");
    }

    if (!code.is_number())
    {
        return error_object("Function 'socket_close' expects arg 'code' to be a number");
    }

    if (!reason.is_string())
    {
        return error_object("Function 'socket_close' expects arg 'reason' to be a number");
    }

    return number_val(emscripten_websocket_close(ws.get_number(), code.get_number(), reason.get_string().c_str()));
}

extern "C" Value socket_get_state(std::vector<Value> &args)
{
    int num_required_args = 1;

    if (args.size() != num_required_args)
    {
        return error_object("Function 'socket_get_state' expects " + std::to_string(num_required_args) + " argument(s)");
    }

    Value ws = args[0];

    if (!ws.is_number())
    {
        return error_object("Function 'socket_get_state' expects arg 'ws' to be a number");
    }

    unsigned short *state;

    emscripten_websocket_get_ready_state(ws.get_number(), state);

    return number_val(*state);
}