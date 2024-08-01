#include "Evaluator.h"
#include <any>

using namespace xanadu::Tokens;

namespace xanadu {
std::any Interpreter::evaluate(Expr *expr) { return expr->accept(this); }

std::any Interpreter::visitLiteralExpr(LiteralExpr *Expr) {
  return Expr->value;
}

std::any Interpreter::visitGroupingExpr(GroupingExpr *Expr) {
  return evaluate(Expr->expression);
}

std::any Interpreter::visitUnaryExpr(UnaryExpr *Expr) {
  auto right = evaluate(Expr->right);

  switch (Expr->Operator.getType()) {
  case BANG:
    return !isTruthy(right);
  case MINUS:
    return right;
  default:
    break;
  }

  return nullptr;
}

} // namespace xanadu
