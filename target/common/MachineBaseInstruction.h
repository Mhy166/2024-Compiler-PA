#ifndef MachineBaseIns_H
#define MachineBaseIns_H
#include <assert.h>
#include <cstring>
#include <iostream>
#include <map>
#include <set>
#include <vector>
//主要定义了基本的机器指令，RISC-V汇编指令继承了该文件中定义的类

//PRINT、ERROR、TODO：这些宏用于输出调试信息、错误信息和待办事项，带有文件和行号的提示

    #ifdef ENABLE_PRINT
        #ifndef PRINT
        #define PRINT(...)                                                                                                     \
            do {                                                                                                               \
                char message[256];                                                                                             \
                sprintf(message, __VA_ARGS__);                                                                                 \
                std::cerr << message << "\n";                                                                                  \
            } while (0)
        #endif
    #else
        #ifndef PRINT
        #define PRINT(...)
        #endif
    #endif

    #ifndef ERROR
    #define ERROR(...)                                                                                                     \
        do {                                                                                                               \
            char message[256];                                                                                             \
            sprintf(message, __VA_ARGS__);                                                                                 \
            std::cerr << "\033[;31;1m";                                                                                    \
            std::cerr << "ERROR: ";                                                                                        \
            std::cerr << "\033[0;37;1m";                                                                                   \
            std::cerr << message << "\n";                                                                                  \
            std::cerr << "\033[0;33;1m";                                                                                   \
            std::cerr << "File: \033[4;37;1m" << __FILE__ << ":" << __LINE__ << "\n";                                      \
            std::cerr << "\033[0m";                                                                                        \
            assert(false);                                                                                                 \
        } while (0)
    #endif

    #ifndef TODO
    #define TODO(...)                                                                                                      \
        do {                                                                                                               \
            char message[256];                                                                                             \
            sprintf(message, __VA_ARGS__);                                                                                 \
            std::cerr << "\033[;34;1m";                                                                                    \
            std::cerr << "TODO: ";                                                                                         \
            std::cerr << "\033[0;37;1m";                                                                                   \
            std::cerr << message << "\n";                                                                                  \
            std::cerr << "\033[0;33;1m";                                                                                   \
            std::cerr << "File: \033[4;37;1m" << __FILE__ << ":" << __LINE__ << "\n";                                      \
            std::cerr << "\033[0m";                                                                                        \
            assert(false);                                                                                                 \
        } while (0)
    #endif

//Log：用于记录日志信息，包含文件名、行号、函数名等
//Lazy：一个简单的日志输出函数，用于标记某些懒加载操作。
    #define ENABLE_LOG
    #ifdef ENABLE_LOG
        #ifndef Log
        #define Log(...)                                                                                                       \
            do {                                                                                                               \
                char message[256];                                                                                             \
                sprintf(message, __VA_ARGS__);                                                                                 \
                std::cerr << "\033[;35;1m[\033[4;33;1m" << __FILE__ << ":" << __LINE__ << "\033[;35;1m "                       \
                        << __PRETTY_FUNCTION__ << "]";                                                                       \
                std::cerr << "\033[0;37;1m ";                                                                                  \
                std::cerr << message << "\n";                                                                                  \
                std::cerr << "\033[0m";                                                                                        \
            } while (0)
        #endif
    #else
        #ifndef Log
        #define Log(...)
        #endif
    #endif

    #define ENABLE_LOG
    #ifdef ENABLE_LOG
        #ifndef Lazy
        #define Lazy(str)                                                                                                      \
            do {                                                                                                               \
                Log("\033[;31;1m%s\033[0m", str);                                                                              \
            } while (0)
        #endif
    #else
        #ifndef Lazy
        #define Lazy(str)
        #endif
    #endif

