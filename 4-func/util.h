#pragma once

#include <istream>
#include <iostream>
#include <limits>
#include <type_traits>
#include <utility>

template<typename T>
struct imm {};

struct reg {};

struct label {};

template<typename ...Args>
struct args {};

template<typename Arg, typename ...Args>
struct args<Arg, Args...> : args<Args...> {
  using type = Arg;
};

template<typename ReaderT, typename ...Args>
struct args_storage;

template<typename ReaderT, typename Arg>
using arg_repr_t = decltype(std::declval<ReaderT>().read(Arg{}));

template <typename ReaderT, typename Arg, typename... Args>
struct args_storage<ReaderT, Arg, Args...>
    : std::tuple<arg_repr_t<ReaderT, Arg>, arg_repr_t<ReaderT, Args>...> {
  using ArgsTupleT = std::tuple<Arg, Args...>;
  using BaseT =
      std::tuple<arg_repr_t<ReaderT, Arg>, arg_repr_t<ReaderT, Args>...>;
  template<typename CtxT>
  using FuncT = void(CtxT &, arg_repr_t<ReaderT, Arg> &&, arg_repr_t<ReaderT, Args> &&...);

  args_storage() = default;
  args_storage(const BaseT &base) : BaseT(base) {}
  args_storage(BaseT &&base) : BaseT(std::move(base)) {}

  BaseT &tuple() & {
    return *this;
  }

  BaseT tuple() && {
    return *this;
  }

  static args_storage read(ReaderT &reader) {
    using HeadT = arg_repr_t<ReaderT, Arg>;
    return std::tuple_cat(std::tuple<HeadT>(reader.read(Arg{})),
                          args_storage<ReaderT, Args...>::read(reader).tuple());
  }

  template<typename WriterT, size_t ...I>
  void write(WriterT &writer, std::index_sequence<I...>) {
    (..., write<I>(writer));
  }

  template<size_t I, typename WriterT>
  void write(WriterT &writer) {
    writer.write(std::get<I>(*this), std::tuple_element_t<I, ArgsTupleT>{});
  }

  template<typename WriterT>
  void write(WriterT &writer) {
    write(writer, std::make_index_sequence<sizeof...(Args) + 1>());
  }
};

template<typename ReaderT>
struct args_storage<ReaderT> : std::tuple<> {
  using BaseT = std::tuple<>;
  template<typename CtxT>
  using FuncT = void(CtxT &);

  args_storage() = default;
  args_storage(const BaseT &base) : BaseT(base) {}
  args_storage(BaseT &&base) : BaseT(std::move(base)) {}

  BaseT &tuple() {
    return *this;
  }

  static args_storage read(ReaderT &reader) {
    return {};
  }

  template<typename WriterT>
  void write(WriterT &writer) {}
};

template<typename ReaderT, typename ...Args>
auto read_args_storage(ReaderT &reader, args<Args...>) {
  return args_storage<ReaderT, Args...>::read(reader);
}

template <typename T> struct argument_type;
template <typename T, typename... U>
struct argument_type<T(U...)> : std::type_identity<args<U...>> {};
