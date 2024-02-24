
#include "jatcc.h"
int64_t MAX_SIZE;
char output_buffer[MAX_OUTPUT_SIZE];

int64_t * code,         // code segment
    * code_dump,    // for dump
    * stack;        // stack segment
char* data;         // data segment

int64_t * pc,           // pc register
    * sp,           // rsp register
    * bp;           // rbp register

int64_t ax,             // common register
    cycle;


// src code & dump
char* src,
    * src_dump;

// symbol table & point64_ter
int64_t * symbol_table,
    * symbol_ptr,
    * main_ptr;

int64_t token, token_val;
int64_t line;

//词法分析（tokenize/lex）：从源码获取到下一个token，得到这个token的类型和值。
void tokenize() {
    char* ch_ptr;
    //从源代码中取出字符
    while((token = *src++)) {
        if (token == '\n') line++;
        // 不支持宏和预编译指令，遇到#直接视为行注释
        else if (token == '#') while (*src != 0 && *src != '\n') src++;
        // 处理标识符
        else if ((token >= 'a' && token <= 'z') || (token >= 'A' && token <= 'Z') || (token == '_')) {
            ch_ptr = src - 1; //标识符的起始位置
            //循环读取字符，构建标识符，并计算其哈希值。
            while ((*src >= 'a' && *src <= 'z') || (*src >= 'A' && *src <= 'Z')
                    || (*src >= '0' && *src <= '9') || (*src == '_'))
                // 计算字符串的哈希值，用来唯一标识一个标识符，假定不会发生哈希冲突
                token = token * 147 + *src++;
            // 保存标识符和哈希值（哈希值左移6位，并加上标识符的长度，得到最终的哈希值。）
            token = (token << 6) + (src - ch_ptr);
            symbol_ptr = symbol_table;
            // 在符号表中查找相同的标识符：
            while(symbol_ptr[Token]) {
                //如果找到，将当前标识符的类型（token）设置为符号表中相应标识符的类型，并返回。
                if (token == symbol_ptr[Hash] && !memcmp((char*)symbol_ptr[Name], ch_ptr, src - ch_ptr)) {
                    token = symbol_ptr[Token];
                    return;
                }
                symbol_ptr = symbol_ptr + SymSize;
            }
            // 如果没找到，添加新标识符到符号表：
            symbol_ptr[Name] = (int64_t)ch_ptr;
            symbol_ptr[Hash] = token;
            token = symbol_ptr[Token] = Id;
            return;
        }
        // 处理数字
        else if (token >= '0' && token <= '9') {
            // 十进制, ch_ptr with 1 - 9
            if ((token_val = token - '0'))
                while (*src >= '0' && *src <= '9') token_val = token_val * 10 + *src++ - '0';
            //16进制, ch_ptr with 0x
            else if (*src == 'x' || *src == 'X')
                while ((token = *++src) && ((token >= '0' && token <= '9') || (token >= 'a' && token <= 'f')
                        || (token >= 'A' && token <= 'F')))
                    token_val = token_val * 16 + (token & 0xF) + (token >= 'A' ? 9 : 0);
            // 8进制, start with 0
            else while (*src >= '0' && *src <= '7') token_val = token_val * 8 + *src++ - '0';
            token = Num;
            return;
        }
        // 处理字符串和字符
        else if (token == '"' || token == '\'') {
            ch_ptr = data;
            //直至遇到字符串结束符 0 或者与字符串起始符相同的字符（token）。
            while (*src != 0 && *src != token) {
                //处理转义字符
                if ((token_val = *src++) == '\\') {
                    // 只支持换行符\n的转义，将其替换为实际的换行符。
                    if ((token_val = *src++) == 'n') token_val = '\n';
                }
               //如果字符串字面量，将处理后的字符存储到数据段中，同时更新数据指针 data。
                if (token == '"') *data++ = token_val;
            }
            src++;
             // 如果处理的是字符串字面量，将 ch_ptr 转换为整数，并将其作为字符串的标记值
            if (token == '"') token_val = (int64_t)ch_ptr; 
            // 字符字面量，则将 token 设置为 Num 表示数字类型。
            else token = Num;
            return;
        }
        // 处理 '/' 符号，代表注释或除号
        else if (token == '/') {
            //判断下一个字符是否是 /
            if (*src == '/') {
                // 跳过整行注释
                while (*src != 0 && *src != '\n') src++;
            } else {
                //处理除号
                token = Div;
                return;
            }
        }
        // 处理操作符 copy from c4.
        else if (token == '=') {if (*src == '=') {src++; token = Eq;} else token = Assign; return;}
        else if (token == '+') {if (*src == '+') {src++; token = Inc;} else token = Add; return;}
        else if (token == '-') {if (*src == '-') {src++; token = Dec;} else token = Sub; return;}
        else if (token == '!') {if (*src == '=') {src++; token = Ne;} return;}
        else if (token == '<') {if (*src == '=') {src++; token = Le;} else if (*src == '<') {src++; token = Shl;} else token = Lt; return;}
        else if (token == '>') {if (*src == '=') {src++; token = Ge;} else if (*src == '>') {src++; token = Shr;} else token = Gt; return;}
        else if (token == '|') {if (*src == '|') {src++; token = Lor;} else token = Or; return;}
        else if (token == '&') {if (*src == '&') {src++; token = Land;} else token = And; return;}
        else if (token == '^') {token = Xor; return;}
        else if (token == '%') {token = Mod; return;}
        else if (token == '*') {token = Mul; return;}
        else if (token == '[') {token = Brak; return;}
        else if (token == '?') {token = Cond; return;}
        else if (token == '~' || token == ';' || token == '{' || token == '}' || token == '(' || token == ')' || token == ']' || token == ',' || token == ':') return;
    }
}


