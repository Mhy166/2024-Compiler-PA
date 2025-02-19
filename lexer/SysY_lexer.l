%option noyywrap 
/* 选项功能是去掉默认的yywarp函数调用 */
%{
#include <cmath>
#include <vector>
#include <cstring>
#include "../include/symtab.h"
#include "../parser/SysY_parser.tab.h"
//这是文字块
extern int line_number;
extern int col_number;
extern int cur_col_number;
extern IdTable id_table;
void yyerror(char* s, ...);
%}
/* 编写注意顺序啊 */
%%
    /* 接下来是规则：匹配尽可能长的+更早出现的 */
    /*单行注释*/
"//"([^\n]*) {}
    /*多行注释*/
"/*"(([^*]|(\*)+[^/])*)(\*)+"/" {
    for(int i = 0;yytext[i];i++){//yytext char*
        if(yytext[i]!='\n') col_number += 1;
        else{
            ++line_number;
            col_number=0;
        }
    }}

"<=" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return LEQ;} //SysY_parser.y中定义 main中的PrintLexerResult的参数       
">=" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return GEQ;}         
"==" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return EQ;}        
"!=" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return NE;}
"&&" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return AND;}         
"||" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return OR;}               
"const" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return CONST;}     
"if" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return IF;}       
"else" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return ELSE;}  
"while" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return WHILE;}
"void" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return NONE_TYPE;}    
"int" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return INT;}    
"float" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return FLOAT;}      
"return" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return RETURN;}    
"break" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return BREAK;}     
"continue" {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return CONTINUE;}

\n {++line_number;col_number = 0;}

[ \t\f\r\v] {col_number += strlen(yytext);}

[\.\+\-\*\/\=\<\!\%\>] {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return yytext[0];}

[\{\}\;\(\)\,\[\]] {
    cur_col_number=col_number;
    col_number += strlen(yytext);
    return yytext[0];}

[_a-zA-Z][_a-zA-Z0-9]* {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.symbol_token = id_table.add_id(std::string(yytext));//这样传的token（IDENT）
    return IDENT;
}

([1-9][0-9]*)|0 {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    long long int tmp=0;
    yylval.int_token = 0;//存储整型常量
    for(int i = 0;yytext[i];i++){
        tmp *= 10;
        tmp += yytext[i] - '0';
    }
    if (tmp > 2147483648) {//溢出检测
        fprintf(stderr, "Error: Integer out of range at %d row %d col \n", line_number,cur_col_number);
        exit(1);
    }
    yylval.int_token=tmp;
    return INT_CONST;
}
    /*
      在词法分析中，你只需要考虑yylval的三种类型，分别为int_token, float_token, symbol_token
      之所以要定义这三种类型，是因为我们只返回这个token的类型是不够的，我们还需要知道这个token的属性
      例如我们需要知道INT_CONST具体的值是多少
    */
    /* 八进制Int */
0[0-7]* {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.int_token = 0;
    long long int tmp=0;
    for(int i = 0;yytext[i];i++){
        tmp *= 8;
        tmp += yytext[i] - '0';
    }
    if (tmp > 2147483648) {
        fprintf(stderr, "Error: Integer out of range at %d row %d col \n", line_number,cur_col_number);
        exit(1);
    }
    yylval.int_token=tmp;
    return INT_CONST;
}
    /* 十六进制Int */
0[xX][0-9a-fA-F]+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.int_token = 0;
    long long int tmp=0;
    for(int i = 2;yytext[i];i++){
        tmp *= 16;
        if(yytext[i]>='a'&&yytext[i]<='f'){tmp += (yytext[i] - 'a' + 10);}
        else if(yytext[i]>='A'&&yytext[i]<='F'){tmp += (yytext[i] - 'A' + 10);}
        else if(yytext[i]>='0'&&yytext[i]<='9'){tmp += (yytext[i] - '0');}
    }
    if (tmp > 2147483648) {
        fprintf(stderr, "Error: Integer out of range at %d row %d col \n", line_number,cur_col_number);
        exit(1);
    }
    yylval.int_token=tmp;
    return INT_CONST;
}
    /* 十进制浮点数 整数+指数 */
[0-9]+[eE][+-]?[0-9]+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    bool exp_flag=false;//指数部分flag
    bool mul=true;//指数正负
    int exp0=0;//指数部分
    yylval.float_token=0.0;//存储浮点常量
    for(int i = 0;yytext[i];i++){
        if(yytext[i]!='e'&&yytext[i]!='E'){
            if(!exp_flag){
                yylval.float_token*=10;
                yylval.float_token += yytext[i] - '0';
            }
            else{
                exp0*=10;
                exp0 += yytext[i] - '0';
            }
        }
        else{
            exp_flag=true;
            if(yytext[i+1]=='+'||yytext[i+1]=='-'){
                i++;
                if(yytext[i]=='-'){mul=false;}          
            }
        }
    }
    if(mul){yylval.float_token*=pow(10,exp0);}
    else{yylval.float_token*=pow(10,-exp0);}
    return FLOAT_CONST;
}
    /* 十进制浮点数 整数.+指数可选 */
