#include "encoder.h"

static void encoder_encode_expr_primary
    (Encoder* encoder, Expr* expr, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_PRIMARY);
    assert(code_ref != NULL);

    ExprPrimary primary = expr->expr.primary;

    switch (primary.kind)
    {
        case EXPR_PRIMARY_UNKNOWN   : UNREACHABLE;
        case EXPR_PRIMARY_NIL       : encoder_push_instr(encoder, code_ref, OP_CODE_PUSH_NIL); return;
        case EXPR_PRIMARY_BOOLEAN   :
            primary.primary.boolean
                ? encoder_push_instr(encoder, code_ref, OP_CODE_PUSH_TRUE )
                : encoder_push_instr(encoder, code_ref, OP_CODE_PUSH_FALSE);
            return;

        case EXPR_PRIMARY_STRING    :
        {
            Obj* obj = encoder_str_to_obj_str(encoder, primary.primary.string);
            encoder_push_instr_push(encoder, code_ref, obj);
            return;
        }

        case EXPR_PRIMARY_NATURAL   :
        {
            Obj* obj = encoder_str_to_obj_nat(encoder, primary.primary.string);
            encoder_push_instr_push(encoder_code_ref, obj);
            return;
        }

        case EXPR_PRIMARY_INTEGER   :
        {
            Obj* obj = encoder_str_to_obj_int(encoder, primary.primary.string);
            encoder_push_instr_push(encoder, code_ref, obj);
            return;
        }

        case EXPR_PRIMARY_REAL      :
        {
            Obj* obj = encoder_str_to_obj_real(encoder, primary.primary.string);
            encoder_push_instr_push(encoder, code_ref, obj);
            return;
        }

        case EXPR_PRIMARY_LIST      :
        {
            ExprPrimaryList list = primary.primary.list;
            assert(list.length >= 0);

            for (int i = 0; i < list.length; ++i)
            {
                Expr* e = list.list[i];
                encoder_encode_expr(encoder, e, code_ref);
            }
            Obj* obj = encoder_uint_to_obj_nat(encoder, (unsigned int) list.length);
            encoder_push_instr_push (encoder, code_ref, obj);
            encoder_push_instr      (encoder, code_ref, OP_CODE_NEW_LIST);
            return;
        }

        case EXPR_PRIMARY_STRUCT    :
        {
            ExprPrimaryStruct structt = primary.primary.structt;
            assert(structt.argc >= 0);

            for (int i = 0; i < structt.argc; ++i)
            {
                ExprPrimaryStructField* field = structt.argv[i];
                assert(field != NULL);
                Expr* e = field->value;

                encoder_encode_expr(encoder, e, code_ref);
            }
            Obj* obj = encoder_uint_to_obj_nat(encoder, (unsigned int) structt.argc);
            encoder_push_instr_push (encoder, code_ref, obj);
            encoder_push_instr      (encoder, code_ref, OP_CODE_NEW_STRUCT);
            return;
        }

        case EXPR_PRIMARY_LAMBDA    :
        {
            Obj* obj = encoder_expr_primary_lambda_to_obj_lambda(encoder, expr);
            encoder_push_instr_push(encoder, code_ref, obj);
            return;
        }

        case EXPR_PRIMARY_IDENTIFIER: UNREACHABLE; // IMPLEMENT:

        case EXPR_PRIMARY_DECL      :
        {
            Obj* obj = encoder_get_decl_nat_pos_in_stack(encoder, primary.decl);
            encoder_push_instr_push (encoder, code_ref, obj);
            encoder_push_instr      (encoder, OP_CODE_LOAD);
            return;
        }
    }
    UNREACHABLE;
}

static void encoder_encode_expr_unary_helper
    (Encoder* encoder, Expr* expr, DYNAMIC_ARRAY(Byte*)* code_ref, OpCode unary_op_code)
{
    assert(encoder != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_UNARY);
    assert(op_code_is_unary(unary_op_code));

    encoder_encode_expr (encoder, unary.unary, code_ref);
    encoder_push_instr  (encoder, code_ref, unary_op_code);
}