void my_assert(int64_t tk) {
    if (token != tk) {
        printf("line %lld: expect token: %lld(%c), get: %lld(%c)\n", line, tk, (char)tk, token, (char)token);
        exit(-1);
    }
    tokenize();
}



void check_local_id() {
    if (token != Id) {printf("line %lld: invalid identifer\n", line); exit(-1);}
    if (symbol_ptr[Class] == Loc) {
        printf("line %lld: duplicate declaration\n", line);
        exit(-1);
    }
}

void check_new_id() {
    if (token != Id) {printf("line %lld: invalid identifer\n", line); exit(-1);}
    if (symbol_ptr[Class]) {
        printf("line %lld: duplicate declaration\n", line);
        exit(-1);
    }
}

void parse_enum() {
    int64_t i;
    i = 0; // enum index
    while (token != '}') {
        check_new_id();
        my_assert(Id);
        // handle custom enum index
        if (token == Assign) {my_assert(Assign); my_assert(Num); i = token_val;}
        symbol_ptr[Class] = Num;
        symbol_ptr[Type] = INT;
        symbol_ptr[Value] = i++;
        if (token == ',') tokenize();
    }
}

int64_t parse_base_type() {
    if (token == Char) {my_assert(Char); return CHAR;}
    else {my_assert(Int); return INT;}
}

//全局符号表的隐藏和恢复。
void hide_global() {
    symbol_ptr[GClass] = symbol_ptr[Class];
    symbol_ptr[GType] = symbol_ptr[Type];
    symbol_ptr[GValue] = symbol_ptr[Value];
}

void recover_global() {
    symbol_ptr[Class] = symbol_ptr[GClass];
    symbol_ptr[Type] = symbol_ptr[GType];
    symbol_ptr[Value] = symbol_ptr[GValue];
}

int64_t ibp;
//解析函数的参数，包括参数的类型、指针类型和参数名
void parse_param() {
    int64_t type, i; //参数类型与序号
    i = 0;
    while (token != ')') {
        type = parse_base_type(); //解析基本数据类型，返回类型并赋值给 type
        // parse point64_ter's star
        while (token == Mul) {my_assert(Mul); type = type + PTR;}
        check_local_id(); my_assert(Id);//检查并解析局部标识符（参数名），确保当前 token 是标识符。
        //隐藏全局符号表的属性
        hide_global();
        symbol_ptr[Class] = Loc;
        symbol_ptr[Type] = type;
        symbol_ptr[Value] = i++;
        if (token == ',') my_assert(',');
    }
    ibp = ++i;
}

int64_t type; //传递和保存当前表达式的类型

