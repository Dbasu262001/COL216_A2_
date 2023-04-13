/**
 * @file MIPS_Processor.hpp
 * @author Mallika Prabhakar and Sayam Sethi
 * 
 */

#ifndef __MIPS_PROCESSOR_HPP__
#define __MIPS_PROCESSOR_HPP__

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <fstream>
#include <exception>
#include <iostream>
#include <boost/tokenizer.hpp>
struct Control_Signals{
	bool WB_1;
	bool WB_branch;
	bool MEM_Stage_1;
	bool MEM_Stage_2;
	bool ALU_Stage_;
	bool RR_;
	bool ID_Stage_1;
	bool ID_Stage_2;
	bool IF_Stage_1;
	bool IF_Stage_2;
	bool stage_7;
	bool stage_9;
	int count;

};
struct IF{
	int PC_value;
	std::vector< std::string> command; 
};
struct ID{
	std::string op;
	int operation;
	bool _stages7;
	bool alu_operation;
	bool branch_instruction;
	std::string register_r1;
	std::string register_r2;
	std::string register_r3;
	std::string label;
	int branch_condition;  // 0 for beq 1 for bne  else -1;
	int immediate;
	int reg1_value;
	int reg2_value;
	int reg3_value;
	//bool load_store;
};
struct RR{
	bool alu_operation;
	std::string op;
	int operation;
	bool _stage7;
	bool branch_instruction;
	std::string destination_register;
	int reg1_value;
	int reg2_value;
	int reg3_value;
	std::string label;
	std::string location;
	//bool load_store;
};

struct EX{
	std::string dest_register;
	int data;
	bool zero_output;
	bool reg_write;
	bool mem_write;
	bool mem_read;
	bool branch_satisfied;
	int load_or_store_mem_address;
	bool branch_instruction;
	int label_address;
	bool jump;
};
struct MEM{
	std::string dest_register;
	bool reg_write;
	int data;

};
struct WB{

};


