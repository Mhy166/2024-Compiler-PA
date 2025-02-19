#include "semant.h"
#include "../include/SysY_tree.h"
#include "../include/ir.h"
#include "../include/type.h"
/*
    语义分析阶段需要对语法树节点上的类型和常量等信息进行标注, 即NodeAttribute类
    同时还需要标注每个变量的作用域信息，即部分语法树节点中的scope变量
    你可以在utils/ast_out.cc的输出函数中找到你需要关注哪些语法树节点中的NodeAttribute类及其他变量
    以及对语义错误的代码输出报错信息
*/

/*
    错误检查的基本要求:
    • 检查 main 函数是否存在 (根据SysY定义，如果不存在main函数应当报错)；
    • 检查未声明变量，及在同一作用域下重复声明的变量；
    • 条件判断和运算表达式：int 和 bool 隐式类型转换（例如int a=5，return a+!a）；
    • 数值运算表达式：运算数类型是否正确 (如返回值为 void 的函数调用结果是否参与了其他表达式的计算)；
    • 检查未声明函数，及函数形参是否与实参类型及数目匹配；
    • 检查是否存在整型变量除以整型常量0的情况 (如对于表达式a/(5-4-1)，编译器应当给出警告或者直接报错)；

    错误检查的进阶要求:
    • 对数组维度进行相应的类型检查 (例如维度是否有浮点数，定义维度时是否为常量等)；
    • 对float进行隐式类型转换以及其他float相关的检查 (例如模运算中是否有浮点类型变量等)；
*/
extern LLVMIR llvmIR;
SemantTable semant_table;
std::vector<std::string> error_msgs{};

//自行添加
//语法树节点我们添加了变量var，在语义分析的过程中，涉及到构建变量的时候，最后同步到节点上，方便中间代码生成时左值的使用。


bool main_flag=false;
Type::ty type_table_bin[5][5]={
    {Type::VOID,Type::VOID ,Type::VOID ,Type::VOID,Type::VOID},
    {Type::VOID,Type::INT  ,Type::FLOAT,Type::INT,Type::VOID},
    {Type::VOID,Type::FLOAT,Type::FLOAT,Type::FLOAT,Type::VOID},
    {Type::VOID,Type::INT  ,Type::FLOAT,Type::INT,Type::VOID},
    {Type::VOID,Type::VOID ,Type::VOID ,Type::VOID,Type::VOID}
};
Type::ty type_table_rel[5][5]={
    {Type::VOID,Type::VOID ,Type::VOID ,Type::VOID,Type::VOID},
    {Type::VOID,Type::BOOL ,Type::BOOL ,Type::BOOL,Type::VOID},
    {Type::VOID,Type::BOOL ,Type::BOOL ,Type::BOOL,Type::VOID},
    {Type::VOID,Type::BOOL ,Type::BOOL ,Type::BOOL,Type::VOID},
    {Type::VOID,Type::VOID ,Type::VOID ,Type::VOID,Type::VOID}
};
Type::ty type_table_sin[2][5]={
    {Type::VOID,Type::INT  ,Type::FLOAT,Type::INT,Type::VOID},//+-
    {Type::VOID,Type::BOOL ,Type::BOOL,Type::BOOL,Type::VOID},//!
};
bool func_rf_match[5][5]={//函数参数支持隐式类型转换
    {0,0,0,0,0},
    {0,1,1,1,0},
    {0,1,1,1,0},
    {0,0,0,0,0},
    {0,0,0,0,1}
};
bool lval_rf_match[5][5]={//左值赋值支持隐式类型转换
    {0,0,0,0,0},
    {0,1,1,1,0},
    {0,1,1,1,0},
    {0,0,0,0,0},
    {0,0,0,0,1}
};
bool ToBool(NodeAttribute a){
    if(a.T.type==Type::INT){
        return a.V.val.IntVal;
    }else if(a.T.type==Type::FLOAT){
        return a.V.val.FloatVal;
    }else if(a.T.type==Type::BOOL){
        return a.V.val.BoolVal;
    }else{
        return false;
    }
}
int ToInt(NodeAttribute a){
    if(a.T.type==Type::INT){
        return a.V.val.IntVal;
    }else if(a.T.type==Type::FLOAT){
        return a.V.val.FloatVal;
    }else if(a.T.type==Type::BOOL){
        return a.V.val.BoolVal;
    }else{
        return 0;
    }
}
float ToFloat(NodeAttribute a){
    if(a.T.type==Type::INT){
        return a.V.val.IntVal;
    }else if(a.T.type==Type::FLOAT){
        return a.V.val.FloatVal;
    }else if(a.T.type==Type::BOOL){
        return a.V.val.BoolVal;
    }else{
        return 0.0;
    }
}
int GetArrayIndex(std::vector<int> dims,std::vector<int> use_dims){
    int idx=0;
    int w=1;
    for(int i=use_dims.size()-1;i>=0;i--){
        idx+=use_dims[i]*w;
        w*=dims[i];
    }
    return idx;
}