//递归下降解析表达式，将表达式的符号串转换为相应的汇编指令序列
void parse_expr(int64_t precd) {
    int64_t tmp_type, i;
    int64_t* tmp_ptr;
    // 处理常量数字
    if (token == Num) {
        tokenize();
        *++code = IMM;
        *++code = token_val;
        type = INT;
    } 
    // 处理常量字符串
    else if (token == '"') {
        *++code = IMM;
        *++code = token_val; // string addr
        my_assert('"'); while (token == '"') my_assert('"'); // 处理多个字符串字面连接的情况"hello""world"
        data = (char*)((int64_t)data + 8 & -8); 
        type = PTR;
    }
    // 处理 sizeof 表达式，目前只支持 char int64_t
    else if (token == Sizeof) {
        tokenize(); my_assert('(');
        type = parse_base_type();
        while (token == Mul) {my_assert(Mul); type = type + PTR;}
        my_assert(')');
        *++code = IMM;
        *++code = (type == CHAR) ? 1 : 8; 
        type = INT;
    }
    // 处理标识符: 变量 or 函数
    else if (token == Id) {
        tokenize();
        tmp_ptr = symbol_ptr;
        /*函数调用*/
       //如果下一个 token 是左括号，则可能是函数调用
        if (token == '(') {
            my_assert('(');
            i = 0; // 参数个数
            
            //循环解析函数调用的参数表达式，直到遇到右括号
            while (token != ')') { 
                //递归调用 parse_expr 解析参数中的表达式  add(add(1,2),2)，处理完之后 值就在 ax 中
                parse_expr(Assign);
               //将参数值推送到堆栈  
                *++code = PUSH;
                i++;
                if (token == ',') my_assert(',');
            }
            my_assert(')');//循环结束后，我们就把函数的参数 push 进了 stack 中
            // 如果是系统调用（Sys），则生成相应的系统调用指令。
            if (tmp_ptr[Class] == Sys) 
                //符号表中直接找到系统调用的地址
                *++code = tmp_ptr[Value];
            // 如果是普通函数调用，生成函数调用指令。
            else if (tmp_ptr[Class] == Fun) {
                *++code = CALL; 
                *++code = tmp_ptr[Value];
            }
            else {printf("line %lld: invalid function call\n", line); exit(-1);}
            // 如果有参数，生成删除参数的指令。
            if (i > 0) {
                *++code = DARG;
                *++code = i;
            }
            type = tmp_ptr[Type];
        }
        /*变量*/
        // 处理枚举值
        else if (tmp_ptr[Class] == Num) {
            *++code = IMM; 
            *++code = tmp_ptr[Value];
            type = INT;
        }
        // 处理变量
        else {
            // 局部变量，计算与bp的相对地址
            if (tmp_ptr[Class] == Loc) {
                *++code = LEA; 
                *++code = ibp - tmp_ptr[Value];
            }
            // 全局 var
            else if (tmp_ptr[Class] == Glo) {
                *++code = IMM; 
                *++code = tmp_ptr[Value];
            }
            // 既不是局部变量也不是全局变量则报错
            else {
                printf("line %lld: invalid variable\n", line); 
                exit(-1);
            }
            type = tmp_ptr[Type]; 
            *++code = (type == CHAR) ? LC : LI;
        }
    }
    //  强制类型转换、括号运算符
    else if (token == '(') {
        my_assert('(');
        //检查括号内的内容是否为强制类型转换。
        if (token == Char || token == Int) {
            tokenize();
            //计算转换后的类型，将 Char 类型映射为 CHAR，int64_t 类型映射为 int64_t。
            tmp_type = token - Char + CHAR;
            while (token == Mul) {
                my_assert(Mul); 
                tmp_type = tmp_type + PTR;
            }
            my_assert(')'); 
            //递归解析括号内的表达式 Inc 作为参数，表示处理所有一元运算符。
            parse_expr(Inc); 
            type = tmp_type;
        }
        //括号内是普通的括号表达式。 
        else {
            //递归解析括号内的表达式， Assign 作为参数，表示处理所有的运算符。
            parse_expr(Assign); 
            my_assert(')');
        }
    }
    // 处理 * 
    else if (token == Mul) {
        //获得  * 后面的token
        tokenize();
        parse_expr(Inc);
        //检查表达式的类型，如果是指针类型（PTR 及以上），则将类型减去 PTR，相当于进行一次解引用
        if (type >= PTR)
            type = type - PTR;
        else {
            printf("line %lld: invalid dereference\n", line);
            exit(-1);
        }
        //生成相应的汇编指令
        *++code = (type == CHAR) ? LC : LI;
    }
    // 处理 取址操作
    else if (token == And) {
        tokenize(); 
        parse_expr(Inc);
        if (*code == LC || *code == LI) 
            code--; // rollback load by addr
        else {
            printf("line %lld: invalid reference\n", line); 
            exit(-1);
        }
        type = type + PTR;
    }
    // 处理 非
    else if (token == '!') {
        tokenize();
        parse_expr(Inc);
        *++code = PUSH; 
        *++code = IMM; 
        *++code = 0; 
        *++code = EQ;
        type = INT;
    }
    // 处理按位取反
    else if (token == '~') {
        tokenize(); 
        parse_expr(Inc);
        *++code = PUSH; 
        *++code = IMM;
        *++code = -1; 
        *++code = XOR;
        type = INT;
    }
    // 处理 正号
    else if (token == Add) {
        tokenize(); 
        parse_expr(Inc); 
        type = INT;
    }
    // 负号
    else if (token == Sub) {
        tokenize(); 
        parse_expr(Inc);
        *++code = PUSH; 
        *++code = IMM; 
        *++code = -1; 
        *++code = MUL;
        type = INT;
    }
    // 自增自减运算符
    else if (token == Inc || token == Dec) {
        i = token; 
        tokenize(); 
        parse_expr(Inc); //递归解析变量名或表达式
        //  加载字符型变量的地址
        if (*code == LC) {
            *code = PUSH; 
            *++code = LC;
        }
        //加载整型变量的值
        else if (*code == LI) {
            *code = PUSH; 
            *++code = LI;
        }
        else {
            printf("line %lld: invalid Inc or Dec\n", line); 
            exit(-1);
        }
        *++code = PUSH; // 保存变量值
        *++code = IMM; *++code = (type > PTR) ? 8 : 1; //确定值的增加量，8表示指针类型
        *++code = (i == Inc) ? ADD : SUB; // 执行自增或自减操作
        *++code = (type == CHAR) ? SC : SI; // 将计算后的值写回到变量地址或变量值中
    }
    else {
        printf("line %lld: invalid expression\n", line); 
        exit(-1);
    }
    // 使用 优先级爬山法 二元运算符或后缀运算符
    // 不断向右扫描，直到遇到优先级小于当前优先级的运算符，参数 precd 指定了当前的优先级
    // 注意结合性的影响，左结合则向右计算优先级更高的运算符，右结合则向右计算优先级相等或者更高的运算符
    while (token >= precd) {    
        tmp_type = type;    
        // 赋值运算符
        if (token == Assign) {
            tokenize();
            // LC/LI表明上一步是加载值到ax（地址存在ax中），也就是=左边是一个左值
            if (*code == LC || *code == LI) 
                *code = PUSH; //将 PUSH 操作添加到code栈中
            else {
                printf("line %lld: invalid assignment\n", line); 
                exit(-1);
            }
            //递归调用 parse_expr 处理右侧表达式
            parse_expr(Assign); 
            type = tmp_type; 
            //最后来将表达式的值存到栈顶地址的位置，实现赋值操作
            *++code = (type == CHAR) ? SC : SI;
        }
        // ? : 三目运算符 与 if 类似
        else if (token == Cond) {
            tokenize(); 
            *++code = JZ; 
            tmp_ptr = ++code;
            parse_expr(Assign); 
            my_assert(':');
            *tmp_ptr = (int64_t)(code + 3);
            *++code = JMP; 
            tmp_ptr = ++code; // save endif addr
            parse_expr(Cond);
            *tmp_ptr = (int64_t)(code + 1); // write back endif point64_t
        }
        // 逻辑运算符 simple and boring, copy from c4
        // ||
        else if (token == Lor) {
            tokenize(); *++code = JNZ; tmp_ptr = ++code;
            parse_expr(Land); *tmp_ptr = (int64_t)(code + 1); type = INT;}
        // &&
        else if (token == Land) {
            tokenize(); *++code = JZ; tmp_ptr = ++code;
            parse_expr(Or); *tmp_ptr = (int64_t)(code + 1); type = INT;}
        else if (token == Or)  {tokenize(); *++code = PUSH; parse_expr(Xor); *++code = OR;  type = INT;}
        else if (token == Xor) {tokenize(); *++code = PUSH; parse_expr(And); *++code = XOR; type = INT;}
        else if (token == And) {tokenize(); *++code = PUSH; parse_expr(Eq);  *++code = AND; type = INT;}
        else if (token == Eq)  {tokenize(); *++code = PUSH; parse_expr(Lt);  *++code = EQ;  type = INT;}
        else if (token == Ne)  {tokenize(); *++code = PUSH; parse_expr(Lt);  *++code = NE;  type = INT;}
        else if (token == Lt)  {tokenize(); *++code = PUSH; parse_expr(Shl); *++code = LT;  type = INT;}
        else if (token == Gt)  {tokenize(); *++code = PUSH; parse_expr(Shl); *++code = GT;  type = INT;}
        else if (token == Le)  {tokenize(); *++code = PUSH; parse_expr(Shl); *++code = LE;  type = INT;}
        else if (token == Ge)  {tokenize(); *++code = PUSH; parse_expr(Shl); *++code = GE;  type = INT;}
        else if (token == Shl) {tokenize(); *++code = PUSH; parse_expr(Add); *++code = SHL; type = INT;}
        else if (token == Shr) {tokenize(); *++code = PUSH; parse_expr(Add); *++code = SHR; type = INT;}
        // 算数 operators
        // +
        else if (token == Add) {
            tokenize(); 
            *++code = PUSH;
            parse_expr(Mul);
            // int64_t pointer * 8
            if (tmp_type > PTR) {
                *++code = PUSH; 
                *++code = IMM;
                *++code = 8; 
                *++code = MUL;
            }
            *++code = ADD; 
            type = tmp_type;
        }
        // - 
        else if (token == Sub) {
            tokenize();
            *++code = PUSH; 
            //解析减号右侧
            parse_expr(Mul);
            //指针偏移 （指针之间的减法）
            if (tmp_type > PTR && tmp_type == type) {
                // point64_ter - point64_ter, ret / 8
                *++code = SUB; 
                *++code = PUSH;
                *++code = IMM; 
                *++code = 8;
                *++code = DIV; 
                type = INT;
            }
            //指针和整数的减法
            else if (tmp_type > PTR) {
                *++code = PUSH;
                *++code = IMM; 
                *++code = 8;
                *++code = MUL;
                *++code = SUB; 
                type = tmp_type;
            }
            //简单的整数减法
            else *++code = SUB;
        }
        else if (token == Mul) {tokenize(); *++code = PUSH; parse_expr(Inc); *++code = MUL; type = INT;}
        else if (token == Div) {tokenize(); *++code = PUSH; parse_expr(Inc); *++code = DIV; type = INT;}
        else if (token == Mod) {tokenize(); *++code = PUSH; parse_expr(Inc); *++code = MOD; type = INT;}
        //后缀自增自减 将变量值++或者--后将原值取到ax中
        else if (token == Inc || token == Dec) {
            if (*code == LC) {
                *code = PUSH; 
                *++code = LC;
            } // save var addr
            else if (*code == LI) {
                *code = PUSH; 
                *++code = LI;
            }
            else {
                printf("%lld: invlid operator=%lld\n", line, token); 
                exit(-1);
            }
            *++code = PUSH; 
            *++code = IMM; 
            *++code = (type > PTR) ? 8 : 1;
            *++code = (token == Inc) ? ADD : SUB;
            *++code = (type == CHAR) ? SC : SI; // 先保存了++/--的结果
            *++code = PUSH; 
            *++code = IMM; 
            *++code = (type > PTR) ? 8 : 1;
            *++code = (token == Inc) ? SUB : ADD; // 再重新恢复原值参与计算
            tokenize();
        }
        // a[x] = *(a + x)
        else if (token == Brak) {
            my_assert(Brak); 
            *++code = PUSH; 
            parse_expr(Assign); my_assert(']');
            if (tmp_type > PTR) {*++code = PUSH; *++code = IMM; *++code = 8; *++code = MUL;}
            else if (tmp_type < PTR) {printf("line %lld: invalid index op\n", line); exit(-1);}
            *++code = ADD; type = tmp_type - PTR;
            *++code = (type == CHAR) ? LC : LI;
        }
        else {printf("%lld: invlid token=%lld\n", line, token); exit(-1);}
    }
}