static void encoder_encode_expr_unary
    (Encoder* encoder, Expr* expr, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_UNARY);

    ExprUnary unary = expr->expr.unary;

    switch (unary.kind)
    {
        case EXPR_UNARY_UNKNOWN: UNREACHABLE;
        case EXPR_UNARY_NOT    : encoder_encode_expr_unary_helper(encoder, expr, code_ref, OP_CODE_NOT);
        case EXPR_UNARY_NEGATE : encoder_encode_expr_unary_helper(encoder, expr, code_ref, OP_CODE_NEG);
    }
    UNREACHABLE;
}

static void encoder_encode_expr_binary_helper
    (Encoder* encoder, Expr* expr, DYNAMIC_ARRAY(Byte*)* code_ref, OpCode binary_op_code)
{
    assert(encoder != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);
    assert(op_code_is_binary(binary_op_code));

    ExprBinary binary = expr->expr.binary;

    encoder_encode_expr(encoder, binary.left , code_ref);
    encoder_encode_expr(encoder, binary.right, code_ref);
    encoder_push_instr (encoder, code_ref, binary_op_code);
}

static void encoder_encode_expr_binary
    (Encoder* encoder, Expr* expr, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_BINARY);

    // IMPLEMENT:
    UNREACHABLE;

    ExprBinary binary = expr->expr.binary;

    switch (binary.kind)
    {
        case EXPR_BINARY_UNKNOWN      : UNREACHABLE;
        case EXPR_BINARY_ADD          : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_ADD);
        case EXPR_BINARY_SUBTRACT     : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_SUB);
        case EXPR_BINARY_MULTIPLY     : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_MUL);
        case EXPR_BINARY_DIVIDE       : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_DIV);
        case EXPR_BINARY_MODULO       : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_MOD);
        case EXPR_BINARY_AND          : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_AND);
        case EXPR_BINARY_OR           : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_OR );
        case EXPR_BINARY_EQUAL        : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_EQL);
        case EXPR_BINARY_NOT_EQUAL    : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_NEQ);
        case EXPR_BINARY_LESS_EQUAL   : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_LSE);
        case EXPR_BINARY_LESS         : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_LS );
        case EXPR_BINARY_GREATER_EQUAL: encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_GRE);
        case EXPR_BINARY_GREATER      : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_GR );
        case EXPR_BINARY_CHAIN        : UNREACHABLE;
        case EXPR_BINARY_ACCESS       : UNREACHABLE; // IMPLEMENT:
        case EXPR_BINARY_ASSIGN       :
        {
            UNREACHABLE; // IMPLEMENT:

            encoder_encode_expr     (encoder, binary.right, code_ref);
            encoder_encode_rvalue   (encoder, binary.left , code_ref);
            encoder_push_instr_push (encoder, code_ref, obj);
            encoder_push_instr      (encoder, code_ref, OP_CODE_STORE);
            return;
        }

        case EXPR_BINARY_INDEX        : encoder_encode_expr_binary_helper (encoder, expr, code_ref, OP_CODE_ACCESS);
    }
    UNREACHABLE;
}

static void encoder_encode_expr_fn
    (Encoder* encoder, Expr* expr, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(expr    != NULL);
    assert(expr->kind == EXPR_FN);

    ExprFn fn = expr->expr.fn;

    assert(fn.argc >= 0);

    for (int i = 0; i < fn.argc; ++i)
    {
        encoder_encode_expr(encoder, fn.argv[i], code_ref);
    }

    encoder_encode_rvalue(encoder, fn.caller, code_ref);
    Obj* obj = encoder_uint_to_obj_nat(encoder, (unsigned int) fn.argc);
    if (encoder_is_expr_rvalue_type_constructor(encoder, fn.caller))
    {
        encoder_push_instr(encoder, code_ref, OP_CODE_TCALL);
    }
    else if (encoder_is_expr_rvalue_fn(encoder, fn.caller))
    {
        encoder_push_instr(encoder, code_ref, OP_CODE_CALL );
    }
    else
    {
        UNREACHABLE;
    }
}