std::vector<int> InitInt(InitVal init,std::vector<int> dims){
    //进入一个初始化器
    int idx=0;
    std::vector<int> res;
    int res_size=1;
    for(auto i:dims){
        res_size*=i;
    }
    res.resize(res_size,0);
    std::vector<InitVal>* initvals;
    if(init->IsConst()){
        initvals=((ConstInitVal*)init)->initval;
    }else{
        initvals=((VarInitVal*)init)->initval;
    }
    for(auto i:*initvals){
        if(i->Exp()){
            NodeAttribute  node=i->GetNodeAttribute();
            res[idx]=ToInt(node);
            idx++;
        }else{
            //碰到新的初始化器
            int dim_scope=1;//下一个初始化器负责的维数
            int num_scope=res_size/dims[0];//下一个初始化器负责的数量
            std::vector<int> new_dims;
            while(idx%num_scope!=0){
                num_scope/=dims[dim_scope];
                dim_scope++;
            }
            for(int t=dim_scope;t<dims.size();t++){
                new_dims.push_back(dims[t]);
            }
            std::vector<int> i_vec=InitInt(i,new_dims);
            for(int t=0;t<num_scope;t++,idx++){
                res[idx]=i_vec[t];
            }
        }
    }
    return res;
}
std::vector<float> InitFloat(InitVal init,std::vector<int> dims){
    //进入一个初始化器
    int idx=0;
    std::vector<float> res;
    int res_size=1;
    for(auto i:dims){
        res_size*=i;
    }
    res.resize(res_size,0);
    std::vector<InitVal>* initvals;
    if(init->IsConst()){
        initvals=((ConstInitVal*)init)->initval;
    }else{
        initvals=((VarInitVal*)init)->initval;
    }
    for(auto i:*initvals){
        if(i->Exp()){
            NodeAttribute node=i->GetNodeAttribute();
            res[idx]=ToFloat(node);
            idx++;
        }else{
            //碰到新的初始化器
            int dim_scope=0;//下一个初始化器负责的维数
            int num_scope=res_size;//下一个初始化器负责的数量
            std::vector<int> new_dims;
            while(idx%num_scope!=0){
                num_scope/=dims[dim_scope];
                dim_scope++;
            }
            for(int t=dim_scope;t<dims.size();t++){
                new_dims.push_back(dims[t]);
            }
            std::vector<float> i_vec=InitFloat(i,new_dims);
            for(int t=0;t<num_scope;t++,idx++){
                res[idx]=i_vec[t];
            }
        }
    }
    return res;
}