struct MIPS_Architecture
{
	int registers[32] = {0}, PCcurr = 0, PCnext;
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions;
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, std::string, std::string)>> instructions_pipeline;
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &,bool,std::string, std::string, std::string)>> instructions_decode;
	std::unordered_map<std::string, std::function<int(MIPS_Architecture &, std::string, int, int)>> instructions_execute;

	std::unordered_map<std::string, int> registerMap, address;
	static const int MAX = (1 << 20);
	int data[MAX >> 2] = {0};
	std::vector<std::vector<std::string>> commands;
	std::vector<int> commandCount;
	std::unordered_map<int, int> memoryDelta;



	enum exit_code
	{
		SUCCESS = 0,
		INVALID_REGISTER,
		INVALID_LABEL,
		INVALID_ADDRESS,
		SYNTAX_ERROR,
		MEMORY_ERROR
	};
	//Enum type added
	enum ALU_execution_mode{
		addition =0,
		substraction,
		multiplication,
		beq_,
		bne_,
		slt_,
		j_,
		lw_,
		sw_,
		addimmediate

	};


	//Structures Added
	struct Control_Signals  pipeline_controls;
	struct IF _IF1_latch;
	struct IF _IF2_latch;
	struct ID _ID1_latch;
	struct ID _ID2_latch;
	struct EX _EX_latch;
	struct RR _RR_latch;
	struct MEM _MEM_latch;
	struct MEM _MEM2_latch;
	struct WB  _WB_latch;

	// constructor to initialise the instruction set
	MIPS_Architecture(std::ifstream &file)
	{
		instructions = {{"add", &MIPS_Architecture::add}, {"sub", &MIPS_Architecture::sub}, {"mul", &MIPS_Architecture::mul}, {"beq", &MIPS_Architecture::beq}, {"bne", &MIPS_Architecture::bne}, {"slt", &MIPS_Architecture::slt}, {"j", &MIPS_Architecture::j}, {"lw", &MIPS_Architecture::lw}, {"sw", &MIPS_Architecture::sw}, {"addi", &MIPS_Architecture::addi}};
		instructions_decode = {{"add", &MIPS_Architecture::decode_add}, {"sub", &MIPS_Architecture::decode_sub}, {"mul", &MIPS_Architecture::decode_mul}, {"beq", &MIPS_Architecture::decode_beq}, {"bne", &MIPS_Architecture::decode_bne}, {"slt", &MIPS_Architecture::decode_slt}, {"j", &MIPS_Architecture::decode_j}, {"lw", &MIPS_Architecture::decode_lw}, {"sw", &MIPS_Architecture::decode_sw}, {"addi", &MIPS_Architecture::decode_addi}};
		instructions_execute ={{"add", &MIPS_Architecture::alu_add}, {"sub", &MIPS_Architecture::alu_sub}, {"mul", &MIPS_Architecture::alu_mul}, {"beq", &MIPS_Architecture::alu_beq}, {"bne", &MIPS_Architecture::alu_bne}, {"slt", &MIPS_Architecture::alu_slt}, {"lw", &MIPS_Architecture::alu_lw}, {"sw", &MIPS_Architecture::alu_sw}, {"addi", &MIPS_Architecture::alu_addi},{"j", &MIPS_Architecture::alu_j}};

		for (int i = 0; i < 32; ++i)
			registerMap["$" + std::to_string(i)] = i;
		registerMap["$zero"] = 0;
		registerMap["$at"] = 1;
		registerMap["$v0"] = 2;
		registerMap["$v1"] = 3;
		for (int i = 0; i < 4; ++i)
			registerMap["$a" + std::to_string(i)] = i + 4;
		for (int i = 0; i < 8; ++i)
			registerMap["$t" + std::to_string(i)] = i + 8, registerMap["$s" + std::to_string(i)] = i + 16;
		registerMap["$t8"] = 24;
		registerMap["$t9"] = 25;
		registerMap["$k0"] = 26;
		registerMap["$k1"] = 27;
		registerMap["$gp"] = 28;
		registerMap["$sp"] = 29;
		registerMap["$s8"] = 30;
		registerMap["$ra"] = 31;

		constructCommands(file);
		commandCount.assign(commands.size(), 0);
	}
	//Perform decode stage 
	int decode_add(bool decode_read,std::string r1,std::string r2, std::string r3){
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		_ID1_latch.alu_operation =true;
		_ID1_latch.operation = addition;
		_ID1_latch._stages7 = true;
		_ID1_latch.register_r1 = r1;
		_ID1_latch.register_r2 = r2;
		_ID1_latch.register_r3 = r3;
		_ID1_latch.branch_instruction = false;
		if(decode_read){
			return 0;
		}

		return 0;
	}
	int decode_sub(bool decode_read,std::string r1,std::string r2, std::string r3){
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		_ID1_latch.alu_operation =true;
		_ID1_latch.operation = substraction;
		_ID1_latch._stages7 = true;
		_ID1_latch.register_r1 = r1;
		_ID1_latch.register_r2 = r2;
		_ID1_latch.register_r3 = r3;
		_ID1_latch.branch_instruction = false;
		if(decode_read){
			return 0;
		}

		return 0;
	}
	int decode_mul(bool decode_read,std::string r1,std::string r2, std::string r3){
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		_ID1_latch.alu_operation =true;
		_ID1_latch.operation = multiplication;
		_ID1_latch.register_r1 = r1;
		_ID1_latch.register_r2 = r2;
		_ID1_latch._stages7 = true;
		_ID1_latch.register_r3 = r3;
		_ID1_latch.branch_instruction = false;
		if(decode_read){
			return 0;
		}

		return 0;
	}
	int decode_beq(bool decode_read,std::string r1,std::string r2, std::string label){
		if (!checkLabel(label)){return 4;}

		if (address.find(label) == address.end() || address[label] == -1){
			return 2;
		}
		if (!checkRegisters({r1, r2})){
			return 1;
		}
		_ID1_latch.alu_operation = true;
		_ID1_latch.operation = beq_;
		_ID1_latch._stages7 = true;
		_ID1_latch.branch_instruction = true;
		_ID1_latch.register_r2 = r1;
		_ID1_latch.register_r3 = r2;
		_ID1_latch.label = label;
		_ID1_latch.branch_condition =0;
		if(decode_read){
			return 0;
		}		
		return 0;
	}
	int decode_bne(bool decode_read,std::string r1,std::string r2, std::string label){
		if (!checkLabel(label)){
			return 4;
		}
		if (address.find(label) == address.end() || address[label] == -1){
			return 2;
		}
		if (!checkRegisters({r1, r2})){
			return 1;
		}
		_ID1_latch.alu_operation = true;
		_ID1_latch.operation= bne_;
		_ID1_latch.branch_instruction = true;
		_ID1_latch.register_r2 = r1;
		_ID1_latch._stages7 = true;
		_ID1_latch.register_r3 = r2;
		_ID1_latch.label = label;
		_ID1_latch.branch_condition =1;
		if(decode_read){
			return 0; 
		}
		return 0;		
	}
	int decode_slt(bool decode_read,std::string r1,std::string r2, std::string r3){
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		_ID1_latch.alu_operation = true;
		_ID1_latch.operation = slt_;
		_ID1_latch.branch_instruction = false;
		_ID1_latch.register_r1 = r1;
		_ID1_latch._stages7 = true;
		_ID1_latch.register_r2 = r2;
		_ID1_latch.register_r3 = r3;
		_ID1_latch.branch_condition =-1;
		if(decode_read){
			return 0;
		}		
		return 0;
	}

	int decode_j(bool decode_read,std::string label,std::string unused1="" , std::string unused2 = "" ){
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		_ID1_latch.alu_operation = false;
		_ID1_latch.operation = j_;
		_ID1_latch._stages7 = true;
		_ID1_latch.branch_instruction = false;
		_ID1_latch.label = label;
		_ID1_latch.branch_condition =-1;
		if(decode_read){
			return 0;
		}
		return 0;
	}

	int decode_lw(bool decode_read,std::string r, std::string location,std::string unused2 =""){
		if (!checkRegister(r) || registerMap[r] == 0)
			return 1;
		if(decode_read){
			return 0;
		}
		_ID1_latch._stages7 = false;
		_ID1_latch.alu_operation =true;
		_ID1_latch.operation = lw_;
		_ID1_latch.branch_instruction = false;
		_ID1_latch.register_r1 = r;
		_ID1_latch.register_r2 = location; //location in r2 
		_ID1_latch.branch_condition =-1;
		if(decode_read){
			return 0;
		}
		return 0;
	}

	int decode_sw(bool decode_read,std::string r,std::string location,std::string unused2 =""){
		if (!checkRegister(r) || registerMap[r] == 0)
			return 1;
		if(decode_read){
			return 0;
		}
		_ID1_latch._stages7 = false;
		_ID1_latch.alu_operation =true;
		_ID1_latch.operation = sw_;
		_ID1_latch.branch_instruction = false;
		_ID1_latch.register_r1 = r;
		_ID1_latch.register_r2 = location; //location in r2 
		_ID1_latch.branch_condition =-1;
		if(decode_read){
			return 0;
		}
		return 0;

	}