extern void encoder_encode_expr(Encoder* encoder, Expr* expr, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(expr    != NULL);

    switch (expr->kind)
    {
        case EXPR_PRIMARY: encoder_encode_expr_primary (encoder, expr, code_ref); return;
        case EXPR_UNARY  : encoder_encode_expr_unary   (encoder, expr, code_ref); return;
        case EXPR_BINARY : encoder_encode_expr_binary  (encoder, expr, code_ref); return;
        case EXPR_FN     : encoder_encode_expr_fn      (encoder, expr, code_ref); return;
    }
    UNREACHABLE;
}

static void encoder_encode_stmt_block(Encoder* encoder, Stmt* stmt)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);
    assert(stmt->kind == STMT_BLOCK);

    // IMPLEMENT:
    UNREACHABLE;

    int    size = stmt->stmt.block.size;
    Stmt** body = stmt->stmt.block.body;

    encoder_set_new_block(encoder, stmt);
    for (int i = 0; i < size; ++i)
    {
        encoder_encode_stmt(encoder, body[i]);
    }
    encoder_restore_old_block(encoder);
}

static void encoder_encode_stmt_let
    (Encoder* encoder, Stmt* stmt, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);
    assert(stmt->kind == STMT_LET);
    // In hindsight, it was probably a bad idea to add an 'EXPR_ASSIGN' automatically when parsing.
    // TODO: Change this later.
    // assert(stmt->stmt.let.expr->kind == EXPR_BINARY);
    // assert(stmt->stmt.let.expr->expr.binary.kind == EXPR_ASSIGN);

    encoder_encode_expr(encoder, stmt->stmt.let.expr, code_ref);
}

static void encoder_encode_stmt_expr
    (Encoder* encoder, Stmt* stmt, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);
    assert(stmt->kind == STMT_EXPR);

    encoder_encode_expr(encoder, stmt->stmt.expr, code_ref);
    encoder_remove_excess_from_stack(encoder);
}

static void encoder_encode_stmt_if
    (Encoder* encoder, Stmt* stmt, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);
    assert(stmt->kind == STMT_IF);

    StmtIf curr_if = stmt->stmt.iff;
    bool there_is_another_branch = true;
    DYNAMIC_ARRAY(unsigned int*) unresolved_jmp_locations = NULL;

    // This looks sooo readable/s
    {
        Obj* obj = NULL;
        unsigned int unresolved_jmp_location = 0;
        DYNAMIC_ARRAY(Byte*) inner_code = NULL;

        // Encode the statement, append jump to end of conditional.
        // We do not know where to jump, so we use obj_nil as a placeholder,
        // and save the location of the obj to replace it later.
        encoder_encode_stmt(encoder, curr_if.body, &inner_code);
        unresolved_jmp_location = arrlen(inner_code);
        obj = encoder_create_obj_nil(encoder);
        encoder_push_instr_push(encoder, &inner_code, obj);
        encoder_push_instr(encoder, &inner_code, OP_CODE_UGOTO);

        // Append jump past inner body in conditional.
        // Previously saved location has to be updated accordingly.
        // Append inner_code to *code_ref, and then free inner_code.
        obj = encoder_uint_to_obj_nat(encoder,
            (unsigned int) (arrlen(inner_code) + arrlen(*code_ref)));
        if (curr_if.condition != NULL)
        {
            encoder_encode_expr(encoder, curr_if.condition, code_ref);
            encoder_push_instr (encoder, code_ref, OP_CODE_NOT);
            encoder_push_instr_push(encoder, code_ref, obj);
            encoder_push_instr (encoder, code_ref, OP_CODE_GOTO);
        }
        else
        {
            encoder_push_instr_push(encoder, code_ref, obj);
            encoder_push_instr(encoder, code_ref, OP_CODE_UGOTO);
        }
        unresolved_jmp_location += arrlen(*code_ref);
        arrput(unresolved_jmp_locations, unresolved_jmp_location);
        append_list_to_list(*code_ref, inner_code);
        arrfree(inner_code);

        if (curr_if.next == NULL)
        {
            there_is_another_branch = false;
        }
        else
        {
            assert(curr_if.next->kind == STMT_IF);
            curr_if = curr_if.next->stmt.iff;
        }
    }
    while (there_is_another_branch);

    unsigned int number_of_unresolved_jmp_locations = arrlen(unresolved_jmp_locations);
    for (int i = 0; i < number_of_unresolved_jmp_locations; ++i)
    {
        unsigned int unresolved_jmp_location = unresolved_jmp_locations[i];
        Obj* obj = encoder_uint_to_obj_nat(encoder, (unsigned int) arrlen(*code_ref));
        encode_replace_obj_at_location_in_push_obj_instr
            (encoder, code_ref, unresolved_jmp_location, obj);
    }
    arrfree(unresolved_jmp_locations);

    encoder_encode_stmt_if(encoder, iff.next, code_ref);
}

