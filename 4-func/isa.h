#pragma once

#include "util.h"
#include "cpu.h"
#include <cstdint>
#include <functional>
#include <tuple>
#include <type_traits>

template <std::size_t N> struct MakeArray {
  std::array<char, N> data;

  template <std::size_t... Is>
  constexpr MakeArray(const char (&arr)[N],
                      std::integer_sequence<std::size_t, Is...>)
      : data{arr[Is]...} {}

  constexpr MakeArray(char const (&arr)[N])
      : MakeArray(arr, std::make_integer_sequence<std::size_t, N>()) {}
};

template <MakeArray A> constexpr auto operator"" _n() { return A.data; }

template<uint8_t OpCode, typename = void>
struct inst_checker {
  friend auto adl_lookup(inst_checker<OpCode>);
};

template<typename Derived, uint8_t OpCode, auto Name, typename ...Args>
struct inst_base {
  static constexpr uint8_t op_code = OpCode;
  static constexpr const char *name = Name.data();
  using args_t = args<Args...>;

  friend auto adl_lookup(inst_checker<OpCode>) {
    return Derived{};
  }
};

// begin
// control flow
template<typename Derived, uint8_t OpCode, auto Name, typename ...Args>
struct inst_control_flow_base : inst_base<Derived, OpCode, Name, Args...> {};

template<typename Derived, typename PredT, uint8_t OpCode, auto Name, typename ...Args>
struct inst_jmp_pred : inst_control_flow_base<Derived, OpCode, Name, Args...> {
  static void exec(auto &ctx, const auto &a, const auto &l) { if (PredT{}(a)) ctx.jump(l); }
};
template<typename Derived, typename PredT, uint8_t OpCode, auto Name, typename ...Args>
struct inst_jmp_cmp : inst_control_flow_base<Derived, OpCode, Name, Args...> {
  static void exec(auto &ctx, const auto &a, const auto &b, const auto &l) { if (PredT{}(a, b)) ctx.jump(l); }
};

struct inst_exit : inst_control_flow_base<inst_exit, 0x01, "EXIT"_n> {
  static void exec(auto &ctx) { ctx.stop(); }
};
struct inst_jmp : inst_control_flow_base<inst_jmp, 0x02, "JMP"_n, label> {
  static void exec(auto &ctx, const auto &l) { ctx.jump(l); }
};
struct inst_jmpz   : inst_jmp_pred<inst_jmpz,   std::logical_not<>, 0x03, "JMPZ"_n,   reg, label> {};
struct inst_jmpnz  : inst_jmp_pred<inst_jmpnz,  std::identity,      0x04, "JMPNZ"_n,  reg, label> {};
struct inst_jmpeqi : inst_jmp_cmp <inst_jmpeqi, std::equal_to<>,    0x05, "JMPEQi"_n, reg, imm<int32_t>, label> {};
struct inst_jmplti : inst_jmp_cmp <inst_jmplti, std::less<>,        0x06, "JMPLTi"_n, reg, imm<int32_t>, label> {};
struct inst_jmpgti : inst_jmp_cmp <inst_jmpgti, std::greater<>,     0x07, "JMPGTi"_n, reg, imm<int32_t>, label> {};

// debug
struct inst_write : inst_base<inst_write, 0x0F, "WRITE"_n, reg> {
  static void exec(auto &ctx, const auto &a) { std::cout << a << '\n'; }
};

// arithmetic
template<typename Derived, uint8_t OpCode, auto Name, typename ...Args>
struct inst_set_base : inst_base<Derived, OpCode, Name, Args...> {
  static void exec(auto &ctx, auto &a, const auto &b) { a = b; }
};
template<typename Derived, typename Op, uint8_t OpCode, auto Name, typename ...Args>
struct inst_binop_base : inst_base<Derived, OpCode, Name, Args...> {
  static void exec(auto &ctx, auto &a, const auto &b, const auto &c) { a = Op{}(b, c); }
};
template<typename Derived, uint8_t OpCode, auto Name, typename ...Args>
struct inst_muladd_base : inst_base<Derived, OpCode, Name, Args...> {
  static void exec(auto &ctx, auto &a, const auto &b, const auto &c, const auto &d) { a = b + c * d; }
};

struct binary_shift_left {
  auto operator()(const auto &lhs, const auto &rhs) {
    return lhs << rhs;
  }
};

struct inst_seti : inst_set_base<      inst_seti,                    0x10, "SETi"_n,    reg, imm<int32_t>> {};
struct inst_addi : inst_binop_base<    inst_addi, std::plus<>,       0x11, "ADDi"_n,    reg, reg, imm<int32_t>> {};
struct inst_muli : inst_binop_base<    inst_muli, std::multiplies<>, 0x12, "MULi"_n,    reg, reg, imm<int32_t>> {};
struct inst_divi : inst_binop_base<    inst_divi, std::divides<>,    0x13, "DIVi"_n,    reg, reg, imm<int32_t>> {};
struct inst_remi : inst_binop_base<    inst_remi, std::modulus<>,    0x14, "REMi"_n,    reg, reg, imm<int32_t>> {};
struct inst_shli : inst_binop_base<    inst_shli, binary_shift_left, 0x15, "SHLi"_n,    reg, reg, imm<int32_t>> {};
struct inst_ceqi : inst_binop_base<    inst_ceqi, std::equal_to<>,   0x16, "CEQi"_n,    reg, reg, imm<int32_t>> {};
struct inst_clti : inst_binop_base<    inst_clti, std::less<>,       0x17, "CLTi"_n,    reg, reg, imm<int32_t>> {};
struct inst_cgti : inst_binop_base<    inst_cgti, std::greater<>,    0x18, "CGTi"_n,    reg, reg, imm<int32_t>> {};
struct inst_muladdi : inst_muladd_base<inst_muladdi,                 0x19, "MULADDi"_n, reg, reg, reg, imm<int32_t>> {};

