#pragma once

#include <vector>
#include <unordered_map>
#include <string>

#include "AST.h"

template<typename T>
class ContextVisitor : public AST::Visitor {
public:
  std::vector<std::unordered_map<std::string, T>> Contexts;

  ContextVisitor(std::unordered_map<std::string, T> GlobalCtx) : Contexts({std::move(GlobalCtx)}) {}

  void enterContext() {
    Contexts.emplace_back();
  }

  void exitContext() {
    Contexts.pop_back();
  }

  void registerName(const std::string &Name, T Value) {
    Contexts.back()[Name] = std::move(Value);
  }

  T *resolveName(const std::string &Name) {
    for (auto CtxIt = Contexts.rbegin(); CtxIt != Contexts.rend(); ++CtxIt) {
      auto &Ctx = *CtxIt;
      if (auto it = Ctx.find(Name); it != Ctx.end())
        return &it->second;
    }
    return nullptr;
  }
};
