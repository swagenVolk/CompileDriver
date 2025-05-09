/*
 * RunTimeInterpreter.h
 *
 *  Created on: Jan 4, 2024
 *      Author: Mike Volk
 */

#ifndef RUNTIMEINTERPRETER_H_
#define RUNTIMEINTERPRETER_H_

#include "InterpretedFileReader.h"
#include "Token.h"
#include <cstdint>
#include <memory>
#include "CompileExecTerms.h"
#include "Utilities.h"
#include "StackOfScopes.h"
#include "UserMessages.h"

class RunTimeInterpreter {
public:
	RunTimeInterpreter();
	RunTimeInterpreter(CompileExecTerms & execTerms, std::shared_ptr<StackOfScopes> inVarNameSpace
		, std::wstring userSrcFileName, std::shared_ptr<UserMessages> userMessages, logLvlEnum logLvl);
	RunTimeInterpreter(std::string interpretedFileName, std::wstring userSrcFileName
		, std::shared_ptr<StackOfScopes> inVarNameSpace,  std::shared_ptr<UserMessages> userMessages
		, logLvlEnum logLvl);

	virtual ~RunTimeInterpreter();
	// TODO: Should I make this static?
	int resolveFlatExpr(std::vector<Token> & flat_expr_tkns);
	int execRootScope();


protected:

private:
  std::shared_ptr <Token> one_tkn;
  std::shared_ptr <Token> zero_tkn;
  CompileExecTerms exec_terms;
	std::wstring this_src_file;
	Utilities util;
	Token scratch_tkn;
	std::shared_ptr<StackOfScopes> scope_name_space;
	std::shared_ptr<UserMessages> user_messages;
	std::wstring usr_src_file_name;
	InterpreterModesType usage_mode;
	InterpretedFileReader file_reader;
	int failed_on_src_line;
	logLvlEnum log_level;
	bool is_illustrative;
	std::wstring tkns_illustrative_str;

  int execCurrScope (uint32_t exec_start_pos, uint32_t after_bndry_pos, uint32_t & break_scope_end_pos);
  int check_expr_element_is_ready (std::vector<Token> & flat_expr_tkns, int curr_idx, bool & is_actor);
  int exec_flat_expr_list_element (std::vector<Token> & flat_expr_tkns, int exec_idx);
  int execFlatExpr_OLR (std::vector<Token> & expr_tkn_stream, int start_idx);
	int execOperation (Operator opr8r, int opr8r_idx, std::vector<Token> & flat_expr_tkns);
	int execExpression (uint32_t obj_start_pos, Token & result_tkn);
	int execVarDeclaration (uint32_t obj_start_pos, uint32_t object_len);
	int execPrePostFixOp (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int execUnaryOp (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int execAssignmentOp(std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int execBinaryOp (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int getEndOfSubExprIdx (std::vector<Token> & expr_tkn_stream, int start_idx, int & last_idx_expr);
	int execTernary1stOp (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int exec_logical_and (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int exec_logical_or (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int execEquivalenceOp (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int execShift (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int execBitWiseOp (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int execStandardMath (std::vector<Token> & expr_tkn_stream, int opr8r_idx);
	int resolveTknOrVar (Token & original_tkn, Token & resolved_tkn, std::wstring & varName, bool is_check_init);
	int resolveTknOrVar (Token & original_tkn, Token & resolved_tkn, std::wstring & varName);
	int exec_if_block (uint32_t scope_start_pos, uint32_t if_scope_len, uint32_t after_parent_scope_pos, uint32_t & break_scope_end_pos);
  int exec_cached_expr (std::vector<Token> expr_tkn_list, bool & is_result_true);
  int get_expr_from_var_declaration (uint32_t start_pos, std::vector<Token> & expr_tkn_list);
  int exec_for_loop (uint32_t scope_start_pos, uint32_t for_scope_len, uint32_t after_parent_scope_pos, uint32_t & break_scope_end_pos);
  int exec_while_loop (uint32_t scope_start_pos, uint32_t for_scope_len, uint32_t after_parent_scope_pos, uint32_t & break_scope_end_pos);
  
  bool isOkToIllustrate ();
  void illustrativeB4op (std::vector<Token> & flat_expr_tkns, int currIdx);	
	void illustrativeAfterOp (std::vector<Token> & flat_expr_tkns);

  // TODO: Should these be protected?  One of them private?
  int exec_system_call (std::vector<Token> & flat_expr_tkns, int sys_call_idx);
  int exec_sys_call_str (std::vector<Token> & flat_expr_tkns, int sys_call_idx);

};

#endif /* RUNTIMEINTERPRETER_H_ */
