#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <stdexcept>
#include <optional>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

enum Instruction
{
    RIGHT,
    LEFT,
    PLUS,
    MINUS,
    PUT,
    GET,
    LOOP,
    JMP
};

std::ostream &operator<<(std::ostream &os, const Instruction &inst)
{
    switch (inst)
    {
    case RIGHT:
        os << ">";
        break;
    case LEFT:
        os << "<";
        break;
    case PLUS:
        os << "+";
        break;
    case MINUS:
        os << "-";
        break;
    case PUT:
        os << ".";
        break;
    case GET:
        os << ",";
        break;
    case LOOP:
        os << "[";
        break;
    case JMP:
        os << "]";
        break;
    }
    return os;
}

struct Command
{
    Command(Instruction inst, size_t jumpTo)
        : inst(inst), jumpTo(jumpTo) {}
    Instruction inst;
    size_t jumpTo;
};

class LoopMismatch : public std::runtime_error
{
public:
    LoopMismatch(const std::string &what) : std::runtime_error(what) {}
};

std::vector<Command>
buildProgram(const std::vector<Instruction> &insts)
{
    std::stack<size_t> loopstack;
    std::vector<Command> cmds;

    for (size_t i = 0; i < insts.size(); i++)
    {
        const auto &inst = insts.at(i);
        switch (inst)
        {
        case JMP:
        {
            size_t jmpto = loopstack.top();
            loopstack.pop();
            cmds.emplace_back(inst, jmpto);
            cmds.at(jmpto).jumpTo = i;
            break;
        }
        case LOOP:
            loopstack.push(i);
            cmds.emplace_back(inst, 0);
            break;
        default:
            cmds.emplace_back(inst, 0);
            break;
        }
    }

    return cmds;
}

std::string asm_right() { return "inc r8"; }
std::string asm_left() { return "dec r8"; }
std::string asm_incr() { return "inc byte [rsp+r8]"; }
std::string asm_decr() { return "dec byte [rsp+r8]"; }
std::vector<std::string> asm_put()
{
    return {
        "mov rax, 1",
        "mov rdi, 1",
        "mov rsi, buf",
        "mov rdx, 1",
        "mov r9b, byte [rsp+r8]",
        "mov byte [buf], r9b",
        "syscall"};
}
std::vector<std::string> asm_get()
{
    return {
        "mov rax, 0",
        "mov rdi, 0",
        "mov rsi, buf",
        "mov rdx, 1",
        "syscall",
        "mov r9b, [buf]",
        "mov byte [rsp+r8], r9b"};
}
std::vector<std::string> asm_loop(const std::string &label, const std::string &jmpTo)
{
    return {
        label + ":",
        "mov r10b, byte [rsp+r8]",
        "test r10b, r10b",
        "jz " + jmpTo};
}
std::vector<std::string> asm_jmp(const std::string &label, const std::string &jmpTo)
{
    return {
        "jmp " + jmpTo,
        label + ":"};
}

std::string int_to_label(int n)
{
    return "LP" + std::to_string(n);
}

std::vector<std::string> asm_init()
{
    return {
        "global _start",
        "section .data",
        "buf: db 0",
        "section .text",
        "_start:",
        "xor r8, r8",
        "sub rsp, 30000"};
}

std::vector<std::string> asm_tail()
{
    return {
        "add rsp, 30000",
        "mov rax, 60",
        "xor rdi, rdi",
        "syscall"};
}

template <typename T>
void extend(std::vector<T> &vec, const std::vector<T> &ext)
{
    for (const auto &elem : ext)
    {
        vec.push_back(elem);
    }
}

std::vector<std::string> assembly(const std::vector<Command> &cmds)
{
    std::vector<std::string> asms = asm_init();
    for (size_t i = 0; i < cmds.size(); i++)
    {
        const auto &cmd = cmds.at(i);
        switch (cmd.inst)
        {
        case RIGHT:
            asms.push_back(asm_right());
            break;
        case LEFT:
            asms.push_back(asm_left());
            break;
        case PLUS:
            asms.push_back(asm_incr());
            break;
        case MINUS:
            asms.push_back(asm_decr());
            break;
        case PUT:
            extend(asms, asm_put());
            break;
        case GET:
            extend(asms, asm_get());
            break;
        case LOOP:
            extend(asms, asm_loop(
                             int_to_label(i),
                             int_to_label(cmd.jumpTo)));
            break;
        case JMP:
            extend(asms, asm_jmp(
                             int_to_label(i),
                             int_to_label(cmd.jumpTo)));
            break;
        }
    }
    extend(asms, asm_tail());
    return asms;
}

std::optional<Instruction> readChar(char c)
{
    switch (c)
    {
    case '+':
        return std::make_optional(PLUS);
    case '-':
        return std::make_optional(MINUS);
    case '>':
        return std::make_optional(RIGHT);
    case '<':
        return std::make_optional(LEFT);
    case '.':
        return std::make_optional(PUT);
    case ',':
        return std::make_optional(GET);
    case '[':
        return std::make_optional(LOOP);
    case ']':
        return std::make_optional(JMP);
    }
    return std::optional<Instruction>();
}

std::vector<Instruction> readInstructions(const std::string &text)
{
    std::vector<Instruction> insts;
    for (const char c : text)
    {
        auto i = readChar(c);
        if (i.has_value())
        {
            insts.push_back(std::move(i.value()));
        }
    }
    return insts;
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cerr << "usage: bfc <filename>" << std::endl;
        return 2;
    }

    std::ifstream in(argv[1]);
    if (!in.is_open())
    {
        std::cerr << "could not read " << argv[1] << std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << in.rdbuf();
    const auto programText = buffer.str();

    const auto insts = readInstructions(programText);
    const auto program = buildProgram(insts);
    const auto asmcode = assembly(program);

    const auto asmName = std::string(argv[1]) + "_out.asm.tmp";
    const auto objName = std::string(argv[1]) + "_obj.o";

    std::ofstream out(asmName);
    if (!out.is_open())
    {
        std::cerr << "could not open out.asm" << std::endl;
        return 1;
    }

    for (const auto &sasm : asmcode)
    {
        out << sasm << std::endl;
    }

    const auto nasmCmd = "nasm -felf64 -o \"" + objName + "\" \"" + asmName + "\"";
    std::cout << nasmCmd << std::endl;
    int nasmCode = system(nasmCmd.c_str());
    if (nasmCode != 0) {
        std::cerr << "NASM failed." << std::endl;
        return 1;
    }

    const auto ldCmd = "ld \"" + objName + "\"";
    std::cout << ldCmd << std::endl;
    int ldCode = system(ldCmd.c_str());
    if (ldCode != 0) {
        std::cerr << "ld failed." << std::endl;
        return 1;
    }

    // remove temporary files
    fs::remove(asmName);
    fs::remove(objName);

    return 0;
}
