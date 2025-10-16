#pragma once
#include <iostream>
#include <vector>
#include <unordered_map>
#include <functional>
#include <sstream>
#include <string>
#include <algorithm>
#include <thread>

// separate enum for instructions
enum class opcode {
    set,
    increase,
    decrease,
    show,
    jmp,
    jmpne,
    jmpe,
    unknown 
};

enum ro_reg : int{
    version,    // r0
    arch,       // r1
    os,         // r2
    cores,      // r3
    uptime      // r4
};
// each instruction consists of an opcode and operands
struct instruction {
    opcode op;
    std::vector<std::string> operands;
};
// ``
class parser {
public:
    parser() {
        //todo: define instructions as classes with an instruction parent with a run method
        //! then whe just call instr.run();
        // define instruction behaviors
        instructions[opcode::set] = [this](const std::vector<std::string>& ops) {
            int value = resolve_operand(ops[1]);
            if (is_rw_register(ops[0])) rw_registers[reg_idx(ops[0])] = value;
            else std::cerr << "cannot write to read-only register " << ops[0] << "\n";
            ip++;
            };

        instructions[opcode::increase] = [this](const std::vector<std::string>& ops) {
            int value = resolve_operand(ops[1]);
            if (is_rw_register(ops[0])) rw_registers[reg_idx(ops[0])] += value;
            else std::cerr << "cannot write to read-only register " << ops[0] << "\n";
            ip++;
            };

        instructions[opcode::decrease] = [this](const std::vector<std::string>& ops) {
            int value = resolve_operand(ops[1]);
            if (is_rw_register(ops[0])) rw_registers[reg_idx(ops[0])] -= value;
            else std::cerr << "cannot write to read-only register " << ops[0] << "\n";
            ip++;
            };
        // todo: fix spaces
        instructions[opcode::show] = [this](const std::vector<std::string>& ops) {
            if (!ops.empty() && ops[0].front() == '"' && ops[0].back() == '"') {
                std::cout << ops[0].substr(1, ops[0].size() - 2) << "\n";
            }
            else {
                int value = resolve_operand(ops[0]);
                std::cout << value << "\n";
            }
            
            ip++;
            };

        instructions[opcode::jmp] = [this](const std::vector<std::string>& ops) { // "jump"
            ip = std::stoi(ops[0]);
            };

        instructions[opcode::jmpe] = [this](const std::vector<std::string>& ops) { // "jump if equal"
            ip = (resolve_operand(ops[0]) == resolve_operand(ops[1])) ? std::stoi(ops[2]) : ip + 1;
            };

        instructions[opcode::jmpne] = [this](const std::vector<std::string>& ops) { // "jump if not equal"
            ip = (resolve_operand(ops[0]) != resolve_operand(ops[1])) ? std::stoi(ops[2]) : ip + 1;
            };

        opcode_map = {
            {"set", opcode::set},
            {"increase", opcode::increase},
            {"decrease", opcode::decrease},
            {"show", opcode::show},
            {"jmp", opcode::jmp},
            {"jmpne", opcode::jmpe},
            {"jmpe", opcode::jmpne}
        };
      
        ro_registers[ro_reg::version] = 1; // program version
        ro_registers[ro_reg::arch] = sizeof(void*) == 8 ? 64 : 32; // CPU arch
#ifdef _WIN32
        ro_registers[ro_reg::os] = 1;
#elif __linux__
        ro_registers[ro_reg::os] = 2;
#elif __APPLE__
        ro_registers[ro_reg::os] = 3;
#else
        ro_registers[ro_reg::os] = 0;
#endif
        
        ro_registers[ro_reg::cores] = std::thread::hardware_concurrency(); // CPU cores
       
        ro_registers[ro_reg::uptime] = 0;

        // writable registers r5-r10
        for (int i = 5; i <= 10; i++) rw_registers[i] = 0;
    }
    // main method to run a cld program
    void run(const std::string& src) {
        parse_source(src);//
        ip = 0;
        
        while (ip < program.size()) {
            auto& inst = program[ip];
            if (instructions.count(inst.op)) instructions[inst.op](inst.operands);
            else {
                std::cerr << "unknown instruction\n";
                ip++;
            }
        }
    }

private:
    std::vector<instruction> program;
    size_t ip = 0;

    int ro_registers[5]; // r0-r4
    int rw_registers[11]; // r0-r10, r5-r10 writable

    std::unordered_map<opcode, std::function<void(const std::vector<std::string>&)>> instructions;
    std::unordered_map<std::string, opcode> opcode_map;

    opcode str_to_opcode(const std::string& s) {
        auto it = opcode_map.find(s);
        return it != opcode_map.end() ? it->second : opcode::unknown;
    }

    int reg_idx(const std::string& reg) {
        return std::stoi(reg.substr(1));
    }

    bool is_rw_register(const std::string& reg) {
        int idx = reg_idx(reg);
        return idx >= 5 && idx <= 10;
    }

    int resolve_operand(const std::string& op) {
        if (op[0] == 'r') {
            int idx = reg_idx(op);
            if (idx <= 4) return ro_registers[idx];
            return rw_registers[idx];
        }
        return std::stoi(op);
    }

    void parse_source(const std::string& src) {
        std::vector<std::string> lines = split_lines(src);
        std::unordered_map<std::string, size_t> labels;

        for (size_t i = 0; i < lines.size(); i++) {
            std::string line = trim(lines[i]);
            if (line.empty() || line[0] == ';') continue;
            if (line.back() == ':') labels[line.substr(0, line.size() - 1)] = program.size();
            else program.push_back({ opcode::unknown, {} });
        }

        size_t prog_index = 0;
        for (auto& line : lines) {
            line = trim(line);
            if (line.empty() || line[0] == ';') continue;
            if (line.back() == ':') continue;

            std::istringstream iss(line);
            std::string cmd;
            iss >> cmd;
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            std::vector<std::string> ops;
            std::string op;
            while (iss >> op) {
                if (labels.count(op)) op = std::to_string(labels[op]);
                ops.push_back(op);
            }

            program[prog_index] = { str_to_opcode(cmd), ops };
            prog_index++;
        }
    }

    std::vector<std::string> split_lines(const std::string& src) {
        std::vector<std::string> lines;
        std::istringstream iss(src);
        std::string line;
        while (std::getline(iss, line)) lines.push_back(line);
        return lines;
    }

    std::string trim(const std::string& s) {
        size_t start = s.find_first_not_of(" \t");
        size_t end = s.find_last_not_of(" \t");
        if (start == std::string::npos) return "";
        return s.substr(start, end - start + 1);
    }
};
