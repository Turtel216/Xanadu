#include "../Ast/Expr.h"

namespace xanadu {

class Interpreter : public ExprVisitor {
private:
  std::any evaluate(Expr *expr);
  bool isTruthy(std::any expr);

public:
  std::any visitLiteralExpr(LiteralExpr *Expr) override;
  std::any visitGroupingExpr(GroupingExpr *Expr) override;
  std::any visitUnaryExpr(UnaryExpr *Expr) override;
};

} // namespace xanadu