//自行添加
//√
void __Program::TypeCheck() {
    //进入scope
    semant_table.symbol_table.enter_scope();
    auto comp_vector = *comp_list;
    for (auto comp : comp_vector) {
        comp->TypeCheck();
    }
    //都检查过了最后看看有没有main
    if(!main_flag){
        error_msgs.push_back("The main function is absent.\n");
    }
}
//√
void Exp::TypeCheck() {
    addexp->TypeCheck();
    attribute = addexp->attribute;
}
//√
void AddExp_plus::TypeCheck() {
    addexp->TypeCheck();
    mulexp->TypeCheck();
    NodeAttribute node1=addexp->GetNodeAttribute();
    NodeAttribute node2=mulexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_bin[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::INT)
            node.V.val.IntVal=ToInt(node1)+ToInt(node2);
        if(node.T.type==Type::FLOAT)
            node.V.val.FloatVal=ToFloat(node1)+ToFloat(node2);
    }
    SetNodeAttribute(node);

}
void AddExp_sub::TypeCheck() {
    addexp->TypeCheck();
    mulexp->TypeCheck();
    NodeAttribute node1=addexp->GetNodeAttribute();
    NodeAttribute node2=mulexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_bin[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::INT)
            node.V.val.IntVal=ToInt(node1)-ToInt(node2);
        if(node.T.type==Type::FLOAT)
            node.V.val.FloatVal=ToFloat(node1)-ToFloat(node2);
    }
    SetNodeAttribute(node);
    
}
void MulExp_mul::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();
    NodeAttribute node1=mulexp->GetNodeAttribute();
    NodeAttribute node2=unary_exp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_bin[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::INT)
            node.V.val.IntVal=ToInt(node1)*ToInt(node2);
        if(node.T.type==Type::FLOAT)
            node.V.val.FloatVal=ToFloat(node1)*ToFloat(node2);
    }
    SetNodeAttribute(node);
}
void MulExp_div::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();
    NodeAttribute node1=mulexp->GetNodeAttribute();
    NodeAttribute node2=unary_exp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_bin[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    bool flag=false;
    if(node2.V.ConstTag){
        if(ToInt(node2)==0&&node.T.type==Type::INT){//除以0
            error_msgs.push_back("The expression divisor is 0 in line " + std::to_string(line_number) + "\n");
            flag=true;
        }
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::INT&&!flag){
            node.V.val.IntVal=ToInt(node1)/ToInt(node2);
        }
        if(node.T.type==Type::FLOAT)
            node.V.val.FloatVal=ToFloat(node1)/ToFloat(node2);
    }
    SetNodeAttribute(node);
}
void MulExp_mod::TypeCheck() {
    mulexp->TypeCheck();
    unary_exp->TypeCheck();
    NodeAttribute node1=mulexp->GetNodeAttribute();
    NodeAttribute node2=unary_exp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_bin[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::INT)
            node.V.val.IntVal=ToInt(node1)%ToInt(node2);
        if(node.T.type==Type::FLOAT)
        {
            error_msgs.push_back("mod on float-point in line " + std::to_string(line_number) + "\n");
        }
    }
    SetNodeAttribute(node);
}
//√-Bool
void RelExp_leq::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    NodeAttribute node1=relexp->GetNodeAttribute();
    NodeAttribute node2=addexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_rel[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::BOOL)
            node.V.val.BoolVal=ToFloat(node1)<=ToFloat(node2);
    }
    SetNodeAttribute(node);
}
void RelExp_lt::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    NodeAttribute node1=relexp->GetNodeAttribute();
    NodeAttribute node2=addexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_rel[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::BOOL)
            node.V.val.BoolVal=ToFloat(node1)<ToFloat(node2);
    }
    SetNodeAttribute(node);
}
void RelExp_geq::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    NodeAttribute node1=relexp->GetNodeAttribute();
    NodeAttribute node2=addexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_rel[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::BOOL)
            node.V.val.BoolVal=ToFloat(node1)>=ToFloat(node2);
    }
    SetNodeAttribute(node);
}
void RelExp_gt::TypeCheck() {
    relexp->TypeCheck();
    addexp->TypeCheck();
    NodeAttribute node1=relexp->GetNodeAttribute();
    NodeAttribute node2=addexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_rel[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::BOOL)
            node.V.val.BoolVal=ToFloat(node1)>ToFloat(node2);
    }
    SetNodeAttribute(node);
}
void EqExp_eq::TypeCheck() {
    eqexp->TypeCheck();
    relexp->TypeCheck();
    NodeAttribute node1=relexp->GetNodeAttribute();
    NodeAttribute node2=eqexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_rel[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::BOOL)
            node.V.val.BoolVal=ToFloat(node1)==ToFloat(node2);
    }
    SetNodeAttribute(node);
}
void EqExp_neq::TypeCheck() {
    eqexp->TypeCheck();
    relexp->TypeCheck();
    NodeAttribute node1=relexp->GetNodeAttribute();
    NodeAttribute node2=eqexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_rel[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::BOOL)
            node.V.val.BoolVal=ToFloat(node1)!=ToFloat(node2);
    }
    SetNodeAttribute(node);
}
void LAndExp_and::TypeCheck() {
    landexp->TypeCheck();
    eqexp->TypeCheck();
    NodeAttribute node1=landexp->GetNodeAttribute();
    NodeAttribute node2=eqexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_rel[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::BOOL)
            node.V.val.BoolVal=ToBool(node1)&&ToBool(node2);
    }
    SetNodeAttribute(node);
}
void LOrExp_or::TypeCheck() {
    lorexp->TypeCheck();
    landexp->TypeCheck();
    NodeAttribute node1=landexp->GetNodeAttribute();
    NodeAttribute node2=lorexp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag&&node2.V.ConstTag;
    node.T.type=type_table_rel[node1.T.type][node2.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::BOOL)
            node.V.val.BoolVal=ToBool(node1)||ToBool(node2);
    }
    SetNodeAttribute(node);
}
//√
void ConstExp::TypeCheck() {
    addexp->TypeCheck();
    attribute = addexp->attribute;
    if (!attribute.V.ConstTag) {    // addexp is not const
        error_msgs.push_back("Expression is not const " + std::to_string(line_number) + "\n");
    }
}
//√--左值--表达式来源之一
void Lval::TypeCheck() {
    //填scope，附带检查未定义
    VarAttribute var;
    if(semant_table.symbol_table.lookup_scope(name)!=-1){
        scope=semant_table.symbol_table.lookup_scope(name);
        var=semant_table.symbol_table.lookup_val(name);
    }else if(semant_table.GlobalTable.find(name)!=semant_table.GlobalTable.end()){
        scope=0;
        var=semant_table.GlobalTable[name];
    }else{//都找不到,未定义！
        error_msgs.push_back("Variable "+name->get_string()+" undefined in line " + std::to_string(line_number) + "\n");
        return;
    }
    std::vector<Expression> use_dims;//使用的维度
    std::vector<int> usedims;
    if(dims!=nullptr){  
        use_dims=*dims;
    }
    bool const_array_flag=true;
    for(auto d:use_dims){
        d->TypeCheck();
        usedims.push_back(d->GetNodeAttribute().V.val.IntVal);
        if(!d->GetNodeAttribute().V.ConstTag){
            const_array_flag=false;
        }
    }
    NodeAttribute node;
    if(var.dims.size()==use_dims.size()){//该左值不是数组
        node.T.type=var.type;
        node.V.ConstTag=const_array_flag&&var.ConstTag;
        if(node.V.ConstTag){
            if(var.dims.size()!=0){
                if(node.T.type==Type::INT){
                    node.V.val.IntVal=var.IntInitVals[GetArrayIndex(var.dims,usedims)];
                }else if(node.T.type==Type::FLOAT){
                    node.V.val.FloatVal=var.FloatInitVals[GetArrayIndex(var.dims,usedims)];
                }
            }else{
                if(node.T.type==Type::INT){
                    node.V.val.IntVal=var.IntInitVals[0];
                }else if(node.T.type==Type::FLOAT){
                    node.V.val.FloatVal=var.FloatInitVals[0];
                }
            }
        }
    }else if(var.dims.size()>use_dims.size()){//维度没有全用，左值是数组
        node.T.type=Type::PTR;
        node.V.ConstTag=false;
    }
    this->var=var;
    SetNodeAttribute(node);
}
//√--函数调用--表达式来源之二
void FuncRParams::TypeCheck() {}
void Func_call::TypeCheck() {
    if(semant_table.FunctionTable.find(name)==semant_table.FunctionTable.end()){
        //未声明
        error_msgs.push_back("Function "+name->get_string()+" undefined in line " + std::to_string(line_number) + "\n");
        return;
    }
    std::vector<Expression> rparams;
    if(funcr_params!=nullptr){  
        rparams=*((FuncRParams*)funcr_params)->params;
    }
    for(auto params:rparams){
        params->TypeCheck();
    }

    FuncDef funcdef=semant_table.FunctionTable[name];
    std::vector<FuncFParam> fparams=*(funcdef->formals);//语法产生式的定义里肯定有
    if(fparams.size()!=rparams.size()){
        error_msgs.push_back("Function "+name->get_string()+" parameter nums do not match in line " + std::to_string(line_number) + "\n");
        return;
    }
    for(int i=0;i<fparams.size();i++){
        if(rparams[i]->GetNodeAttribute().T.type!=Type::PTR){
            if(!func_rf_match[fparams[i]->attribute.T.type][rparams[i]->GetNodeAttribute().T.type]){
                error_msgs.push_back("Function "+name->get_string()+" parameter types do not match in line " + std::to_string(line_number) + "\n");
                return;
            }
        }
    }
    attribute.T.type = funcdef->return_type;
    attribute.V.ConstTag=false;
}
//√
void UnaryExp_plus::TypeCheck() {
    unary_exp->TypeCheck();
    NodeAttribute node1=unary_exp->GetNodeAttribute();
    Type::ty t=node1.T.type;
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag;
    node.T.type=type_table_sin[0][t];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::INT)
            node.V.val.IntVal=ToInt(node1);
        if(node.T.type==Type::FLOAT)
            node.V.val.FloatVal=ToFloat(node1);
    }
    SetNodeAttribute(node);
}
void UnaryExp_neg::TypeCheck() {
    unary_exp->TypeCheck();
    NodeAttribute node1=unary_exp->GetNodeAttribute();
    Type::ty t=node1.T.type;
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag;
    node.T.type=type_table_sin[0][t];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::INT)
            node.V.val.IntVal=-ToInt(node1);
        if(node.T.type==Type::FLOAT)
            node.V.val.FloatVal=-ToFloat(node1);
    }
    SetNodeAttribute(node);
}
void UnaryExp_not::TypeCheck() {
    unary_exp->TypeCheck();
    NodeAttribute node1=unary_exp->GetNodeAttribute();
    NodeAttribute node;
    node.V.ConstTag=node1.V.ConstTag;
    node.T.type=type_table_sin[1][node1.T.type];
    if(node.T.type==Type::VOID){
        error_msgs.push_back("The operand is of type void  in line " +
                                 std::to_string(line_number) + "\n");
    }
    if(node.V.ConstTag){
        if(node.T.type==Type::BOOL)
            node.V.val.BoolVal=!ToBool(node1);
    }
    SetNodeAttribute(node);
}
//√--常数--表达式来源之三
void IntConst::TypeCheck() {
    attribute.T.type = Type::INT;
    attribute.V.ConstTag = true;
    attribute.V.val.IntVal = val;
}
void FloatConst::TypeCheck() {
    attribute.T.type = Type::FLOAT;
    attribute.V.ConstTag = true;
    attribute.V.val.FloatVal = val;
}
void StringConst::TypeCheck() {}
//√
void PrimaryExp_branch::TypeCheck() {
    exp->TypeCheck();
    attribute = exp->attribute;
}