static void encoder_encode_stmt_while
    (Encoder* encoder, Stmt* stmt, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);
    assert(code_ref != NULL);

    StmtWhile whilee = stmt->stmt.whilee;
    Obj* old_continue = NULL;
    Obj* obj = NULL;
    DYNAMIC_ARRAY(unsigned int*) old_breaks = NULL;
    DYNAMIC_ARRAY(unsigned int*) new_breaks = NULL;
    unsigned int init_code_length = arrlen(*code_ref);
    unsigned int unresolved_jmp_location = 0;

    old_breaks = encoder_get_unresolved_breaks(encoder);
    encoder_set_unresolved_breaks(new_breaks);
    old_continue = encoder_get_curr_continue(encoder);

    encoder_encode_stmt(encoder, curr_if.body, &inner_code);
    obj = encoder_uint_to_obj_nat(encoder, init_code_length);
    encoder_push_instr_push(encoder, &inner_code, obj);
    encoder_push_instr(encoder, &inner_code, OP_CODE_UGOTO);

    // Because that's where the location is calculated.
    encoder_set_curr_continue(encoder, obj);

    // Append jump past inner body in conditional.
    // Previously saved location has to be updated accordingly.
    // Append inner_code to *code_ref, and then free inner_code.
    if (curr_if.condition != NULL)
    {
        encoder_encode_expr(encoder, curr_if.condition, code_ref);
        encoder_push_instr (encoder, code_ref, OP_CODE_NOT);
        unresolved_jmp_location = arrlen(inner_code);
        encoder_push_instr_nil(encoder);
        encoder_push_instr (encoder, code_ref, OP_CODE_GOTO);
    }
    else
    {
        unresolved_jmp_location = arrlen(inner_code);
        encoder_push_instr_nil(encoder);
        encoder_push_instr(encoder, code_ref, OP_CODE_UGOTO);
    }
    append_list_to_list(*code_ref, inner_code);
    arrfree(inner_code);

    unresolved_jmp_location += arrlen(*code_ref);
    obj = encoder_uint_to_obj_nat(encoder, arrlen(*code_ref));
    encode_replace_obj_at_location_in_push_obj_instr
        (encoder, code_ref, unresolved_jmp_location, obj);

    // Setting all of the unresolved breaks
    new_breaks = encoder_get_unresolved_breaks(encoder);
    unsigned int number_of_breaks = arrlen(new_breaks);
    for (int i = 0; i < number_of_breaks; ++i)
    {
        unsigned int break_location = new_breaks[i];
        encode_replace_obj_at_location_in_push_obj_instr
            (encoder, code_ref, break_location, obj);
    }
    arrfree(new_breaks);

    encoder_set_unresolved_breaks(encoder, old_breaks);
    encoder_set_curr_continue(encoder, old_continue);
}

static void encoder_encode_stmt_break
    (Encoder* encoder, Stmt* stmt, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);
    assert(stmt->kind == STMT_BREAK);

    unsigned int unresolved_break = 0;

    unresolved_break = arrlen(*code_ref);
    encoder_push_instr_push(encoder, code_ref, obj);
    encoder_push_instr (encoder, code_ref, OP_CODE_GOTO);

    encoder_append_break(encoder, unresolved_break);
}