/////////////////////
	int decode_addi(bool decode_read,std::string r1, std::string r2, std::string num)
	{
		if (!checkRegisters({r1, r2}) || registerMap[r1] == 0)
			return 1;
		_ID1_latch.alu_operation =true;
		_ID1_latch.operation = addimmediate;
		_ID1_latch._stages7 = true;
		_ID1_latch.branch_instruction = false;
		_ID1_latch.register_r1 = r1;
		_ID1_latch.register_r2 = r2; 
		_ID1_latch.immediate = stoi(num);//location in r2 
		_ID1_latch.branch_condition =-1;
		if(decode_read){
			return 0;
		}
		return 0;
		
	}

//ALU STAGE IMPLEMENTATION
	int alu_add(std::string dest_regs,int a,int b){
		_EX_latch.data = a+b;
		pipeline_controls.stage_7 = true;
		_EX_latch.reg_write = true;
		_EX_latch.mem_read =false;
		_EX_latch.mem_write =false;
		_EX_latch.dest_register = dest_regs;
		_EX_latch.zero_output = false;
		_EX_latch.branch_instruction = false;
		_EX_latch.branch_satisfied = false; 

		return 0;
	}
	int alu_addi(std::string dest_regs,int a,int b){
		_EX_latch.data = a+b;
		_EX_latch.reg_write = true;
		_EX_latch.mem_read =false;
		pipeline_controls.stage_7 = true;
		_EX_latch.mem_write =false;
		_EX_latch.dest_register = dest_regs;
		_EX_latch.zero_output = false;
		_EX_latch.branch_satisfied = false; 
		_EX_latch.branch_instruction = false;

		return 0;
	}
	int alu_sub(std::string dest_regs,int a,int b){
		_EX_latch.data = a-b;
		_EX_latch.reg_write = true;
		_EX_latch.mem_read =false;
		pipeline_controls.stage_7 = true;
		_EX_latch.mem_write =false;
		_EX_latch.dest_register = dest_regs;
		_EX_latch.zero_output = false;
		_EX_latch.branch_instruction = false;

		return 0;
	}
	int alu_mul(std::string dest_regs,int a,int b){
		_EX_latch.data = a*b;
		_EX_latch.reg_write = true;
		_EX_latch.mem_read =false;
		_EX_latch.mem_write =false;
		_EX_latch.dest_register = dest_regs;
		pipeline_controls.stage_7 = true;
		_EX_latch.zero_output = false;
		_EX_latch.branch_instruction = false;

		return 0;
	}
	int alu_bne(std::string dest_regs,int a,int b){
		_EX_latch.data = a-b;
		_EX_latch.reg_write = false;
		_EX_latch.mem_read =false;
		_EX_latch.mem_write =false;
		pipeline_controls.stage_7 = true;
		_EX_latch.dest_register = dest_regs;
		std::cout<<"I am called bneq"<<std::endl;

		if(a !=b){
			_EX_latch.branch_satisfied = true; 
			pipeline_controls.WB_branch = true;
			_EX_latch.label_address = address[_RR_latch.label];
		}else{
			pipeline_controls.WB_branch = false;
			_EX_latch.branch_satisfied = false; 
		}
		_EX_latch.branch_instruction = true;

		return 0;
	}
	int alu_beq(std::string dest_regs,int a,int b){
		_EX_latch.reg_write = false;
		_EX_latch.mem_read =false;
		_EX_latch.mem_write =false;
		_EX_latch.dest_register = dest_regs;
		pipeline_controls.stage_7 = true;
		std::cout<<"I am called beq"<<std::endl;
		if(a ==b){
			_EX_latch.branch_satisfied = true; 
			_EX_latch.label_address = address[_RR_latch.label];
			pipeline_controls.WB_branch = true;
		}else{
			_EX_latch.branch_satisfied = false; 
			pipeline_controls.WB_branch = false;
		}
		_EX_latch.branch_instruction = true;
		return 0;
	}
	int alu_slt(std::string dest_regs,int a,int b){
		_EX_latch.data = a < b;
		_EX_latch.reg_write = true;
		_EX_latch.mem_read =false;
		_EX_latch.mem_write =false;
		pipeline_controls.stage_7 = true;
		_EX_latch.dest_register = dest_regs;
		_EX_latch.branch_satisfied = false; 
		_EX_latch.branch_instruction = false;

		return 0;
	}
	
	int alu_lw(std::string dest_regs,int a =0,int b =10){
		_EX_latch.mem_read = true;
		_EX_latch.mem_write = false;
		_EX_latch.reg_write = true;
		_EX_latch.dest_register = dest_regs;
		_EX_latch.load_or_store_mem_address = locateAddress(_RR_latch.location);
		_EX_latch.zero_output = false;
		_EX_latch.branch_instruction = false;

		return 0;
	}

	int alu_sw(std::string data,int address=0,int b =10){
		_EX_latch.mem_read = false;
		_EX_latch.reg_write = false;
		_EX_latch.data = _RR_latch.reg1_value;
		_EX_latch.mem_write = true;

		_EX_latch.load_or_store_mem_address = locateAddress(_RR_latch.location);
		_EX_latch.zero_output = false;
		_EX_latch.branch_instruction = false;

		return 0;
	}

	int alu_j(std::string data ="",int a =0, int b =0){
		_EX_latch.label_address = address[_RR_latch.label];
		_EX_latch.jump = true;
		return 0;
	}
	// perform add operation
	int add(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a + b; });
	}
	// perform subtraction operation
	int sub(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
				  { return a - b; });
	}

	// perform multiplication operation
	int mul(std::string r1, std::string r2, std::string r3)
	{
		return op(r1, r2, r3, [&](int a, int b)
		  { return a * b; });
	}

	// perform the binary operation
	int op(std::string r1, std::string r2, std::string r3, std::function<int(int, int)> operation)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = operation(registers[registerMap[r2]], registers[registerMap[r3]]);
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the beq operation
	int beq(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a == b; });
	}

	// perform the bne operation
	int bne(std::string r1, std::string r2, std::string label)
	{
		return bOP(r1, r2, label, [](int a, int b)
				   { return a != b; });
	}

	// implements beq and bne by taking the comparator
	int bOP(std::string r1, std::string r2, std::string label, std::function<bool(int, int)> comp)
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		if (!checkRegisters({r1, r2}))
			return 1;
		PCnext = comp(registers[registerMap[r1]], registers[registerMap[r2]]) ? address[label] : PCcurr + 1;
		return 0;
	}

	// implements slt operation
	int slt(std::string r1, std::string r2, std::string r3)
	{
		if (!checkRegisters({r1, r2, r3}) || registerMap[r1] == 0)
			return 1;
		registers[registerMap[r1]] = registers[registerMap[r2]] < registers[registerMap[r3]];
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform the jump operation
	int j(std::string label, std::string unused1 = "", std::string unused2 = "")
	{
		if (!checkLabel(label))
			return 4;
		if (address.find(label) == address.end() || address[label] == -1)
			return 2;
		PCnext = address[label];
		return 0;
	}

	// perform load word operation
	int lw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r) || registerMap[r] == 0)
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		registers[registerMap[r]] = data[address];
		PCnext = PCcurr + 1;
		return 0;
	}

	// perform store word operation
	int sw(std::string r, std::string location, std::string unused1 = "")
	{
		if (!checkRegister(r))
			return 1;
		int address = locateAddress(location);
		if (address < 0)
			return abs(address);
		data[address] = registers[registerMap[r]];
		PCnext = PCcurr + 1;
		return 0;
	}

	int get_register(std::string location){
		try{
			int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
			std::string reg = location.substr(lparen + 1);
			reg.pop_back();
			if (!checkRegister(reg))
				return -3;
			int reg_id = registerMap[reg];
			return reg_id;
		}catch(std::exception &e){
			return -4;
		}
	}
	int locateAddress(std::string location)
	{
		if (location.back() == ')')
		{
			try
			{
				int lparen = location.find('('), offset = stoi(lparen == 0 ? "0" : location.substr(0, lparen));
				std::string reg = location.substr(lparen + 1);
				reg.pop_back();
				if (!checkRegister(reg))
					return -3;
				int address = registers[registerMap[reg]] + offset;
				if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
					return -3;
				return address / 4;
			}
			catch (std::exception &e)
			{
				return -4;
			}
		}
		try
		{
			int address = stoi(location);
			if (address % 4 || address < int(4 * commands.size()) || address >= MAX)
				return -3;
			return address / 4;
		}
		catch (std::exception &e)
		{
			return -4;
		}
	}

	// perform add immediate operation
	int addi(std::string r1, std::string r2, std::string num)
	{
		if (!checkRegisters({r1, r2}) || registerMap[r1] == 0)
			return 1;
		try
		{
			registers[registerMap[r1]] = registers[registerMap[r2]] + stoi(num);
			PCnext = PCcurr + 1;
			return 0;
		}
		catch (std::exception &e)
		{
			return 4;
		}
	}

	// checks if label is valid
	inline bool checkLabel(std::string str)
	{
		return str.size() > 0 && isalpha(str[0]) && all_of(++str.begin(), str.end(), [](char c)
														   { return (bool)isalnum(c); }) &&
			   instructions.find(str) == instructions.end();
	}

	// checks if the register is a valid one
	inline bool checkRegister(std::string r)
	{
		return registerMap.find(r) != registerMap.end();
	}

	// checks if all of the registers are valid or not
	bool checkRegisters(std::vector<std::string> regs)
	{
		return std::all_of(regs.begin(), regs.end(), [&](std::string r)
						   { return checkRegister(r); });
	}

	/*
		handle all exit codes:
		0: correct execution
		1: register provided is incorrect
		2: invalid label
		3: unaligned or invalid address
		4: syntax error
		5: commands exceed memory limit
	*/
	void handleExit(exit_code code, int cycleCount)
	{
		std::cout << '\n';
		switch (code)
		{
		case 1:
			std::cerr << "Invalid register provided or syntax error in providing register\n";
			break;
		case 2:
			std::cerr << "Label used not defined or defined too many times\n";
			break;
		case 3:
			std::cerr << "Unaligned or invalid memory address specified\n";
			break;
		case 4:
			std::cerr << "Syntax error encountered\n";
			break;
		case 5:
			std::cerr << "Memory limit exceeded\n";
			break;
		default:
			break;
		}
		if (code != 0)
		{
			std::cerr << "Error encountered at:\n";
			for (auto &s : commands[PCcurr])
				std::cerr << s << ' ';
			std::cerr << '\n';
		}
		std::cout << "\nFollowing are the non-zero data values:\n";
		for (int i = 0; i < MAX / 4; ++i)
			if (data[i] != 0)
				std::cout << 4 * i << '-' << 4 * i + 3 << std::hex << ": " << data[i] << '\n'
						  << std::dec;
		std::cout << "\nTotal number of cycles: " << cycleCount << '\n';
		std::cout << "Count of instructions executed:\n";
		for (int i = 0; i < (int)commands.size(); ++i)
		{
			std::cout << commandCount[i] << " times:\t";
			for (auto &s : commands[i])
				std::cout << s << ' ';
			std::cout << '\n';
		}
	}

	// parse the command assuming correctly formatted MIPS instruction (or label)
	void parseCommand(std::string line)
	{
		// strip until before the comment begins
		line = line.substr(0, line.find('#'));
		std::vector<std::string> command;
		boost::tokenizer<boost::char_separator<char>> tokens(line, boost::char_separator<char>(", \t"));
		for (auto &s : tokens)
			command.push_back(s);
		// empty line or a comment only line
		if (command.empty())
			return;
		else if (command.size() == 1)
		{
			std::string label = command[0].back() == ':' ? command[0].substr(0, command[0].size() - 1) : "?";
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command.clear();
		}
		else if (command[0].back() == ':')
		{
			std::string label = command[0].substr(0, command[0].size() - 1);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command = std::vector<std::string>(command.begin() + 1, command.end());
		}
		else if (command[0].find(':') != std::string::npos)
		{
			int idx = command[0].find(':');
			std::string label = command[0].substr(0, idx);
			if (address.find(label) == address.end())
				address[label] = commands.size();
			else
				address[label] = -1;
			command[0] = command[0].substr(idx + 1);
		}
		else if (command[1][0] == ':')
		{
			if (address.find(command[0]) == address.end())
				address[command[0]] = commands.size();
			else
				address[command[0]] = -1;
			command[1] = command[1].substr(1);
			if (command[1] == "")
				command.erase(command.begin(), command.begin() + 2);
			else
				command.erase(command.begin(), command.begin() + 1);
		}
		if (command.empty())
			return;
		if (command.size() > 4)
			for (int i = 4; i < (int)command.size(); ++i)
				command[3] += " " + command[i];
		command.resize(4);
		commands.push_back(command);
	}

	// construct the commands vector from the input file
	void constructCommands(std::ifstream &file)
	{
		std::string line;
		while (getline(file, line))
			parseCommand(line);
		file.close();
	}

	// execute the commands sequentially (no pipelining)
	void executeCommandsUnpipelined()
	{
		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

		int clockCycles = 0;
		while (PCcurr < commands.size())
		{
			++clockCycles;
			std::vector<std::string> &command = commands[PCcurr];
			if (instructions.find(command[0]) == instructions.end())
			{
				handleExit(SYNTAX_ERROR, clockCycles);
				return;
			}
			exit_code ret = (exit_code) instructions[command[0]](*this, command[1], command[2], command[3]);
			if (ret != SUCCESS)
			{
				handleExit(ret, clockCycles);
				return;
			}
			++commandCount[PCcurr];
			PCcurr = PCnext;
			printRegisters(clockCycles);
		}
		handleExit(SUCCESS, clockCycles);
	}