/*
解析语句：
statement = if_statement
        | while_statement
        | "{", {statement}, "}"
        | return, [expression], ";"
        | [expression], ";";
if_statement = if, "(", expression, ")", statement, [else, statement];
while_statement = while, "(", expression, ")", statement;

代码生成：
if-else语句：
if (condition)
{
    true_statements;
}

if (condition)
{
    true_statements;
}
else
{
    false_statements;
}

[condition]
JZ [end]
[true_statements]
...                 <-----[end]

[condition]
JZ [a]
[true_statements]
JMP [end]
[false_statements]   <-----[a]
...                  <-----[end]

while语句：
while (condition)
{
    while_statements;
}

[condition]         <-----[a]
JZ [end]
[while_statements]
JMP [a]
...                 <-----[end]
*/

//解析语句
void parse_stmt() {
    int64_t* a;
    int64_t* b;
    if (token == If) {
        my_assert(If); 
        my_assert('('); 
        //解析括号中的表达式，表达式的值在 ax 中
        parse_expr(Assign); 
        my_assert(')');
        *++code = JZ; 
        b = ++code; // 预留一个位置，用于跳转到false 
        parse_stmt(); // 解析 true 段的语句
        if (token == Else) {
            my_assert(Else);
            *b = (int64_t)(code + 3); //  在之前保留的位置（跳转到 false 分支的位置）写入当前指令的地址确定JZ的位置
            *++code = JMP; 
            b = ++code; //再次预留一个位置，用于跳过 false段
            parse_stmt(); // 解析 false 段的语句
        }
        *b = (int64_t)(code + 1); // write back endif point64_t
    }
    else if (token == While) {
        my_assert(While);
        //保存 循环的起始点位
        a = code + 1; 
        my_assert('('); 
        parse_expr(Assign); 
        my_assert(')');
        *++code = JZ; 
        b = ++code; // 预留位置，用于跳出循环
        parse_stmt();
        *++code = JMP; 
        *++code = (int64_t)a; // JMP to 循环起始位置
        *b = (int64_t)(code + 1); // 将循环结束位置写入 b 
    }
    else if (token == Return) {
        my_assert(Return);
        if (token != ';') 
        parse_expr(Assign);
        my_assert(';');
        *++code = RET;
    }
    else if (token == '{') {
        my_assert('{');
        while (token != '}') 
            parse_stmt(Assign);
        my_assert('}');
    }
    else if (token == ';') 
        my_assert(';');
    else {
        //如果是表达式，则进行表达式解析
        parse_expr(Assign); 
        my_assert(';');
    }
}