static void encoder_encode_stmt_continue(Encoder* encoder, Stmt* stmt)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);
    assert(stmt->kind == STMT_CONTINUE);

    Obj* obj = encoder_get_curr_continue(encoder);
    encoder_push_instr_push(encoder, code_ref, obj);
    encoder_push_instr (encoder, code_ref, OP_CODE_GOTO);
}

// TODO: Make this handle closures.
static void encoder_encode_stmt_fn(Encoder* encoder, Stmt* stmt)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);
    assert(stmt->kind == STMT_FN);
    assert(stmt->stmt.fn.argc >= 0);

    StmtFn fn = stmt->stmt.fn;
    DYNAMIC_ARRAY(Byte*) code = NULL;
    Obj* obj = NULL;
    Byte* final_code = NULL;
    unsigned int final_code_length = 0;
    Obj* old_return = NULL;

    old_return = encoder_get_curr_return(encoder);

    for (int i = 0; i < fn.argc; ++i)
    {
        FnArg* arg = fn.argv[i];
        encoder_formalize_fn_arg(encoder, arg->decl);
    }
    encoder_set_curr_return(encoder,
                            encoder_get_fn_return(encoder));
    encoder_encode_stmt(encoder, stmt, &code);

    encoder_make_code_final(encoder, code, &final_code, &final_code_length);
    obj = encoder_create_obj_fn
        (encoder, (unsigned int) fn.argc, final_code, final_code_length);
    encoder_bind_obj_to_decl(encoder, fn.decl, obj);

    encoder_set_curr_return(encoder, old_return);
}

static void encoder_encode_stmt_return
    (Encoder* encoder, Stmt* stmt, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);
    assert(stmt->kind == STMT_RETURN);

    Obj* obj = encoder_get_curr_return(encoder);
    encoder_push_instr_push(encoder, code_ref, obj);
    encoder_push_instr (encoder, code_ref, OP_CODE_GOTO);
}

static void encoder_encode_stmt_alias(Encoder* encoder, Stmt* stmt)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);

    // IMPLEMENT:
    UNREACHABLE;
}

static void encoder_encode_stmt_type(Encoder* encoder, Stmt* stmt)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);

    // IMPLEMENT:
    UNREACHABLE;
}

static void encoder_encode_stmt_match(Encoder* encoder, Stmt* stmt)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);

    // IMPLEMENT:
    UNREACHABLE;
}

extern void encoder_encode_stmt(Encoder* encoder, Stmt* stmt)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);

    switch (stmt->kind)
    {
        case STMT_BLOCK   : encoder_encode_stmt_block    (encoder, stmt); return;
        case STMT_LET     : encoder_encode_stmt_let      (encoder, stmt); return;
        case STMT_EXPR    : encoder_encode_stmt_expr     (encoder, stmt); return;
        case STMT_IF      : encoder_encode_stmt_if       (encoder, stmt); return;
        case STMT_WHILE   : encoder_encode_stmt_while    (encoder, stmt); return;
        case STMT_BREAK   : encoder_encode_stmt_break    (encoder, stmt); return;
        case STMT_CONTINUE: encoder_encode_stmt_continue (encoder, stmt); return;
        case STMT_FN      : encoder_encode_stmt_fn       (encoder, stmt); return;
        case STMT_RETURN  : encoder_encode_stmt_return   (encoder, stmt); return;
        case STMT_ALIAS   : encoder_encode_stmt_alias    (encoder, stmt); return;
        case STMT_TYPE    : encoder_encode_stmt_type     (encoder, stmt); return;
        case STMT_MATCH   : encoder_encode_stmt_match    (encoder, stmt); return;
    }
    UNREACHABLE;
}

extern void encoder_encode_stmts(Encoder* encoder, Stmt* stmt, DYNAMIC_ARRAY(Byte*)* code_ref)
{
    assert(encoder != NULL);
    assert(stmt    != NULL);

    encoder_encode_stmt(Encoder* encoder, stmt, code_ref);
    encoder_reset_state(encoder);
}