//Stages of ALU Execution

//Instruction fetch
	int IF_Stage1(int Program_Counter,int clockCycles){
		if(pipeline_controls.IF_Stage_1 ==false){
			return 0;
		}
		if(pipeline_controls.IF_Stage_2 == true){
			PCcurr = PCcurr -1;
			return 0;
		}
		std::cout<<"Program Counter "<<Program_Counter<<std::endl;
		if(Program_Counter < commands.size()){
			_IF1_latch.command = commands[Program_Counter];
			pipeline_controls.IF_Stage_2 = true;
			pipeline_controls.count = pipeline_controls.count -1;

		}else{
			pipeline_controls.IF_Stage_1 = false;
			pipeline_controls.count = pipeline_controls.count +1;
		}
		

		return 0;
	}

	//Stage 2
	int IF_Stage2(int clockCycles){
		if(pipeline_controls.IF_Stage_2 == false){
			return 0;
		}
		if(pipeline_controls.ID_Stage_1 == true){
			return 0;
		}
		
		_IF2_latch.command = _IF1_latch.command;
		pipeline_controls.IF_Stage_2 = false;
		pipeline_controls.ID_Stage_1 = true;
		return 0;
	}
//Instruction Decode
	int ID_Stage1(std::vector<std::string> &command,int clockCycles){

		if(pipeline_controls.ID_Stage_1 ==false){
			return 0;
		}
		if(pipeline_controls.ID_Stage_2){
			return 0;			
		}
		if (instructions.find(command[0]) == instructions.end())
			{
				handleExit(SYNTAX_ERROR, clockCycles);
				return 4;
			}
			//latch->operation= command[0];
		exit_code ret = (exit_code)instructions_decode[_IF2_latch.command[0]](*this,false,_IF2_latch.command[1],_IF2_latch.command[2],_IF2_latch.command[3]);
		_ID1_latch.op = _IF2_latch.command[0];
		pipeline_controls.ID_Stage_1 = false;
		pipeline_controls.ID_Stage_2 = true;
		
		return 0;
	}
	int ID_Stage2(int clockCycles){
		if(pipeline_controls.ID_Stage_2 ==false){
			return 0;
		}
		if(pipeline_controls.RR_){
			return 0;			
		}
		_ID2_latch = _ID1_latch;
		pipeline_controls.ID_Stage_2 = false;
		pipeline_controls.RR_ = true;
		return 0;
	}

	int RR(int clockCycles){
		if(pipeline_controls.RR_ ==false){
			return 0;
		}
		if(pipeline_controls.ALU_Stage_){
			return 0;						
		}
		//Check Stalls
		int t1 =-10,t2 =-10,t3=-10;
		if(pipeline_controls.WB_1){
			if(pipeline_controls.stage_7 ==true && _EX_latch.reg_write==true){
				t3 = registerMap[_EX_latch.dest_register];
			}else if(pipeline_controls.stage_9 ==true && _MEM2_latch.reg_write == true){
				t3 = registerMap[_MEM2_latch.dest_register];
			}
		}
		if(pipeline_controls.MEM_Stage_2 ==true && _MEM_latch.reg_write ==true){
			t1 = registerMap[_MEM_latch.dest_register];
		}
		if(pipeline_controls.MEM_Stage_1 == true && _EX_latch.reg_write == true){
			t2 = registerMap[_EX_latch.dest_register]; 
		}
		if(_ID2_latch.operation == j_){
			PCcurr = address[_ID2_latch.label];
			if(pipeline_controls.ID_Stage_2 == true){
				pipeline_controls.ID_Stage_2 = false;
				pipeline_controls.count = pipeline_controls.count + 1;
			}
			if(pipeline_controls.ID_Stage_1 == true){
				pipeline_controls.ID_Stage_1 = false;
				pipeline_controls.count = pipeline_controls.count +1;
			}
			if(pipeline_controls.IF_Stage_2 == true){
				pipeline_controls.ID_Stage_2 = false;
				pipeline_controls.count = pipeline_controls.count +1;
			}
			pipeline_controls.RR_ = false;
			pipeline_controls.count = pipeline_controls.count +1;
			if(!pipeline_controls.IF_Stage_1){
				pipeline_controls.IF_Stage_1 = true;
				pipeline_controls.count=pipeline_controls.count-1;
				}

			return 0;
			
		}else if(!_ID2_latch.branch_instruction && _ID2_latch._stages7 ==true){

			if(_ID2_latch.operation != addimmediate){
				if(t3 == registerMap[_ID2_latch.register_r3] || t3 == registerMap[_ID2_latch.register_r2] ||t1 == registerMap[_ID2_latch.register_r3] || t1 == registerMap[_ID2_latch.register_r2] || t2 == registerMap[_ID2_latch.register_r2] || t2 == registerMap[_ID2_latch.register_r3]){
					return 0;
				}
				_RR_latch.reg3_value = registers[registerMap[_ID2_latch.register_r3]];
			}else{
				if(t3 == registerMap[_ID2_latch.register_r2] || t1 == registerMap[_ID2_latch.register_r2] || t2 == registerMap[_ID2_latch.register_r2] ){
					return 0;
				}
				_RR_latch.reg3_value = _ID2_latch.immediate;
			}
			_RR_latch.op  = _ID2_latch.op;
			_RR_latch.alu_operation = true;
			_RR_latch.branch_instruction = false;
			_RR_latch._stage7 =true;
			_RR_latch.destination_register = _ID2_latch.register_r1;
			_RR_latch.reg2_value = registers[registerMap[_ID2_latch.register_r2]];
			

		}else if(_ID2_latch.branch_instruction){
			if(t3 == registerMap[_ID2_latch.register_r3] || t3 == registerMap[_ID2_latch.register_r2] ||t1 == registerMap[_ID2_latch.register_r3] || t1 == registerMap[_ID2_latch.register_r2] || t2 == registerMap[_ID2_latch.register_r2] || t2 == registerMap[_ID2_latch.register_r3]){
					return 0;
				}
			_RR_latch.label = _ID2_latch.label;
			_RR_latch.alu_operation = true;
			_RR_latch.op  = _ID2_latch.op;
			_RR_latch.branch_instruction = true;
			_RR_latch._stage7 =true;
			_RR_latch.destination_register = _ID2_latch.register_r1;
			_RR_latch.reg2_value = registers[registerMap[_ID2_latch.register_r2]];
			_RR_latch.reg3_value = registers[registerMap[_ID2_latch.register_r3]];

		}else if(_ID2_latch.operation == lw_ || sw_){
			 if(_ID2_latch.operation == lw_){
				if(t3 ==get_register(_ID2_latch.register_r2) || t1 ==get_register(_ID2_latch.register_r2) || t2 ==get_register(_ID2_latch.register_r2) ){
					return 0;
				}
			 }
			if(_ID2_latch.operation = sw_){
				if(t3 ==get_register(_ID2_latch.register_r2) || t3 == registerMap[_ID2_latch.register_r1] || t1 ==get_register(_ID2_latch.register_r2) || t2 ==get_register(_ID2_latch.register_r2) || t1 == registerMap[_ID2_latch.register_r1] || t2 == registerMap[_ID2_latch.register_r1]){
					return 0;
				}
				_RR_latch.reg1_value = registers[registerMap[_ID2_latch.register_r1]];
			}
			_RR_latch.alu_operation = true;
			_RR_latch.branch_instruction = false;
			_RR_latch._stage7 =false;
			_RR_latch.destination_register = _ID2_latch.register_r1;
			_RR_latch.location =  _ID2_latch.register_r2;
			_RR_latch.op  = _ID2_latch.op;
			_RR_latch.reg2_value =0;
			_RR_latch.reg3_value =0;
			
		}
		pipeline_controls.ALU_Stage_ =true;
		pipeline_controls.RR_ = false;
		return 0;
	}
