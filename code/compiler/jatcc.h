#ifndef JATCC_H
#define JATCC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#define MAX_OUTPUT_SIZE 1024


extern int64_t MAX_SIZE;

// code segment 代码段
extern int64_t *code;
extern int64_t *code_dump;  // for dump
extern int64_t *stack;      // stack segment 运行栈
extern char *data;      // data segment 数据段


// 寄存器
extern int64_t *pc;  // pc register
extern int64_t *sp;  // rsp register stack pomy_inter
extern int64_t *bp;  // rbp register base pomy_inter


extern int64_t ax;     // common register
extern int64_t cycle;


// 指令集
enum {
    IMM, LEA, JMP, JZ, JNZ, CALL, NVAR, DARG, RET, LI, LC, SI, SC, PUSH,
    OR, XOR, AND, EQ, NE, LT, GT, LE, GE, SHL, SHR, ADD, SUB, MUL, DIV, MOD,
    OPEN, READ, CLOS, PRTF, MALC, FREE, MSET, MCMP, EXIT
};

// classes/keywords
enum {
    Num = 128, Fun, Sys, Glo, Loc, Id,
    Char, Int, Enum, If, Else, Return, Sizeof, While,
    Assign, Cond, Lor, Land, Or, Xor, And, Eq, Ne, Lt, Gt, Le, Ge,
    Shl, Shr, Add, Sub, Mul, Div, Mod, Inc, Dec, Brak
};

// 符号表项
enum {Token, Hash, Name, Class, Type, Value, GClass, GType, GValue, SymSize};

// 变量类型
enum {CHAR, INT, PTR};


// src code & dump 源代码
extern char *src;
extern char *src_dump;


// 符号表指针
extern int64_t *symbol_table;  // 符号表
extern int64_t *symbol_ptr;    // 当前正在操作的符号表记录
extern int64_t *main_ptr;      // main函数的符号表记录


extern int64_t line;
extern int64_t token_val;
extern int64_t token;

// 声明函数原型
void tokenize();
void my_assert(int64_t tk);
void check_local_id();
void check_new_id();
void parse_enum();
int64_t parse_base_type();
void hide_global();
void recover_global();
void parse_param();
void parse_expr(int64_t precd);

void parse_stmt();
void parse_fun();
void parse();
void keyword();
int64_t init_vm();
char* run_vm(int64_t argc, char** argv);
void write_as();
int64_t load_srcCode(const char* code);
char* Jatcc(const char* code); 

#ifdef __cplusplus
}
#endif

#endif