void parse_fun() {
    int64_t type, i;
    i = ibp; // bp handle by NVAR itself.
    // local variables must be declare in advance 
    while (token == Char || token == Int) {
        type = parse_base_type();
        while (token != ';') {
            // parse point64_ter's star
            while (token == Mul) {my_assert(Mul); type = type + PTR;}
            check_local_id(); my_assert(Id);
            hide_global();
            symbol_ptr[Class] = Loc;
            symbol_ptr[Type] = type;
            symbol_ptr[Value] = ++i;
            if (token == ',') my_assert(',');
        }
        my_assert(';');
    }
    // new stack frame for vars
    *++code = NVAR;
    // stack frame size
    *++code = i - ibp;
    while (token != '}') parse_stmt();
    if (*code != RET) *++code = RET; // void function
    // recover global variables
    symbol_ptr = symbol_table;
    while (symbol_ptr[Token]) {
        if (symbol_ptr[Class] == Loc) recover_global();
        symbol_ptr = symbol_ptr + SymSize;
    }
}

/*
    解析源代码生成虚拟机指令，code区存放指令，data区存放各种字面量、全局变量
    处理变量与函数，变量直接处理；函数调用 parse_param 与 parse_fun 分别处理参数与函数体
    在parse_fun 中再调用 parse_stmt 与 parse_expr 处理语句与表达式
*/
void parse() {
    int64_t type, base_type;
    int64_t* p;
    line = 1; token = 1; // just for loop condition
    while (token > 0) {
        tokenize(); // start or skip last ; | }
        // parse enum
        if (token == Enum) {
            my_assert(Enum);
            if (token != '{') my_assert(Id); // skip enum name
            my_assert('{'); parse_enum(); my_assert('}');
        } else if (token == Int || token == Char) {
            base_type = parse_base_type();
            // parse var or func definition
            while (token != ';' && token != '}') {
                // parse point64_ter's star
                type = base_type;
                while (token == Mul) {my_assert(Mul); type = type + PTR;}
                check_new_id();
                my_assert(Id);
                symbol_ptr[Type] = type;
                if (token == '(') {
                    // function
                    symbol_ptr[Class] = Fun;
                    symbol_ptr[Value] = (int64_t)(code + 1);
                    my_assert('('); parse_param(); my_assert(')'); my_assert('{');
                    parse_fun();
                } else {
                    // variable
                    symbol_ptr[Class] = Glo;
                    symbol_ptr[Value] = (int64_t)data;
                    data = data + 8; // keep 64 bits for each var
                }
                // handle int64_t a,b,c;
                if (token == ',') my_assert(',');
            }
        }  
    }
}