//ALU Stage
	int ALU_Stage(int clockCycles){
		if(!pipeline_controls.ALU_Stage_){
			return 0;
		}
		if(_RR_latch._stage7 == true && _RR_latch.branch_instruction == true){
			exit_code ret = (exit_code)instructions_execute[_RR_latch.op](*this,_RR_latch.destination_register,_RR_latch.reg2_value,_RR_latch.reg3_value);
			pipeline_controls.stage_7 = false;
			pipeline_controls.ALU_Stage_ = false;
			pipeline_controls.count = pipeline_controls.count+1;
			return 0;
		}
		if(_RR_latch._stage7){
			if(pipeline_controls.WB_1 ==true){
			return 0;
			}else{
				exit_code ret = (exit_code)instructions_execute[_RR_latch.op](*this,_RR_latch.destination_register,_RR_latch.reg2_value,_RR_latch.reg3_value);
				pipeline_controls.stage_7 =true;
				pipeline_controls.WB_1 =true;
				pipeline_controls.ALU_Stage_ =false;

			}
		}else{
			if(pipeline_controls.MEM_Stage_1){
				return 0;
			}else{
				exit_code ret = (exit_code)instructions_execute[_RR_latch.op](*this,_RR_latch.destination_register,_RR_latch.reg2_value,_RR_latch.reg3_value);
				pipeline_controls.ALU_Stage_ = false;
				pipeline_controls.MEM_Stage_1 = true;
			}
		}
	return 0;
	}