//Assert：断言宏，用于在调试时检查条件是否成立，若不成立会触发错误并打印堆栈信息
    #ifndef Assert
    #define Assert(EXP)                                                                                                    \
        do {                                                                                                               \
            if (!(EXP)) {                                                                                                  \
                ERROR("Assertion failed: %s", #EXP);                                                                       \
            }                                                                                                              \
        } while (0)
    #endif

typedef unsigned __int128 Uint128;
typedef unsigned long long Uint64;
typedef unsigned int Uint32;

//机器数据类型（MachineDataType）： 该结构体用于表示机器的基本数据类型和长度：

//数据类型：INT 或 FLOAT（整数或浮点数）
//数据宽度：32位、64位、128位
//包含构造函数、赋值操作符、==运算符以及用于获取数据宽度和字符串表示的函数

struct MachineDataType {//机器数据 类型
    enum { INT, FLOAT };//类型
    enum { B32, B64, B128 };//宽度
    unsigned data_type;
    unsigned data_length;
    MachineDataType() {}//构造函数
    MachineDataType(const MachineDataType &other) {
        this->data_type = other.data_type;
        this->data_length = other.data_length;
    }
    MachineDataType operator=(const MachineDataType &other) {
        this->data_type = other.data_type;
        this->data_length = other.data_length;
        return *this;
    }
    MachineDataType(unsigned data_type, unsigned data_length) : data_type(data_type), data_length(data_length) {}
    bool operator==(const MachineDataType &other) const {
        return this->data_type == other.data_type && this->data_length == other.data_length;
    }
    int getDataWidth() {//数据宽度
        switch (data_length) {
        case B32:
            return 4;
        case B64:
            return 8;
        case B128:
            return 16;
        }
        return 0;
    }
    std::string toString() {
        std::string ret;
        if (data_type == INT)
            ret += 'i';
        if (data_type == FLOAT)
            ret += 'f';
        if (data_length == B32)
            ret += "32";
        if (data_length == B64)
            ret += "64";
        if (data_length == B128)
            ret += "128";
        return ret;
    }
};

extern MachineDataType INT32, INT64, INT128, FLOAT_32, FLOAT64, FLOAT128;

//寄存器类（Register）： 

//表示一个寄存器，包含寄存器编号、是否为虚拟寄存器、以及该寄存器的数据类型
//提供了多个构造函数、赋值操作符、优先级判断（用于寄存器的排序）和比较函数

struct Register {//寄存器类
public:
    int reg_no;         // 寄存器编号
    bool is_virtual;    // 是否为虚拟寄存器
    MachineDataType type;// 所属类型
    Register() {}
    Register(bool is_virtual, int reg_no, MachineDataType type, bool save = false)
        : is_virtual(is_virtual), reg_no(reg_no), type(type) {}
    int getDataWidth() { return type.getDataWidth(); }
    Register(const Register &other) {
        this->is_virtual = other.is_virtual;
        this->reg_no = other.reg_no;
        this->type = other.type;
    }
    Register operator=(const Register &other) {
        this->is_virtual = other.is_virtual;
        this->reg_no = other.reg_no;
        this->type = other.type;
        return *this;
    }
    bool operator<(Register other) const {//优先级判断
        if (is_virtual != other.is_virtual)
            return is_virtual < other.is_virtual;//物理寄存器<虚拟寄存器
        if (reg_no != other.reg_no)
            return reg_no < other.reg_no;//编号<
        if (type.data_type != other.type.data_type)//INT<FLOAT
            return type.data_type < other.type.data_type;
        if (type.data_length != other.type.data_length)//32<64<128
            return type.data_length < other.type.data_length;
        return false;
    }
    bool operator==(Register other) const {
        return reg_no == other.reg_no && is_virtual == other.is_virtual && type.data_type == other.type.data_type &&
               type.data_length == other.type.data_length;
    }
};

//机器操作数（MachineBaseOperand）： 

//这是一个抽象基类，用于表示机器指令中的操作数
//操作数有不同的类型，包括寄存器（REG）、整数立即数（IMMI）、浮点数立即数（IMMF）和双精度立即数（IMMD），它是其他具体操作数类的基类

//MachineRegister：表示寄存器操作数，包含一个寄存器对象。
//MachineImmediateInt、MachineImmediateFloat、MachineImmediateDouble：分别表示整数、浮点数和双精度浮点数的立即数操作数。
struct MachineBaseOperand {//机器操作数基类
    MachineDataType type;
    enum { REG, IMMI, IMMF, IMMD };
    int op_type;
    MachineBaseOperand(int op_type) : op_type(op_type) {}
    virtual std::string toString() = 0;
};

struct MachineRegister : public MachineBaseOperand {
    Register reg;
    MachineRegister(Register reg) : MachineBaseOperand(MachineBaseOperand::REG), reg(reg) {}
    std::string toString() {
        if (reg.is_virtual)
            return "%" + std::to_string(reg.reg_no);
        else
            return "phy_" + std::to_string(reg.reg_no);
    }
};

struct MachineImmediateInt : public MachineBaseOperand {
    int imm32;
    MachineImmediateInt(int imm32) : MachineBaseOperand(MachineBaseOperand::IMMI), imm32(imm32) {}
    std::string toString() { return std::to_string(imm32); }
};
struct MachineImmediateFloat : public MachineBaseOperand {
    float fimm32;
    MachineImmediateFloat(float fimm32) : MachineBaseOperand(MachineBaseOperand::IMMF), fimm32(fimm32) {}
    std::string toString() { return std::to_string(fimm32); }
};
struct MachineImmediateDouble : public MachineBaseOperand {
    double dimm64;
    MachineImmediateDouble(double dimm64) : MachineBaseOperand(MachineBaseOperand::IMMD), dimm64(dimm64) {}
    std::string toString() { return std::to_string(dimm64); }
};

//机器指令基类（MachineBaseInstruction）： 
//这是一个抽象类，表示一个机器指令
//它包含了指令架构类型（如RISC-V、ARM等），指令编号，指令操作数的读取和写入寄存器的函数，延迟时间（用于指令调度优化）等
//它的派生类需要实现获取读写寄存器以及延迟时间等方法。

//机器指令基类
class MachineBaseInstruction {
    public:
        enum { ARM = 0, RiscV, PHI};
        const int arch;

    private:
        int ins_number; // 指令编号, 用于活跃区间计算

    public:
        void setNumber(int ins_number) { this->ins_number = ins_number; }
        int getNumber() { return ins_number; }
        MachineBaseInstruction(int arch) : arch(arch) {}
        virtual std::vector<Register *> GetReadReg() = 0;     // 获得该指令所有读的寄存器
        virtual std::vector<Register *> GetWriteReg() = 0;    // 获得该指令所有写的寄存器
        virtual int GetLatency() = 0;    // 如果你不打算实现指令调度优化，可以忽略该函数
};

//Phi指令（MachinePhiInstruction）： 
//它包含了一个结果寄存器，并且有一个phi_list，用于存储与不同标签（通常是基本块标签）相关的操作数。

//GetReadReg和GetWriteReg：获取该指令涉及的读取和写入寄存器。
//removePhiList：从phi_list中移除指定标签的操作数。
//pushPhiList：向phi_list中添加新的操作数，可以是寄存器或立即数。

// 如果你没有实现优化的进阶要求，可以忽略下面的指令类
class MachinePhiInstruction : public MachineBaseInstruction {
    private:
        Register result;
        std::vector<std::pair<int, MachineBaseOperand *>> phi_list;

    public:
        std::vector<Register *> GetReadReg();
        std::vector<Register *> GetWriteReg();

        MachinePhiInstruction(Register result) : result(result), MachineBaseInstruction(MachineBaseInstruction::PHI) {}
        Register GetResult() { return result; }
        void SetResult(Register result) { this->result = result; }
        std::vector<std::pair<int, MachineBaseOperand *>> &GetPhiList() { return phi_list; }
        MachineBaseOperand *removePhiList(int label) {//移除某个label对应的操作数
            for (auto it = phi_list.begin(); it != phi_list.end(); ++it) {
                if (it->first == label) {
                    auto ret = it->second;
                    phi_list.erase(it);
                    return ret;
                }
            }
            return nullptr;
        }
        void pushPhiList(int label, Register reg) { phi_list.push_back(std::make_pair(label, new MachineRegister(reg))); }
        void pushPhiList(int label, int imm32) {
            phi_list.push_back(std::make_pair(label, new MachineImmediateInt(imm32)));
        }
        void pushPhiList(int label, float immf32) {
            phi_list.push_back(std::make_pair(label, new MachineImmediateFloat(immf32)));
        }
        void pushPhiList(int label, MachineBaseOperand *op) { phi_list.push_back(std::make_pair(label, op)); }
        int GetLatency() { return 0; }
};
#endif