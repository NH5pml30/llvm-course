#define ARG(i) (std::get<i>(arg_values))

#ifndef ISA
#define ISA(opcode, name, exec, args)
#endif

// control flow
ISA(0x01, EXIT, { cpu->stop(); }, args<>)

ISA(0x02, JMP, { cpu->next_PC = ARG(0); }, args<label>)

ISA(0x03, JMPZ, { if (!cpu->reg_file[ARG(0)]) cpu->next_PC = ARG(1); }, (args<reg, label>))

ISA(0x04, JMPNZ, { if (cpu->reg_file[ARG(0)]) cpu->next_PC = ARG(1); }, (args<reg, label>))

ISA(0x05, JMPEQi, { if (cpu->reg_file[ARG(0)] == ARG(1)) cpu->next_PC = ARG(2); }, (args<reg, imm<int32_t>, label>))

ISA(0x0F, WRITE, { std::cout << cpu->reg_file[ARG(0)] << '\n'; }, args<reg>)

// arithmetic
ISA(0x10, SETi, { cpu->reg_file[ARG(0)] = ARG(1); }, (args<reg, imm<int32_t>>))
ISA(0x11, ADDi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] + ARG(2); }, (args<reg, reg, imm<int32_t>>))
ISA(0x12, MULi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] * ARG(2); }, (args<reg, reg, imm<int32_t>>))
ISA(0x13, DIVi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] / ARG(2); }, (args<reg, reg, imm<int32_t>>))
ISA(0x14, REMi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] % ARG(2); }, (args<reg, reg, imm<int32_t>>))
ISA(0x15, SHLi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] << ARG(2); }, (args<reg, reg, imm<int32_t>>))
ISA(0x16, CEQi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] == ARG(2); }, (args<reg, reg, imm<int32_t>>))
ISA(0x17, CLTi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] < ARG(2); }, (args<reg, reg, imm<int32_t>>))
ISA(0x18, CGTi, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] > ARG(2); }, (args<reg, reg, imm<int32_t>>))

ISA(0x20, SET, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)]; }, (args<reg, reg>))
ISA(0x21, ADD, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] + cpu->reg_file[ARG(2)]; }, (args<reg, reg, reg>))
ISA(0x22, MUL, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] * cpu->reg_file[ARG(2)]; }, (args<reg, reg, reg>))
ISA(0x23, DIV, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] / cpu->reg_file[ARG(2)]; }, (args<reg, reg, reg>))
ISA(0x24, REM, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] % cpu->reg_file[ARG(2)]; }, (args<reg, reg, reg>))
ISA(0x25, SHL, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] << cpu->reg_file[ARG(2)]; }, (args<reg, reg, reg>))
ISA(0x26, CEQ, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] == cpu->reg_file[ARG(2)]; }, (args<reg, reg, reg>))
ISA(0x27, CLT, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] < cpu->reg_file[ARG(2)]; }, (args<reg, reg, reg>))
ISA(0x28, CGT, { cpu->reg_file[ARG(0)] = cpu->reg_file[ARG(1)] > cpu->reg_file[ARG(2)]; }, (args<reg, reg, reg>))

// stack & memory
ISA(0x30, ALLOCA, { cpu->reg_file[ARG(0)] = cpu->stack -= ARG(1); }, (args<reg, imm<uint32_t>>))

ISA(0x32, STORE, {
    //std::cout << "store " << cpu->reg_file[ARG(1)] << " @" << cpu->reg_file[ARG(0)] << "\n";
    *(uint32_t *)(cpu->stack_mem + cpu->reg_file[ARG(0)]) = cpu->reg_file[ARG(1)];
  }, (args<reg, reg>))

ISA(0x33, STOREi, {
    //std::cout << "store " << ARG(1) << " @" << cpu->reg_file[ARG(0)] << "\n";
    *(uint32_t *)(cpu->stack_mem + cpu->reg_file[ARG(0)]) = ARG(1);
  }, (args<reg, imm<uint32_t>>))

ISA(0x34, LOAD, {
    //std::cout << "load from @" << cpu->reg_file[ARG(1)] << "\n";
    cpu->reg_file[ARG(0)] = *(uint32_t *)(cpu->stack_mem + cpu->reg_file[ARG(1)]);
  }, (args<reg, reg>))

// simulator functions
ISA(0x40, SIM_RAND, { cpu->reg_file[ARG(0)] = simRand(); }, args<reg>)

ISA(0x41, SIM_FLUSH, { simFlush(); }, args<>)

ISA(0x42, SIM_PUT_PIXEL, { simPutPixel(cpu->reg_file[ARG(0)], cpu->reg_file[ARG(1)], cpu->reg_file[ARG(2)]); }, (args<reg, reg, reg>))

ISA(0x43, SIM_CLEAR, { simClear(cpu->reg_file[ARG(0)]); }, args<reg>)