//MEM Stage
	int  MEM_stage1(int clockCycles){
		if(!pipeline_controls.MEM_Stage_1){
			return 0;
		}
		if(pipeline_controls.MEM_Stage_2){
			return 0;			
		}
		_MEM_latch.reg_write =_EX_latch.reg_write;
		_MEM_latch.dest_register = _EX_latch.dest_register;
		if(_EX_latch.mem_read){
			_MEM_latch.data = data[_EX_latch.load_or_store_mem_address];
		}else if(_EX_latch.mem_write){
			if (data[_EX_latch.load_or_store_mem_address] != _EX_latch.data)
				memoryDelta[_EX_latch.load_or_store_mem_address] = _EX_latch.data;
			data[_EX_latch.load_or_store_mem_address] = _EX_latch.data;
		}
		pipeline_controls.MEM_Stage_1 = false;
		pipeline_controls.MEM_Stage_2 = true;
		return 0;
	}

	int MEM_Stage2(int clockCycles){
		if(pipeline_controls.MEM_Stage_2 ==false){
			return 0;
		}
		if(pipeline_controls.WB_1){
			return 0;
		}

		pipeline_controls.stage_9 = true;
		_MEM2_latch = _MEM_latch;
		pipeline_controls.MEM_Stage_2 = false;
		pipeline_controls.WB_1 = true;
		return 0;
	}