//以上的所有节点实质上，都已经传播好NodeAttribute了，形成一个表达式大圈子，下面的节点，节点属性不一定都传好。

//√--赋值表达式实质上是不存在一个运算类型的
void assign_stmt::TypeCheck() {
    lval->TypeCheck();
    exp->TypeCheck();
    ((Lval*)lval)->left_flag=true;//括号不能少，否则默认类型Expression
    if(!lval_rf_match[lval->GetNodeAttribute().T.type][exp->GetNodeAttribute().T.type]){
        error_msgs.push_back("The assignment statement is invalid in line " + std::to_string(line_number) +"\n");
    }
}
//√
void expr_stmt::TypeCheck() {
    exp->TypeCheck();
    attribute = exp->attribute;
}
//√
void block_stmt::TypeCheck() { b->TypeCheck(); }
//√
void ifelse_stmt::TypeCheck() {
    Cond->TypeCheck();
    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("if cond type is invalid in line" + std::to_string(line_number) + "\n");
    }
    ifstmt->TypeCheck();
    elsestmt->TypeCheck();
}
//√
void if_stmt::TypeCheck() {
    Cond->TypeCheck();
    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("if cond type is invalid in line " + std::to_string(line_number) + "\n");
    }
    ifstmt->TypeCheck();
}
//√
void while_stmt::TypeCheck() {
    Cond->TypeCheck();
    if (Cond->attribute.T.type == Type::VOID) {
        error_msgs.push_back("if cond type is invalid in line " + std::to_string(line_number) + "\n");
    }
    body->TypeCheck();
}
//√
void continue_stmt::TypeCheck() {}
void break_stmt::TypeCheck() {}
void return_stmt::TypeCheck() { 
    return_exp->TypeCheck();
    Type::ty t=return_exp->GetNodeAttribute().T.type;
    if(t==Type::PTR||t==Type::VOID){
        error_msgs.push_back("The return expr is invalid in line " +
                                 std::to_string(line_number) + "\n");
    }
}
void return_stmt_void::TypeCheck() {}
//√--下面的初值实质上要接入表达式大圈子里的，对于有exp的赋好属性
void ConstInitVal::TypeCheck() {
    for(auto init:*initval){
        init->TypeCheck();
    }
}
void ConstInitVal_exp::TypeCheck() {
    exp->TypeCheck();
    Type::ty t=exp->GetNodeAttribute().T.type;
    if(t==Type::VOID){
        error_msgs.push_back("The init expr is invalid in line " +
                                 std::to_string(line_number) + "\n");
    }
    SetNodeAttribute(exp->GetNodeAttribute());
}
void VarInitVal::TypeCheck() {
    for(auto init:*initval){
        init->TypeCheck();
    }
}
void VarInitVal_exp::TypeCheck() {
    exp->TypeCheck();
    Type::ty t=exp->GetNodeAttribute().T.type;
    if(t==Type::VOID){
        error_msgs.push_back("The init expr is invalid in line " +
                                 std::to_string(line_number) + "\n");
    }
    SetNodeAttribute(exp->GetNodeAttribute());
}

