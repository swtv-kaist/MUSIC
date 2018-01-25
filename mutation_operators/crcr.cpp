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

	SourceLocation start_loc = e->getLocStart();
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
	SourceLocation start_loc = e->getLocStart();
	SourceLocation end_loc = GetEndLocOfExpr(e, context->comp_inst_);

	if (ExprIsIntegral(context->comp_inst_, e))
	{
		for (auto num: range_integral_)
		{
			string mutated_token{"(" + num + ")"};

			if (num.compare("min") == 0)
				mutated_token = GetMinValue(e->getType());

			if (num.compare("max") == 0)
				mutated_token = GetMaxValue(e->getType());

			context->mutant_database_.AddMutantEntry(
					name_, start_loc, end_loc, token, mutated_token, 
				  context->getStmtContext().getProteumStyleLineNum());
		}

		return;
	}

	if (ExprIsFloat(e))
	{
		for (auto num: range_float_)
		{
			string mutated_token{"(" + num + ")"};

			if (num.compare("min") == 0)
				mutated_token = GetMinValue(e->getType());

			if (num.compare("max") == 0)
				mutated_token = GetMaxValue(e->getType());

			context->mutant_database_.AddMutantEntry(
					name_, start_loc, end_loc, token, mutated_token, 
					context->getStmtContext().getProteumStyleLineNum());
		}

		return;
	}
}

string CRCR::GetMaxValue(QualType qualtype)
{
	string ret;
	const Type *type = qualtype.getCanonicalType().getTypePtr();
	string type_string = qualtype.getAsString();

	if (type_string.find(string("float")) != string::npos)
		ret = "(3.4E38)";
	else if (type_string.find(string("double")) != string::npos)
		ret = "(1.7E308)";
	else if (type_string.find(string("char")) != string::npos)
		if (type->isSignedIntegerType())
			ret = "(127)";
		else
			ret = "(255)";
	else if (type_string.find(string("short")) != string::npos)
		if (type->isSignedIntegerType())
			ret = "(32767)";
		else
			ret = "(65535)";
	else if (type_string.find(string("long")) != string::npos)
		if (type->isSignedIntegerType())
			ret = "(9223372036854775807)";
		else
			ret = "(18446744073709551615)";
	else	// int type	
	{ 
		if (type->isSignedIntegerType())
			ret = "(2147483647)";
		else
			ret = "(4294967295)";
	}

	return ret;
}

string CRCR::GetMinValue(QualType qualtype)
{
	string ret;
	const Type *type = qualtype.getCanonicalType().getTypePtr();
	string type_string = qualtype.getAsString();

	if (type_string.find(string("float")) != string::npos)
		ret = "(-3.4E38)";
	else if (type_string.find(string("double")) != string::npos)
		ret = "(-1.7E308)";
	else if (type_string.find(string("char")) != string::npos)
		if (type->isSignedIntegerType())
			ret = "(-128)";
		else
			ret = "(0)";
	else if (type_string.find(string("short")) != string::npos)
		if (type->isSignedIntegerType())
			ret = "(-32768)";
		else
			ret = "(0)";
	else if (type_string.find(string("long")) != string::npos)
		if (type->isSignedIntegerType())
			ret = "(-9223372036854775808)";
		else
			ret = "(0)";
	else	// int type	
	{ 
		if (type->isSignedIntegerType())
			ret = "(-2147483648)";
		else
			ret = "(0)";
	}
	
	return ret;
}