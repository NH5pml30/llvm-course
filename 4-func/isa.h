#define ARG(i) (std::get<i>(arg_values))

#ifndef ISA
#define ISA(opcode, name, exec, args)
#endif

// control flow
ISA(0x01, EXIT, { cpu->stop(); }, args<>)

ISA(0x02, JMP, { cpu->next_PC = ARG(0); }, args<label>)

ISA(0x03, JMPnz, { if (cpu->reg_file[ARG(0)]) cpu->next_PC = ARG(1); }, (args<reg, label>))

ISA(0x0F, WRITE, { std::cout << cpu->reg_file[ARG(0)] << '\n'; }, args<reg>)

// arithmetic
ISA(0x10, SETi, { cpu->reg_file[ARG(0)] = ARG(1); }, (args<reg, imm<int32_t>>))

ISA(0x11, SET, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)]; }, (args<reg, reg>))

ISA(0x12, ADDi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] + ARG(2); }, (args<reg, reg, imm<int32_t>>))

ISA(0x13, MULi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] * ARG(2); }, (args<reg, reg, imm<int32_t>>))

ISA(0x14, DIVi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] / ARG(2); }, (args<reg, reg, imm<int32_t>>))

ISA(0x15, REMi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] % ARG(2); }, (args<reg, reg, imm<int32_t>>))

ISA(0x16, SHLi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] << ARG(2); }, (args<reg, reg, imm<int32_t>>))

ISA(0x17, CEQi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] == ARG(2); }, (args<reg, reg, imm<int32_t>>))

ISA(0x18, CLTi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] < ARG(2); }, (args<reg, reg, imm<int32_t>>))

ISA(0x19, CGTi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] > ARG(2); }, (args<reg, reg, imm<int32_t>>))

// stack & memory
ISA(0x20, ALLOCA, { cpu->reg_file[ARG(0)] = cpu->stack -= ARG(1); }, (args<reg, imm<uint32_t>>))

ISA(0x22, STORE, { *(uint32_t *)(cpu->stack_mem + cpu->reg_file[ARG(0)]) = cpu->reg_file[ARG(1)]; }, (args<reg, reg>))

ISA(0x23, STOREi, { *(uint32_t *)(cpu->stack_mem + cpu->reg_file[ARG(0)]) = ARG(1); }, (args<reg, imm<uint32_t>>))

ISA(0x24, LOAD, { cpu->reg_file[ARG(0)] = *(uint32_t *)(cpu->stack_mem + cpu->reg_file[ARG(1)]); }, (args<reg, reg>))

// simulator functions
ISA(0x30, SIM_RAND, { cpu->reg_file[ARG(0)] = simRand(); }, args<reg>)

ISA(0x31, SIM_FLUSH, { simFlush(); }, args<>)

ISA(0x32, SIM_PUT_PIXEL, { simPutPixel(cpu->reg_file[ARG(0)], cpu->reg_file[ARG(1)], cpu->reg_file[ARG(2)]); }, (args<reg, reg, reg>))

ISA(0x33, SIM_CLEAR, { simClear(cpu->reg_file[ARG(0)]); }, args<reg>)