//√--定义语句本身类型也是没有的，主要是登记
void VarDef_no_init::TypeCheck() {
    if(semant_table.symbol_table.lookup_scope(GetName())==semant_table.symbol_table.get_current_scope()){
        error_msgs.push_back("Multiple definitions of local variables in line " + std::to_string(line_number) +"\n");
    }
    //构造局部表内的VarAttr
    scope=semant_table.symbol_table.get_current_scope();
    VarAttribute var;
    var.type=GetNodeAttribute().T.type;
    var.ConstTag=IsConst();
    if(GetDims()!=nullptr){
        for(auto dim:*(GetDims())){
            dim->TypeCheck();
            var.dims.push_back(dim->attribute.V.val.IntVal);
        }
    }
    this->var=var;
    semant_table.symbol_table.add_Symbol(GetName(),var);
}
void VarDef::TypeCheck() {
    if(semant_table.symbol_table.lookup_scope(GetName())==semant_table.symbol_table.get_current_scope()){
        error_msgs.push_back("Multiple definitions of local variables in line " + std::to_string(line_number) +"\n");
    }
    //构造局部表内的VarAttr
    scope=semant_table.symbol_table.get_current_scope();
    VarAttribute var;
    var.type=GetNodeAttribute().T.type;;
    var.ConstTag=IsConst();
    if(GetDims()!=nullptr){
        for(auto dim:*(GetDims())){
            dim->TypeCheck();
            var.dims.push_back(dim->attribute.V.val.IntVal);
        }
    }
    InitVal init=GetInitVal();
    init->TypeCheck();
    if(init->Exp()){//非数组,int or const
        if(var.type==Type::INT){
            var.IntInitVals.push_back(ToInt(init->GetNodeAttribute()));
        }else if(var.type==Type::FLOAT){
            var.FloatInitVals.push_back(ToFloat(init->GetNodeAttribute()));
        }
    }else{//数组，表示遇到初始化器了
        if(var.type==Type::INT){
            var.IntInitVals=InitInt(init,var.dims);
        }else if(var.type==Type::FLOAT){
            var.FloatInitVals=InitFloat(init,var.dims);
        }
    }
    this->var=var;
    semant_table.symbol_table.add_Symbol(GetName(),var);
}
void ConstDef::TypeCheck() {
    if(semant_table.symbol_table.lookup_scope(GetName())==semant_table.symbol_table.get_current_scope()){
        error_msgs.push_back("Multiple definitions of local variables in line " + std::to_string(line_number) +"\n");
    } 
    //构造局部表内的VarAttr
    scope=semant_table.symbol_table.get_current_scope();
    VarAttribute var;
    var.type=GetNodeAttribute().T.type;
    var.ConstTag=IsConst();
    if(GetDims()!=nullptr){
        for(auto dim:*(GetDims())){
            dim->TypeCheck();
            var.dims.push_back(dim->attribute.V.val.IntVal);
        }
    }
    //初值
    InitVal init=GetInitVal();
    init->TypeCheck();
    if(init->Exp()){//非数组,int or const
        if(var.type==Type::INT){
            var.IntInitVals.push_back(ToInt(init->GetNodeAttribute()));
        }else if(var.type==Type::FLOAT){
            var.FloatInitVals.push_back(ToFloat(init->GetNodeAttribute()));
        }
    }else{//数组，表示遇到初始化器了
        if(var.type==Type::INT){
            var.IntInitVals=InitInt(init,var.dims);
        }else if(var.type==Type::FLOAT){
            var.FloatInitVals=InitFloat(init,var.dims);
        }
    }
    //登记
    this->var=var;
    semant_table.symbol_table.add_Symbol(GetName(),var);
}

