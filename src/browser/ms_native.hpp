#pragma once

#include <quickjs.h>
#include <vector>
#include <string>
#include <unordered_map>
#include <limits>
#include <cmath>
#include <algorithm>
#include <iostream>

// OPCODES DEFINITION
enum Opcodes {
    OPCODE_TYPE = 1,
    OPCODE_VARIABLE_TYPE = 2,
    OPCODE_PROPERTY_TYPE = 3,
    OPCODE_LOAD_IMPORT = 4,
    OPCODE_LOAD_THIS = 5,
    OPCODE_LOAD_GLOBAL = 6,
    OPCODE_LOAD_VALUE = 10,
    OPCODE_LOAD_LOCAL = 11,
    OPCODE_LOAD_VARIABLE = 12,
    OPCODE_LOAD_LOCAL_OBJECT = 13,
    OPCODE_LOAD_VARIABLE_OBJECT = 14,
    OPCODE_POP = 15,
    OPCODE_LOAD_PROPERTY = 16,
    OPCODE_LOAD_PROPERTY_OBJECT = 17,
    OPCODE_CREATE_OBJECT = 18,
    OPCODE_MAKE_OBJECT = 19,
    OPCODE_CREATE_ARRAY = 20,
    OPCODE_STORE_LOCAL = 21,
    OPCODE_STORE_LOCAL_POP = 22,
    OPCODE_STORE_VARIABLE = 23,
    OPCODE_CREATE_PROPERTY = 24,
    OPCODE_STORE_PROPERTY = 25,
    OPCODE_DELETE = 26,
    OPCODE_UPDATE_CLASS = 27,
    OPCODE_CREATE_CLASS = 28,
    OPCODE_NEW_CALL = 29,
    OPCODE_ADD = 30,
    OPCODE_SUB = 31,
    OPCODE_MUL = 32,
    OPCODE_DIV = 33,
    OPCODE_MODULO = 34,
    OPCODE_BINARY_AND = 35,
    OPCODE_BINARY_OR = 36,
    OPCODE_SHIFT_LEFT = 37,
    OPCODE_SHIFT_RIGHT = 38,
    OPCODE_NEGATE = 39,
    OPCODE_EQ = 40,
    OPCODE_NEQ = 41,
    OPCODE_LT = 42,
    OPCODE_GT = 43,
    OPCODE_LTE = 44,
    OPCODE_GTE = 45,
    OPCODE_NOT = 50,
    OPCODE_LOAD_PROPERTY_ATOP = 68,
    OPCODE_JUMP = 80,
    OPCODE_JUMPY = 81,
    OPCODE_JUMPN = 82,
    OPCODE_JUMPY_NOPOP = 83,
    OPCODE_JUMPN_NOPOP = 84,
    OPCODE_LOAD_ROUTINE = 89,
    OPCODE_FUNCTION_CALL = 90,
    OPCODE_FUNCTION_APPLY_VARIABLE = 91,
    OPCODE_FUNCTION_APPLY_PROPERTY = 92,
    OPCODE_SUPER_CALL = 93,
    OPCODE_RETURN = 94,
    OPCODE_FORLOOP_INIT = 95,
    OPCODE_FORLOOP_CONTROL = 96,
    OPCODE_FORIN_INIT = 97,
    OPCODE_FORIN_CONTROL = 98,
    OPCODE_UNARY_OP = 100,
    OPCODE_BINARY_OP = 101,
    OPCODE_AFTER = 110,
    OPCODE_EVERY = 111,
    OPCODE_DO = 112,
    OPCODE_SLEEP = 113,
    OPCODE_COMPILED = 200
};

extern JSClassID js_routine_class_id;
extern JSClassID js_processor_class_id;

struct Routine {
    int num_args = 0;
    std::vector<int> opcodes;
    JSValue arg1; // JS array
    std::vector<JSValue> ref;
    int label_count = 0;
    std::unordered_map<std::string, int> labels;
    bool transpile = false;
    std::vector<int> import_refs;
    std::vector<JSValue> import_values;
    int import_self = -1;
    int locals_size = 0;
    bool uses_arguments = false;
    JSValue callback = JS_UNDEFINED;
    JSValue object = JS_UNDEFINED;
    JSValue as_function = JS_UNDEFINED;

    Routine() = default;
    void free_values(JSContext* ctx) {
        for (auto v : ref) JS_FreeValue(ctx, v);
        for (auto v : import_values) JS_FreeValue(ctx, v);
        JS_FreeValue(ctx, callback);
        JS_FreeValue(ctx, object);
        JS_FreeValue(ctx, as_function);
        JS_FreeValue(ctx, arg1);
    }
};

struct CallFrame {
    Routine* routine = nullptr;
    JSValue object = JS_UNDEFINED;
    JSValue sup = JS_UNDEFINED;
    std::string supername;
    int op_index = 0;
};

struct Processor {
    JSValue runner = JS_UNDEFINED;
    std::vector<JSValue> locals;
    std::vector<JSValue> stack;
    std::vector<CallFrame> call_stack;
    bool log = false;
    double time_limit = std::numeric_limits<double>::infinity();
    bool done = true;

    Routine* routine = nullptr;
    int local_index = 0;
    int stack_index = -1;
    int op_index = 0;
    int call_stack_index = 0;
    JSValue global = JS_UNDEFINED;
    JSValue object = JS_UNDEFINED;
    int locals_offset = 0;
    JSValue call_super = JS_UNDEFINED;
    std::string call_supername = "";

    Processor() = default;
    void free_values(JSContext* ctx) {
        JS_FreeValue(ctx, runner);
        for (auto v : locals) JS_FreeValue(ctx, v);
        for (auto v : stack) JS_FreeValue(ctx, v);
        for (auto& frame : call_stack) {
            JS_FreeValue(ctx, frame.object);
            JS_FreeValue(ctx, frame.sup);
        }
        JS_FreeValue(ctx, global);
        JS_FreeValue(ctx, object);
        JS_FreeValue(ctx, call_super);
    }
};

// for testing this has to be fetched as extern
void js_init_native_vm(JSContext* ctx, JSValue ns);