// 准备用于符号表的关键字
void keyword() {
    int64_t i;
    src = "char int enum if else return sizeof while "
        "open read close printf malloc free memset memcmp exit void main";
    // 预先将关键字添加到符号表中
    i = Char; 
    while (i <= While) {
        tokenize(); 
        symbol_ptr[Token] = i++;
    }
    // 将系统调用添加到符号表中
    i = OPEN; 
    while (i <= EXIT) {
        tokenize();
        symbol_ptr[Class] = Sys;
        symbol_ptr[Type] = INT;
        symbol_ptr[Value] = i++;
    }
    tokenize(); 
    symbol_ptr[Token] = Char; // handle void type
    tokenize(); 
    //main 在code区的地址
    main_ptr = symbol_ptr; 
    src = src_dump;
}


int64_t init_vm() {
    // allocate memory for virtual machine
    if (!(code = code_dump = malloc(MAX_SIZE))) {
        printf("could not malloc(%lld) for code segment\n", MAX_SIZE);
        return -1;
    }
    if (!(data = malloc(MAX_SIZE))) {
        printf("could not malloc(%lld) for data segment\n", MAX_SIZE);
        return -1;
    }
    if (!(stack = malloc(MAX_SIZE))) {
        printf("could not malloc(%lld) for stack segment\n", MAX_SIZE);
        return -1;
    }
    if (!(symbol_table = malloc(MAX_SIZE / 16))) {
        printf("could not malloc(%lld) for symbol_table\n", MAX_SIZE / 16);
        return -1;
    }
    memset(code, 0, MAX_SIZE);
    memset(data, 0, MAX_SIZE);
    memset(stack, 0, MAX_SIZE);
    memset(symbol_table, 0, MAX_SIZE / 16);
    return 0;
}