//√
void VarDecl::TypeCheck() {
    for(auto var_def:*(GetDef())){
        NodeAttribute node;
        node.T.type=type_decl;
        var_def->SetNodeAttribute(node);
        var_def->TypeCheck();
    }
}
//√
void ConstDecl::TypeCheck() {
    for(auto var_def:*(GetDef())){
        NodeAttribute node;
        node.T.type=type_decl;
        var_def->SetNodeAttribute(node);
        var_def->TypeCheck();
    }
}
//√--block
void BlockItem_Decl::TypeCheck() { decl->TypeCheck(); }
void BlockItem_Stmt::TypeCheck() { stmt->TypeCheck(); }
void __Block::TypeCheck() {
    semant_table.symbol_table.enter_scope();
    auto item_vector = *item_list;
    for (auto item : item_vector) {
        item->TypeCheck();
    }
    semant_table.symbol_table.exit_scope();
}

//√
void __FuncFParam::TypeCheck() {
    VarAttribute val;
    val.ConstTag = false;
    attribute.V.ConstTag=false;
    val.type = type_decl;
    scope = 1;//形参作用域就是顶层下一层

    // 如果dims为nullptr, 表示该变量不含数组下标, 如果你在语法分析中采用了其他方式处理，这里也需要更改
    if (dims != nullptr) {    
        auto dim_vector = *dims;

        // the fisrt dim of FuncFParam is empty
        // eg. int f(int A[][30][40])
        val.dims.push_back(-1);    // 这里用-1表示empty，你也可以使用其他方式
        for (int i = 1; i < dim_vector.size(); ++i) {
            auto d = dim_vector[i];
            d->TypeCheck();
            val.dims.push_back(d->attribute.V.val.IntVal);
        }
        attribute.T.type = Type::PTR;
    } else {
        attribute.T.type = type_decl;
    }

    if (name != nullptr) {
        //因为函数位于顶层，当前scope为1，全局变量又不会登记到局部符号表，因此这里直接不等于-1就好了。
        if (semant_table.symbol_table.lookup_scope(name) != -1) {
            error_msgs.push_back("multiple difinitions of formals in function " + name->get_string() + " in line " +
                                 std::to_string(line_number) + "\n");
        }
        if(attribute.T.type==Type::VOID){
            error_msgs.push_back("The func fparam is of type void  in line " +
                                    std::to_string(line_number) + "\n");
        }
        this->var=val;
        semant_table.symbol_table.add_Symbol(name, val);
    }
}
//√
void __FuncDef::TypeCheck() {
    //函数声明只能在顶层单元里出现，不要求实现重名函数情况
    semant_table.symbol_table.enter_scope();
    semant_table.FunctionTable[name] = this;
    //检查main函数
    if(name->get_string()=="main"){
        main_flag=true;
    }
    if(semant_table.GlobalTable.find(name)!=semant_table.GlobalTable.end()){
        error_msgs.push_back("The function name is repeatedly defined with the variable name in line " + std::to_string(line_number) +"\n");
    }
    auto formal_vector = *formals;
    for (auto formal : formal_vector) {
        formal->TypeCheck();
    }

    // block TypeCheck
    if (block != nullptr) {
        auto item_vector = *(block->item_list);
        for (auto item : item_vector) {
            item->TypeCheck();
        }
    }

    semant_table.symbol_table.exit_scope();
}
//√
void CompUnit_Decl::TypeCheck() {
    //顶层声明是全局声明和局部声明不一样
    
    for(auto var_def:*(decl->GetDef())){
        //Ident检查
        if(semant_table.GlobalTable.find(var_def->GetName())!=semant_table.GlobalTable.end()){
            error_msgs.push_back("Multiple definitions of global variables in line " + std::to_string(line_number) +"\n");
        }
        //SysY要求顶层的变量和函数也不能重命名
        // if(semant_table.FunctionTable.find(var_def->GetName())!=semant_table.FunctionTable.end()){
        //     error_msgs.push_back("The variable name is repeatedly defined with the function name in line " + std::to_string(line_number) +"\n");
        // }
        //构造全局表内的VarAttr
        var_def->scope=0;
        VarAttribute var;
        var.type=decl->GetType();
        var.ConstTag=decl->IsConst();
        if(var_def->GetDims()!=nullptr){
            for(auto dim:*(var_def->GetDims())){
                dim->TypeCheck();
                var.dims.push_back(dim->attribute.V.val.IntVal);
            }
        }
        //初值
        InitVal init=var_def->GetInitVal();
        if(init!=nullptr){
            init->TypeCheck();
            
                if(init->Exp()){
                    if(var.type==Type::INT){
                        var.IntInitVals.push_back(ToInt(init->GetNodeAttribute()));
                    }else if(var.type==Type::FLOAT){
                        var.FloatInitVals.push_back(ToFloat(init->GetNodeAttribute()));
                    }
                }else{
                    if(var.type==Type::INT){
                        var.IntInitVals=InitInt(init,var.dims);
                    }else if(var.type==Type::FLOAT){
                        var.FloatInitVals=InitFloat(init,var.dims);
                    }
                }
        }
        //登记
        var_def->var=var;
        semant_table.GlobalTable[var_def->GetName()]=var;
    }
}
//√
void CompUnit_FuncDef::TypeCheck() { func_def->TypeCheck(); }