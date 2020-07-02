#include <iostream>
#include <string>
#include <vector>
#include <stack>
#include <stdexcept>

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

std::string asm_right() { return "incr r9"; }
std::string asm_left() { return "decr r9"; }
std::string asm_incr() {}
std::string asm_decr() {}
std::string asm_put() {}
std::string asm_get() {}
std::string asm_loop(const std::string &label) {}
std::string asm_jmp(const std::string &label) {}

std::string int_to_label(int n)
{
    return "LP" + std::to_string(n);
}

std::vector<std::string> assembly(const std::vector<Command> &cmds)
{
    std::vector<std::string> asms;
    for (size_t i = 0; i < cmds.size(); i++)
    {
        const auto &cmd = cmds.at(i);
        switch (cmd.inst)
        {
        case RIGHT:
            asm_right();
            break;
        case LEFT:
            asm_left();
            break;
        case PLUS:
            asm_incr();
            break;
        case MINUS:
            asm_decr();
            break;
        case PUT:
            asm_put();
            break;
        case GET:
            asm_get();
            break;
        case LOOP:
            asm_loop(int_to_label(i));
            break;
        case JMP:
            asm_jmp(int_to_label(cmd.jumpTo));
            break;
        }
    }
    return asms;
}

int main(int argc, char **argv)
{
    std::vector<Instruction> insts{PLUS, PLUS, LOOP, PLUS, MINUS, JMP, RIGHT};
    const auto program = buildProgram(insts);
    const auto asmcode = assembly(program);
    for (const auto &sasm : asmcode)
    {
        std::cout << sasm << std::endl;
    }
    return 0;
}