// 调用run_vm函数，执行code区的所有指令
char* run_vm(int64_t argc, char** argv) {
    // 清空 output_buffer
    memset(output_buffer, 0, MAX_OUTPUT_SIZE);
    int offset = 0;
    int64_t op;
    int64_t* tmp;
    // 程序出口 
    bp = sp = (int64_t*)((int64_t)stack + MAX_SIZE);
    *--sp = EXIT;
    *--sp = PUSH; 
    tmp = sp;
    *--sp = argc; 
    *--sp = (int64_t)argv;
    *--sp = (int64_t)tmp;
    //程序主函数的地址
    if (!(pc = (int64_t*)main_ptr[Value])) {
        printf("main function is not defined\n");
        sprintf(output_buffer, "main function is not defined\n");

        return output_buffer;
    }
    cycle = 0;
    while (1) {
        cycle++; 
        op = *pc++; // 读取指令
        // load & save
        if (op == IMM)          ax = *pc++;                     // load immediate(or global addr)
        else if (op == LEA)     ax = (int64_t)(bp + *pc++);         // load local addr
        else if (op == LC)      ax = *(char*)ax;                // load char
        else if (op == LI)      ax = *(int64_t*)ax;                 // load int64_t
        else if (op == SC)      *(char*)*sp++ = ax;             // save char to stack
        else if (op == SI)      *(int64_t*)*sp++ = ax;              // save int64_t to stack
        else if (op == PUSH)    *--sp = ax;                     // push ax to stack
        // jump
        else if (op == JMP)     pc = (int64_t*)*pc;                 // jump
        else if (op == JZ)      pc = ax ? pc + 1 : (int64_t*)*pc;   // jump if ax == 0
        else if (op == JNZ)     pc = ax ? (int64_t*)*pc : pc + 1;   // jump if ax != 0
        // arithmetic
        else if (op == OR)      ax = *sp++ |  ax;
        else if (op == XOR)     ax = *sp++ ^  ax;
        else if (op == AND)     ax = *sp++ &  ax;
        else if (op == EQ)      ax = *sp++ == ax;
        else if (op == NE)      ax = *sp++ != ax;
        else if (op == LT)      ax = *sp++ <  ax;
        else if (op == LE)      ax = *sp++ <= ax;
        else if (op == GT)      ax = *sp++ >  ax;
        else if (op == GE)      ax = *sp++ >= ax;
        else if (op == SHL)     ax = *sp++ << ax;
        else if (op == SHR)     ax = *sp++ >> ax;
        else if (op == ADD)     ax = *sp++ +  ax;
        else if (op == SUB)     ax = *sp++ -  ax;
        else if (op == MUL)     ax = *sp++ *  ax;
        else if (op == DIV)     ax = *sp++ /  ax;
        else if (op == MOD)     ax = *sp++ %  ax;
        // some complicate instructions for function call
        // call function: push pc + 1 to stack & pc jump to func addr(pc point64_t to)
        else if (op == CALL)    {*--sp = (int64_t)(pc+1); pc = (int64_t*)*pc;}
        // new stack frame for vars: save bp, bp -> caller stack, stack add frame
        else if (op == NVAR)    {*--sp = (int64_t)bp; bp = sp; sp = sp - *pc++;}
        // delete stack frame for args: same as x86 : add esp, <size>
        else if (op == DARG)    sp = sp + *pc++;
        // return caller: retore stack, retore old bp, pc point64_t to caller code addr(store by CALL) 
        else if (op == RET)     {sp = bp; bp = (int64_t*)*sp++; pc = (int64_t*)*sp++;}        
        // end for call function.
        // native call
        else if (op == OPEN)    {ax = open((char*)sp[1], sp[0]);}
        else if (op == CLOS)    {ax = close(*sp);}
        else if (op == READ)    {ax = read(sp[2], (char*)sp[1], *sp);}
        else if (op == PRTF)    {
            tmp = sp + pc[1] - 1; 
            ax = printf((char*)tmp[0], tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5]);
            if(offset)
                offset += sprintf(output_buffer + offset, (char*)tmp[0], tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5]);
            else
                offset = sprintf(output_buffer, (char*)tmp[0], tmp[-1], tmp[-2], tmp[-3], tmp[-4], tmp[-5]);
        }
        else if (op == MALC)    {ax = (int64_t)malloc(*sp);}
        else if (op == FREE)    {free((void*)*sp);}
        else if (op == MSET)    {ax = (int64_t)memset((char*)sp[2], sp[1], *sp);}
        else if (op == MCMP)    {ax = memcmp((char*)sp[2], (char*)sp[1], *sp);}
        else if (op == EXIT)    {
            printf("exit(%lld)\n", *sp);
            if(offset)
                offset += sprintf(output_buffer + offset, "exit(%lld)\n",*sp);
            else
                sprintf(output_buffer, "exit(%lld)\n", *sp);
            return output_buffer;
        }
        else 
        {
            
            printf("unkown instruction: %lld, cycle: %lld\n", op, cycle); 
            if(offset)
                offset += sprintf(output_buffer + offset, "unkown instruction: %lld, cycle: %lld\n", op, cycle);
            else
                sprintf(output_buffer, "unkown instruction: %lld, cycle: %lld\n", op, cycle);

            return output_buffer;
        }
    }
    return output_buffer ;
}


int64_t load_srcCode(const char* code) {
    int64_t cnt = strlen(code);

    if (!(src = src_dump = malloc(MAX_SIZE))) {
        printf("could not malloc(%lld) for source code\n", MAX_SIZE);
        return -1;
    }

    strncpy(src, code, MAX_SIZE - 1);
    src[cnt] = 0; // Null-terminate the string
    return 0;
}

char* Jatcc(const char* code) {
    // 设置虚拟机内存的最大大小为1MB，其中1MB由128k个64位组成。
    MAX_SIZE = 128 * 1024 * 8; // 1MB = 128k * 64bit

    // 加载源代码
    if (load_srcCode(code) != 0) return -1;

    // 初始化虚拟机的内存和寄存器
    if (init_vm() != 0) return -1;

    keyword();

    parse();

    // 调用write_as函数，打印汇编代码，即虚拟机指令，用于调试。
    //write_as();

    return run_vm(0, NULL);
}

// int64_t main(){
//     const char* user_code = "#include <stdio.h>\n\nint64_t main () {\n    printf(\"Hello World\\n\");\n    return 0;\n}";
//     return Jatcc(user_code);
// }