[0-9]+(\.)([eE][+-]?[0-9]+)? {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    bool exp_flag=false;
    bool mul=true;
    int exp0=0;
    yylval.float_token=0.0;
    for(int i = 0;yytext[i];i++){
        if(yytext[i]!='.'){
            if(!exp_flag){
                yylval.float_token*=10;
                yylval.float_token += yytext[i] - '0';
            }
            else{
                exp0*=10;
                exp0+= yytext[i] - '0';
            }
        }
        else{
            if(yytext[i+1]){
                i++;
                exp_flag=true;
                if(yytext[i+1]=='+'||yytext[i+1]=='-'){
                    i++;
                    if(yytext[i]=='-'){mul=false;}          
                }
            }
        }
    }
    if(exp_flag){
        if(mul){yylval.float_token*=pow(10,exp0);}
        else{yylval.float_token/=pow(10,exp0);}
    }
    return FLOAT_CONST;
}
    /* 十进制浮点数 整数（可选）.整数+指数可选 */
[0-9]*(\.)[0-9]+([eE][+-]?[0-9]+)? {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    int dot_cnt=-1;//记录小数点后有多少位，在10的幂次做处理就好
    bool exp_flag=false;
    bool mul=true;
    int exp0=0;
    yylval.float_token=0.0;
    for(int i = 0;yytext[i];i++){
        if(yytext[i]!='.'&&yytext[i]!='e'&&yytext[i]!='E'){
            if(!exp_flag){
                yylval.float_token*=10;
                yylval.float_token += yytext[i] - '0';
                if(dot_cnt>=0){
                    dot_cnt++;
                }
            }
            else{
                exp0*=10;
                exp0+= yytext[i] - '0';
            }
        }
        else{
            if(yytext[i]=='.')dot_cnt++;
            else {
                exp_flag=true;
                if(yytext[i+1]=='+'||yytext[i+1]=='-'){
                    i++;
                    if(yytext[i]=='-'){mul=false;}          
                }
            }
        }
    }
    
    if(mul){yylval.float_token*=pow(10,exp0-dot_cnt);}
    else{yylval.float_token*=pow(10,-exp0-dot_cnt);}
    
    return FLOAT_CONST;
}

    /*十六进制浮点数*/
0[xX][0-9a-fA-F]+(\.)?[pP][+-]?[0-9]+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    bool exp_flag=false;
    bool mul=true;
    int exp0=0;
    yylval.float_token=0.0;
    for(int i =2;yytext[i];i++){
        if(yytext[i]!='.'){
            if(yytext[i]!='p'&&yytext[i]!='P'){
                if(!exp_flag){
                    yylval.float_token*=16;
                    if(yytext[i]>='a'&&yytext[i]<='f'){yylval.float_token += (yytext[i] - 'a' + 10);}
                    else if(yytext[i]>='A'&&yytext[i]<='F'){yylval.float_token += (yytext[i] - 'A' + 10);}
                    else if(yytext[i]>='0'&&yytext[i]<='9'){yylval.float_token += (yytext[i] - '0');}
                }
                else{
                    exp0*=10;
                    exp0+= yytext[i] - '0';
                }
            }
            else{
                exp_flag=true;
                if(yytext[i+1]=='+'||yytext[i+1]=='-'){
                    i++;
                    if(yytext[i]=='-'){mul=false;}          
                }
            }
        }
    }
    if(mul){yylval.float_token*=pow(2,exp0);}
    else{yylval.float_token/=pow(2,exp0);}
    return FLOAT_CONST;
}

0[xX][0-9a-fA-F]*(\.)[0-9a-fA-F]+[pP][+-]?[0-9]+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    int dot_cnt=-1;
    bool exp_flag=false;
    bool mul=true;
    int exp0=0;
    yylval.float_token=0.0;
    for(int i = 2;yytext[i];i++){
        if(yytext[i]!='.'&&yytext[i]!='p'&&yytext[i]!='P'){
            if(!exp_flag){
                yylval.float_token*=16;
                if(yytext[i]>='a'&&yytext[i]<='f'){yylval.float_token += (yytext[i] - 'a' + 10);}
                else if(yytext[i]>='A'&&yytext[i]<='F'){yylval.float_token += (yytext[i] - 'A' + 10);}
                else if(yytext[i]>='0'&&yytext[i]<='9'){yylval.float_token += (yytext[i] - '0');}
                if(dot_cnt>=0){
                    dot_cnt++;
                }
            }
            else{
                exp0*=10;
                exp0+= yytext[i] - '0';
            }
        }
        else{
            if(yytext[i]=='.')dot_cnt++;
            else {
                exp_flag=true;
                if(yytext[i+1]=='+'||yytext[i+1]=='-'){
                    i++;
                    if(yytext[i]=='-'){mul=false;}          
                }
            }
        }
    }
    yylval.float_token/=pow(16,dot_cnt);
    if(mul){yylval.float_token*=pow(2,exp0);}
    else{yylval.float_token*=pow(2,-exp0);}
    return FLOAT_CONST;
}
    /*unknown tokens, return ERROR*/
. {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.error_msg = yytext;//存储错误描述
    return ERROR;
}
%%