struct inst_set : inst_set_base<      inst_set,                    0x20, "SET"_n,    reg, reg> {};
struct inst_add : inst_binop_base<    inst_add, std::plus<>,       0x21, "ADD"_n,    reg, reg, reg> {};
struct inst_mul : inst_binop_base<    inst_mul, std::multiplies<>, 0x22, "MUL"_n,    reg, reg, reg> {};
struct inst_div : inst_binop_base<    inst_div, std::divides<>,    0x23, "DIV"_n,    reg, reg, reg> {};
struct inst_rem : inst_binop_base<    inst_rem, std::modulus<>,    0x24, "REM"_n,    reg, reg, reg> {};
struct inst_shl : inst_binop_base<    inst_shl, binary_shift_left, 0x25, "SHL"_n,    reg, reg, reg> {};
struct inst_ceq : inst_binop_base<    inst_ceq, std::equal_to<>,   0x26, "CEQ"_n,    reg, reg, reg> {};
struct inst_clt : inst_binop_base<    inst_clt, std::less<>,       0x27, "CLT"_n,    reg, reg, reg> {};
struct inst_cgt : inst_binop_base<    inst_cgt, std::greater<>,    0x28, "CGT"_n,    reg, reg, reg> {};
struct inst_muladd : inst_muladd_base<inst_muladd,                 0x29, "MULADD"_n, reg, reg, reg, reg> {};

// stack & memory
struct inst_alloca : inst_base<inst_alloca, 0x30, "ALLOCA"_n, reg, imm<uint32_t>> {
  static void exec(auto &ctx, auto &a, const auto &b) { a = ctx.alloc(b); }
};
template<typename Derived, uint8_t OpCode, auto Name, typename ...Args>
struct inst_store_base : inst_base<Derived, OpCode, Name, Args...> {
  static void exec(auto &ctx, const auto &a, const auto &b) { ctx.deref(a) = b; }
};

struct inst_storei : inst_store_base<inst_storei, 0x32, "STOREi"_n, reg, imm<int32_t>> {};
struct inst_store : inst_store_base<inst_store, 0x33, "STORE"_n, reg, reg> {};
struct inst_load : inst_base<inst_load, 0x34, "LOAD"_n, reg, reg> {
  static void exec(auto &ctx, auto &a, const auto &b) { a = ctx.deref(b); }
};

struct inst_sim_rand : inst_base<inst_sim_rand, 0x40, "SIM_RAND"_n, reg> {
  static void exec(auto &ctx, auto &a) { a = ctx.simRand(); }
};
struct inst_sim_flush : inst_base<inst_sim_flush, 0x41, "SIM_FLUSH"_n> {
  static void exec(auto &ctx) { ctx.simFlush(); }
};
struct inst_sim_put_pixel : inst_base<inst_sim_put_pixel, 0x42, "SIM_PUT_PIXEL"_n, reg, reg, reg> {
  static void exec(auto &ctx, const auto &x, const auto &y, const auto &c) { ctx.simPutPixel(x, y, c); }
};
struct inst_sim_clear : inst_base<inst_sim_clear, 0x43, "SIM_CLEAR"_n, reg> {
  static void exec(auto &ctx, const auto &c) { ctx.simClear(c); }
};

// end

template<uint8_t OpCode, typename = void>
struct op_code2inst {
  using type = void;
};

template<uint8_t OpCode>
struct op_code2inst<OpCode, std::void_t<decltype(adl_lookup(inst_checker<OpCode>{}))>> {
  using type = decltype(adl_lookup(inst_checker<OpCode>{}));
};

template<uint8_t OpCode>
using op_code2inst_t = op_code2inst<OpCode>::type;

template<uint8_t OpCode>
struct inst_enum {
  constexpr static auto run() {
    using cur_type = op_code2inst_t<OpCode>;
    if constexpr (std::is_same_v<cur_type, void>) {
      return inst_enum<OpCode - 1>::run();
    } else {
      return std::tuple_cat(inst_enum<OpCode - 1>::run(), std::make_tuple(cur_type{}));
    }
  }
};

template<>
struct inst_enum<0> {
  constexpr static std::tuple<> run() { return {}; }
};

using inst_enum_t = decltype(inst_enum<255>::run());

template<typename R = void, uint8_t OpCode = 255>
R find_inst(auto &&Pred, auto &&Action) {
  if constexpr (std::is_same_v<op_code2inst_t<OpCode>, void>) {
    if constexpr (OpCode > 0)
      return find_inst<R, OpCode - 1>(Pred, Action);
    else if constexpr (!std::is_same_v<R, void>)
      return R{};
  } else {
    if (Pred(op_code2inst_t<OpCode>{}))
      return Action(op_code2inst_t<OpCode>{});
    else
      return find_inst<R, OpCode - 1>(Pred, Action);
  }
}