// Writeback
	int WB(int clockCycles){
		if(pipeline_controls.WB_1==false){
			if(pipeline_controls.WB_branch){
				if(_EX_latch.branch_satisfied == true && _EX_latch.branch_instruction == true){
					_EX_latch.branch_instruction = false;
					_EX_latch.branch_satisfied = false;
					pipeline_controls.WB_branch = false;
					PCcurr = _EX_latch.label_address;
					if(pipeline_controls.ALU_Stage_==true){
						pipeline_controls.ALU_Stage_ = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					if(pipeline_controls.RR_==true){
						pipeline_controls.RR_ = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					if(pipeline_controls.ID_Stage_2 == true){
						pipeline_controls.ID_Stage_2 = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					if(pipeline_controls.ID_Stage_1 == true){
						pipeline_controls.ID_Stage_1 = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					if(pipeline_controls.IF_Stage_2 == true){
						pipeline_controls.ID_Stage_2 = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					PCcurr=_EX_latch.label_address;
				if(pipeline_controls.IF_Stage_1 == false){
					pipeline_controls.IF_Stage_1 = true;
					pipeline_controls.count=pipeline_controls.count-1;
				}
								std::cout<<"hhhhhhhhhhhhhh"<<PCcurr<<std::endl;
			std::cout<<pipeline_controls.IF_Stage_1<<pipeline_controls.IF_Stage_2<<pipeline_controls.ID_Stage_1 <<pipeline_controls.ID_Stage_2<<pipeline_controls.RR_<< pipeline_controls.ALU_Stage_<<pipeline_controls.MEM_Stage_1<<pipeline_controls.MEM_Stage_2<<pipeline_controls.WB_1<<std::endl;
				return 0;

				}

				pipeline_controls.WB_branch=false;
				

			}
			PCcurr= PCcurr+1;
			return 0;
		}
		if(pipeline_controls.WB_branch){
				pipeline_controls.WB_branch=false;
				if(_EX_latch.branch_satisfied == true && _EX_latch.branch_instruction == true){
					
					PCcurr = _EX_latch.label_address-1;
					if(pipeline_controls.ALU_Stage_==true){
						pipeline_controls.ALU_Stage_ = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					if(pipeline_controls.RR_==true){
						pipeline_controls.RR_ = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					if(pipeline_controls.ID_Stage_2 == true){
						pipeline_controls.ID_Stage_2 = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					if(pipeline_controls.ID_Stage_1 == true){
						pipeline_controls.ID_Stage_1 = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					if(pipeline_controls.IF_Stage_2 == true){
						pipeline_controls.ID_Stage_2 = false;
						pipeline_controls.count = pipeline_controls.count +1;
					}
					PCcurr=_EX_latch.label_address;
					if(!pipeline_controls.IF_Stage_1){
						pipeline_controls.IF_Stage_1 = true;
						pipeline_controls.count=pipeline_controls.count-1;
					}
				}
				_EX_latch.branch_instruction = false;
				_EX_latch.branch_satisfied = false;
				pipeline_controls.WB_branch = false;
				pipeline_controls.stage_7 = false;
		}
		
		if(pipeline_controls.stage_9 == true){
			pipeline_controls.stage_9 = false;
			if(_MEM2_latch.reg_write){
				registers[registerMap[_MEM2_latch.dest_register]] = _MEM2_latch.data;
			}
			pipeline_controls.WB_1 = false;
		}else if(pipeline_controls.stage_7 ==true){
			pipeline_controls.stage_7 = false;
			if(_EX_latch.reg_write==true){
				registers[registerMap[_EX_latch.dest_register]] = _EX_latch.data;
			}
		}
		pipeline_controls.WB_1 = false;
		pipeline_controls.count = pipeline_controls.count +1;
		PCcurr = PCcurr+1;
		return 0;
		
	}


	void execute79pipeline(){
		pipeline_controls.ALU_Stage_ = false;
		pipeline_controls.ID_Stage_1 = false;
		pipeline_controls.ID_Stage_2 = false;
		pipeline_controls.IF_Stage_1 = true;
		pipeline_controls.IF_Stage_2 = false;
		pipeline_controls.MEM_Stage_1 = false;
		pipeline_controls.MEM_Stage_2 = false;
		pipeline_controls.RR_ = false;
		pipeline_controls.stage_7 = false;
		pipeline_controls.stage_9 = false;
		pipeline_controls.WB_1 = false;
		if (commands.size() >= MAX / 4)
		{
			handleExit(MEMORY_ERROR, 0);
			return;
		}

		int clockCycles = 0;
		PCcurr = -1;
		pipeline_controls.count = 8;
		while(true){
			++clockCycles;
			if(true){
			WB(clockCycles);
			MEM_Stage2(clockCycles);
			MEM_stage1(clockCycles);
			ALU_Stage(clockCycles);
			RR(clockCycles);
			ID_Stage2(clockCycles);
			ID_Stage1(_IF2_latch.command,clockCycles);
			IF_Stage2(clockCycles);
			IF_Stage1(PCcurr,clockCycles);
			printRegisters(clockCycles);
			std::cout<<pipeline_controls.count<<"   "<<PCcurr<<std::endl;
			if(pipeline_controls.count == 9){
				break;
			}
			std::cout<<pipeline_controls.IF_Stage_1<<pipeline_controls.IF_Stage_2<<pipeline_controls.ID_Stage_1 <<pipeline_controls.ID_Stage_2<<pipeline_controls.RR_<< pipeline_controls.ALU_Stage_<<pipeline_controls.MEM_Stage_1<<pipeline_controls.MEM_Stage_2<<pipeline_controls.WB_1<<std::endl;
			}
			
		}


	}


	// print the register data in hexadecimal
	void printRegisters(int clockCycle)
	{
		for (int i = 0; i < 32; ++i)
			std::cout << registers[i] << ' ';
		std::cout <<'\n';
		std::cout << memoryDelta.size() << ' ';
		if(memoryDelta.size()==0){
			std::cout<<'\n';
		}
		for (auto &p : memoryDelta)
			std::cout << p.first << ' ' << p.second <<'\n';
		memoryDelta.clear();
	}
};

#endif