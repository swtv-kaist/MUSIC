#include "expr_mutant_operator.h"

bool ExprMutantOperator::CanMutate(clang::Stmt *s, ComutContext *context) final {
	return false;
}

void Mutate(clang::Stmt *s, ComutContext *context) final {}