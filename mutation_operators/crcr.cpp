#include "../music_utility.h"
#include "crcr.h"

bool CRCR::ValidateDomain(const std::set<std::string> &domain)
{
	return domain.empty();
}

bool CRCR::ValidateRange(const std::set<std::string> &range)
{
	for (auto num: range)
		if (!StringIsANumber(num))
			return false;

	return true;
}

void CRCR::setRange(std::set<std::string> &range)
{
	range_integral_ = {"0", "1", "-1", "min", "max"};
  range_float_ = {"0.0", "1.0", "-1.0", "min", "max"};

  for (auto num: range)
  	if (NumIsFloat(num))
  		range_float_.insert(num);
  	else
  		range_integral_.insert(num);
}


// Return True if the mutant operator can mutate this expression
bool CRCR::IsMutationTarget(clang::Expr *e, MusicContext *context)
{
	if (!ExprIsScalarReference(e))
		return false;

	SourceLocation start_loc = e->getBeginLoc();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	StmtContext &stmt_context = context->getStmtContext();

	return context->IsRangeInMutationRange(SourceRange(start_loc, end_loc)) &&
				 !stmt_context.IsInEnumDecl() &&
				 !stmt_context.IsInArrayDeclSize() &&
				 !stmt_context.IsInLhsOfAssignmentRange(e) &&
				 !stmt_context.IsInAddressOpRange(e) &&
				 !stmt_context.IsInUnaryIncrementDecrementRange(e);
}

void CRCR::Mutate(clang::Expr *e, MusicContext *context)
{
	Rewriter rewriter;
	SourceManager &src_mgr = context->comp_inst_->getSourceManager();
	rewriter.setSourceMgr(src_mgr, context->comp_inst_->getLangOpts());

	string token{ConvertToString(e, context->comp_inst_->getLangOpts())};
	SourceLocation start_loc = e->getBeginLoc();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	if (ExprIsIntegral(context->comp_inst_, e))
	{
		for (auto num: range_integral_)
		{
			string mutated_token{"(" + num + ")"};

			if (num.compare("min") == 0)
			{
				mutated_token = GetMinValue(e->getType());
				context->mutant_database_.AddMutantEntry(context->getStmtContext(),
					name_, start_loc, end_loc, token, mutated_token, 
				  context->getStmtContext().getProteumStyleLineNum(), 
				  "min");
				continue;
			}

			if (num.compare("max") == 0)
			{
				mutated_token = GetMaxValue(e->getType());
				context->mutant_database_.AddMutantEntry(context->getStmtContext(),
					name_, start_loc, end_loc, token, mutated_token, 
				  context->getStmtContext().getProteumStyleLineNum(), 
				  "max");
				continue;
			}

			context->mutant_database_.AddMutantEntry(context->getStmtContext(),
					name_, start_loc, end_loc, token, mutated_token, 
				  context->getStmtContext().getProteumStyleLineNum(), num);
		}

		return;
	}

	if (ExprIsFloat(e))
	{
		for (auto num: range_float_)
		{
			string mutated_token{"(" + num + ")"};
			string info{""};

			if (num.compare("min") == 0)
			{
				mutated_token = GetMinValue(e->getType());
				info = "min";
			}

			if (num.compare("max") == 0)
			{
				mutated_token = GetMaxValue(e->getType());
				info = "max";				
			}

			if (num.compare("0.0") == 0)
				info = "0";
			if (num.compare("1.0") == 0)
				info = "1";
			if (num.compare("-1.0") == 0)
				info = "-1";

			context->mutant_database_.AddMutantEntry(context->getStmtContext(),
					name_, start_loc, end_loc, token, mutated_token, 
					context->getStmtContext().getProteumStyleLineNum(), info);
		}

		return;
	}
}
