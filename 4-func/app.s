app:
  ALLOCA r0 2400

init_header:
  JMPEQi r1 200 init_exit

  MULADDi r2 r0 r1 12

  SIM_RAND r3
  REMi r3 r3 200
  ADDi r3 r3 -100
  STORE r2 r3

  SIM_RAND r3
  REMi r3 r3 200
  ADDi r3 r3 -100
  ADDi r2 r2 4
  STORE r2 r3

  SIM_RAND r3
  REMi r3 r3 100
  ADDi r2 r2 4
  STORE r2 r3
  ADDi r1 r1 1
  JMP init_header

init_exit:
  SETi r1 0
  SIM_CLEAR r1

update_header:
  JMPEQi r1 200 update_exit

  MULADDi r2 r0 r1 12
  SET r3 r2
  ADDi r3 r3 8
  LOAD r4 r3
  ADDi r4 r4 -1
  STORE r3 r4
  CLTi r5 r4 1
  JMPZ r5 compute

  SIM_RAND r4
  REMi r4 r4 200
  ADDi r4 r4 -100
  SET r6 r2
  STORE r6 r4

  SIM_RAND r4
  REMi r4 r4 200
  ADDi r4 r4 -100
  ADDi r6 r6 4
  STORE r6 r4

  ADDi r6 r6 4
  SETi r4 100
  STORE r6 r4

compute:
  LOAD r5 r2
  SHLi r5 r5 8
  DIV r5 r5 r4
  ADDi r5 r5 256

  ADDi r6 r2 4
  LOAD r6 r6
  SHLi r6 r6 7
  DIV r6 r6 r4
  ADDi r6 r6 128

  JMPLTi r5 0 update_end
  JMPGTi r5 511 update_end
  JMPLTi r6 0 update_end
  JMPGTi r6 255 update_end

  SETi r7 -1
  SIM_PUT_PIXEL r5 r6 r7

update_end:
  ADDi r1 r1 1
  JMP update_header

update_exit:
  SIM_FLUSH
  JMP init_exit
