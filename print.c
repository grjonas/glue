#include "print.h"

void stmt_print_indent(FILE* file, int depth)
{
    const char* indent = " ";
    for (int i = 0; i < depth; ++i)
    {
        fprintf(file, "%s", indent);
    }
}

const char* stmt_kind_name(StmtKind kind)
{
    switch (kind)
    {
        case STMT_BLOCK:             return "do";
        case STMT_LET:               return "let";
        case STMT_EXPR:              return "expr";
        case STMT_IF:                return "if";
        case STMT_WHILE:             return "while";
        case STMT_BREAK:             return "break";
        case STMT_CONTINUE:          return "continue";
        case STMT_FN:                return "fn";
        case STMT_RETURN:            return "return";
        case STMT_ALIAS:             return "alias";
        case STMT_TYPE:              return "type";
    }

    return "UNKNOWN";
}

void stmt_print_inner(FILE* file, Stmt* show_stmt, int depth)
{
    assert(depth >= 0);

    const int indent = 4;
    if (show_stmt == NULL)
    {
        stmt_print_indent(file, indent * depth);
        fprintf(file, "NULL\n");
        return;
    }

    char    * identifier = NULL;
    Expr    * expr       = NULL;
    TypeExpr* type_expr  = NULL;
    Decl    * decl       = NULL;
    Stmt    * stmt       = NULL;
    Stmt    * next       = NULL;
    int       block_size = 0   ;
    Stmt   ** block      = NULL;
    StmtFn    stmt_fn;

    char   ** argv       = NULL;
    int       argc       = 0   ;
    StmtTypeConstructor** constructors = NULL;
    int constructor_num                = 0   ;

    stmt_print_indent(file, indent * depth);
    fprintf(file, "%s", stmt_kind_name(show_stmt->kind));
    switch (show_stmt->kind)
    {
        case STMT_BLOCK   :
            block_size = show_stmt->stmt.block.size;
            block      = show_stmt->stmt.block.body;

            fprintf(file, "\n");

            for (int i = 0; i < block_size; ++i)
            {
                Stmt* s = block[i];
                stmt_print_inner(file, s, depth + 1);
            }

            stmt_print_indent(file, indent * depth);
            fprintf(file, "end\n");
            break;

        case STMT_LET     :
            identifier = show_stmt->stmt.let.identifier;
            type_expr  = show_stmt->stmt.let.type      ;
            decl       = show_stmt->stmt.let.decl      ;
            expr       = show_stmt->stmt.let.expr      ;

            fprintf(file, " %s", identifier);
            decl_print(file, decl);
            fprintf(file, " : ");
            type_expr_print(file, type_expr);
            fprintf(file, " = ");
            expr_print     (file, expr     );
            fprintf(file, "\n");
            break;

        case STMT_EXPR    :
            expr       = show_stmt->stmt.expr;

            fprintf(file, " ");
            expr_print     (file, expr     );
            fprintf(file, "\n");
            break;

        case STMT_IF      :
            expr = show_stmt->stmt.iff.condition;
            stmt = show_stmt->stmt.iff.body;
            next = show_stmt->stmt.iff.next;

            fprintf(file, " ");
            expr_print     (file, expr     );
            fprintf(file, "\n");
            stmt_print_inner(file, stmt, depth + 1);
            if (next != NULL)
            {
                stmt_print_inner(file, next, depth);
            }
            break;

        case STMT_WHILE   :
            expr = show_stmt->stmt.whilee.condition;
            stmt = show_stmt->stmt.whilee.body;

            fprintf(file, " ");
            expr_print     (file, expr     );
            fprintf(file, "\n");
            stmt_print_inner(file, stmt, depth + 1);
            break;

        case STMT_BREAK   :
            fprintf(file, "\n");
            break;

        case STMT_CONTINUE:
            fprintf(file, "\n");
            break;

        case STMT_FN      :
            stmt_fn = show_stmt->stmt.fn;

            fprintf(file, " %s", stmt_fn.identifier);
            decl_print(file, stmt_fn.decl);
            fprintf(file, "(");

            for (int i = 0; i < stmt_fn.argc; ++i)
            {
                StmtFnArg* arg = stmt_fn.argv[i];
                fprintf(file, "%s", arg->identifier);
                decl_print(file, arg->decl);
                fprintf(file, " : ");
                type_expr_print(file, arg->type);

                if (i + 1 < stmt_fn.argc)
                {
                    fprintf(file, ", ");
                }
            }

            fprintf(file, ") : ");
            type_expr_print(file, stmt_fn.return_type);
            fprintf(file, "\n");
            stmt_print_inner(file, stmt_fn.body, depth + 1);

            break;

        case STMT_RETURN  :
            expr = show_stmt->stmt.returnn.expr;

            if (expr != NULL)
            {
                fprintf(file, " ");
                expr_print     (file, expr     );
            }
            fprintf(file, "\n");
            break;

        case STMT_ALIAS   :
            identifier = show_stmt->stmt.alias.identifier;
            type_expr  = show_stmt->stmt.alias.type      ;

            fprintf(file, " %s ", identifier);
            type_expr_print(file, type_expr);
            fprintf(file, "\n");
            break;

        case STMT_TYPE:
            identifier      = show_stmt->stmt.type.identifier     ;
            argv            = show_stmt->stmt.type.argv           ;
            constructors    = show_stmt->stmt.type.constructors   ;
            argc            = show_stmt->stmt.type.argc           ;
            constructor_num = show_stmt->stmt.type.constructor_num;

            fprintf(file, "type %s(", identifier);
            for (int i = 0; i < argc; ++i)
            {
                fprintf(file, "%s", argv[i]);
                if (i + 1 < argc)
                {
                    fprintf(file, ", ");
                }
            }
            fprintf(file, ")\n");
            break;
    }
}

void stmt_print(FILE* file, Stmt* stmt)
{
    stmt_print_inner(file, stmt, 0);
}

const char* expr_primary_kind_name(ExprPrimaryKind primary)
{
    switch (primary)
    {
        case EXPR_PRIMARY_UNKNOWN   : return "EP_UNKNOWN";
        case EXPR_PRIMARY_NIL       : return "EP_NIL";
        case EXPR_PRIMARY_BOOLEAN   : return "EP_BOOLEAN";
        case EXPR_PRIMARY_STRING    : return "EP_STRING";
        case EXPR_PRIMARY_NATURAL   : return "EP_NATURAL";
        case EXPR_PRIMARY_INTEGER   : return "EP_INTEGER";
        case EXPR_PRIMARY_REAL      : return "EP_REAL";
        case EXPR_PRIMARY_LIST      : return "EP_LIST";
        case EXPR_PRIMARY_STRUCT    : return "EP_STRUCT";
        case EXPR_PRIMARY_FN        : return "EP_FN";
        case EXPR_PRIMARY_IDENTIFIER: return "EP_IDENTIFIER";
        case EXPR_PRIMARY_DECL      : return "EP_DECL";
    }

    return "[EXPR_PRIMARY]";
}

const char* expr_unary_kind_name(ExprUnaryKind unary)
{
    switch (unary)
    {
        case EXPR_UNARY_UNKNOWN: return "EU_UNKNOWN";
        case EXPR_UNARY_NOT    : return "EU_NOT";
        case EXPR_UNARY_NEGATE : return "EU_NEGATE";
    }

    return "[EXPR_UNARY]";
}

const char* expr_binary_kind_name(ExprBinaryKind binary)
{
    switch (binary)
    {

        case EXPR_BINARY_UNKNOWN      : return "EB_UNKNOWN";
        case EXPR_BINARY_ADD          : return "EB_ADD";
        case EXPR_BINARY_SUBTRACT     : return "EB_SUBTRACT";
        case EXPR_BINARY_MULTIPLY     : return "EB_MULTIPLY";
        case EXPR_BINARY_DIVIDE       : return "EB_DIVIDE";
        case EXPR_BINARY_MODULO       : return "EB_MODULO";
        case EXPR_BINARY_AND          : return "EB_AND";
        case EXPR_BINARY_OR           : return "EB_OR";
        case EXPR_BINARY_EQUAL        : return "EB_EQUAL";
        case EXPR_BINARY_NOT_EQUAL    : return "EB_NOT_EQUAL";
        case EXPR_BINARY_LESS_EQUAL   : return "EB_LESS_EQUAL";
        case EXPR_BINARY_LESS         : return "EB_LESS";
        case EXPR_BINARY_GREATER_EQUAL: return "EB_GREATER_EQUAL";
        case EXPR_BINARY_GREATER      : return "EB_GREATER";
        case EXPR_BINARY_CHAIN        : return "EB_CHAIN";
        case EXPR_BINARY_ACCESS       : return "EB_ACCESS";
        case EXPR_BINARY_ASSIGN       : return "EB_ASSIGN";
        case EXPR_BINARY_INDEX        : return "EB_INDEX";
    }

    return "[EXPR_BINARY]";
}

void expr_primary_print(FILE* file, ExprPrimary primary)
{
    switch (primary.kind)
    {
        case EXPR_PRIMARY_UNKNOWN   :
            fprintf(file, "[EP_UNKNOWN]");
            break; // not return

        case EXPR_PRIMARY_NIL       :
            fprintf(file, "nil");
            return;

        case EXPR_PRIMARY_BOOLEAN   :
            fprintf(file, primary.primary.boolean ? "true" : "false");
            return;

        case EXPR_PRIMARY_STRING    :
            fprintf(file, "\"%s\"", primary.primary.string);
            return;

        case EXPR_PRIMARY_NATURAL   :
            fprintf(file, "%s", primary.primary.natural);
            return;

        case EXPR_PRIMARY_INTEGER   :
            fprintf(file, "%s", primary.primary.integer);
            return;

        case EXPR_PRIMARY_REAL      :
            fprintf(file, "%s", primary.primary.real);
            return;

        case EXPR_PRIMARY_LIST      :
            fprintf(file, "[ ");
            for (int i = 0; i < primary.primary.list.length; ++i)
            {
                Expr* e = primary.primary.list.list[i];

                expr_print(file, e);
                if (i + 1 < primary.primary.list.length)
                {
                    fprintf(file, ", ");
                }
            }
            fprintf(file, " ]");
            return;

        case EXPR_PRIMARY_STRUCT    :
            fprintf(file, "{ ");
            for (int i = 0; i < primary.primary.structt.argc; ++i)
            {
                ExprPrimaryStructField* field = primary.primary.structt.argv[i];

                fprintf(file, "%s", field->key == NULL ? "[NULL]" : field->key);
                if (field->type != NULL)
                {
                    fprintf(file, " : ");
                    type_expr_print(file, field->type);
                }
                if (field->value != NULL)
                {
                    fprintf(file, " = ");
                    expr_print(file, field->value);
                }


                if (i + 1 < primary.primary.structt.argc)
                {
                    fprintf(file, ", ");
                }
            }
            fprintf(file, " }");
            return;

        case EXPR_PRIMARY_FN        :
            fprintf(file, "[EXPR_PRIMARY_FN]");
            return;

        case EXPR_PRIMARY_IDENTIFIER:
            fprintf(file, "id:%s", primary.primary.identifier);
            return;

        case EXPR_PRIMARY_DECL      :
            decl_print(file, primary.primary.decl);
            return;
    }

    fprintf(file, "[EXPR_PRIMARY]");
}

void expr_print(FILE* file, Expr* expr)
{
    // fprintf(file, "[EXPR]");

    if (expr == NULL)
    {
        fprintf(file, "[NULL]");
        return;
    }

    switch (expr->kind)
    {
        case EXPR_PRIMARY:
            expr_primary_print(file, expr->expr.primary);
            break;

        case EXPR_UNARY  :
            fprintf(file, "( %s ", expr_unary_kind_name(expr->expr.unary.kind));
            expr_print(file, expr->expr.unary.unary);
            fprintf(file, ")");
            break;

        case EXPR_BINARY :
            fprintf(file, "( %s ", expr_binary_kind_name(expr->expr.binary.kind));
            expr_print(file, expr->expr.binary.left);
            fprintf(file, " ");
            expr_print(file, expr->expr.binary.right);
            fprintf(file, ")");
            break;

        case EXPR_FN     :
            expr_print(file, expr->expr.fn.caller);
            fprintf(file, "(");

            for (int i = 0; i < expr->expr.fn.argc; ++i)
            {
                Expr* e = expr->expr.fn.argv[i];
                expr_print(file, e);
                if (i + 1 < expr->expr.fn.argc)
                {
                    fprintf(file, ", ");
                }
            }

            fprintf(file, ")");
            break;
    }
}

void type_expr_print(FILE* file, TypeExpr* type_expr)
{
    if (type_expr == NULL)
    {
        fprintf(file, "[TE_NULL]");
        return;
    }

    switch (type_expr->kind)
    {
        case TYPE_EXPR_IDENTIFIER:
            fprintf(file, "%s", type_expr->type_expr.identifier.identifier);
            return;

        case TYPE_EXPR_VARIABLE :
            decl_print(file, type_expr->type_expr.variable.decl);
            return;

        case TYPE_EXPR_NIL       :
            fprintf(file, "Nil");
            return;

        case TYPE_EXPR_BOOL      :
            fprintf(file, "Bool");
            return;

        case TYPE_EXPR_INT       :
            fprintf(file, "Int");
            return;

        case TYPE_EXPR_REAL      :
            fprintf(file, "Real");
            return;

        case TYPE_EXPR_STRING    :
            fprintf(file, "String");
            return;

        case TYPE_EXPR_LIST      :
            fprintf(file, "[");
            type_expr_print(file, type_expr->type_expr.list.type);
            fprintf(file, "]");
            return;

        case TYPE_EXPR_STRUCT    :
            fprintf(file, "{ ");

            for (int i = 0; i < type_expr->type_expr.structt.argc; ++i)
            {
                TypeExprStructField* field = type_expr->type_expr.structt.argv[i];
                fprintf(file, "%s : ", field->key);
                type_expr_print(file, field->value);
                if (i + 1 < type_expr->type_expr.structt.argc)
                {
                    fprintf(file, ", ");
                }
            }

            fprintf(file, " }");
            return;

        case TYPE_EXPR_FN        :
            TypeExprFn type_fn = type_expr->type_expr.fn;
            if (type_fn.left->kind == TYPE_EXPR_FN) fprintf(file, "(");
            type_expr_print(file, type_fn.left);
            if (type_fn.left->kind == TYPE_EXPR_FN) fprintf(file, ")");
            fprintf(file, " -> ");
            type_expr_print(file, type_fn.right);
            return;

        case TYPE_EXPR_INSTANCE  :
            fprintf(file, "[TE_INSTANCE]");
            return;
    }

    fprintf(file, "[TYPE_EXPR]");
}

void type_print(FILE* file, Type* type)
{
    if (type == NULL)
    {
        fprintf(file, "[T_NULL]");
        return;
    }

    fprintf(file, "[TYPE]");

//     switch (type->kind)
//     {
//         case TYPE_NIL        :
//             fprintf(file, "Nil");
//             return;
// 
//         case TYPE_BOOL       :
//             fprintf(file, "Bool");
//             return;
// 
//         case TYPE_INT        :
//             fprintf(file, "Int");
//             return;
// 
//         case TYPE_REAL       :
//             fprintf(file, "Real");
//             return;
// 
//         case TYPE_STRING     :
//             fprintf(file, "String");
//             return;
// 
//         case TYPE_LIST       :
//             fprintf(file, "[");
//             type_print(file, type->type.list.type);
//             fprintf(file, "]");
//             return;
// 
//         case TYPE_STRUCT     :
//             fprintf(file, "[TYPE_STRUCT]");
//             return;
// 
//         case TYPE_FN         :
//             TypeFn type_fn = type->type.fn;
//             if (type_fn.left->kind == TYPE_FN) fprintf(file, "(");
//             type_print(file, type_fn.left);
//             if (type_fn.left->kind == TYPE_FN) fprintf(file, ")");
//             fprintf(file, " -> ");
//             type_print(file, type_fn.right);
//             return;
// 
//         case TYPE_VARIABLE   :
//             fprintf(file, "@%d", type->type.variable.id);
//             return;
// 
//         case TYPE_ALIAS      :
//             fprintf(file, "[TYPE_ALIAS]");
//             return;
// 
//         case TYPE_ABSTRACTION:
//             fprintf(file, "[TYPE_ABSTRACTION]");
//             return;
// 
//         case TYPE_APPLICATION:
//             fprintf(file, "[TYPE_APPLICATION]");
//             return;
//     }
// 
//     fprintf(file, "[TYPE]");
}

void decl_print(FILE* file, Decl* decl)
{
    fprintf(file, "<");

    if (decl == NULL)
    {
        fprintf(file, "[NULL]");
    }
    else
    {
        switch (decl->kind)
        {
            case DECL_LET             :
                fprintf(file, "let");
                break;

            case DECL_TYPE_VARIABLE   :
                fprintf(file, "type_var");
                break;

            case DECL_ALIAS           :
                fprintf(file, "alias");
                break;

            case DECL_TYPE            :
                fprintf(file, "type");
                break;

            case DECL_TYPE_CONSTRUCTOR:
                fprintf(file, "constructor");
                break;

            default:
                fprintf(file, "[DECL]");
        }

        fprintf(file, ": %s@%d", decl->identifier == NULL? "[NULL]" : decl->identifier, decl->id);
        if (decl->type != NULL)
        {
            fprintf(file, " : ");
            type_print(file, decl->type);
        }
    }

    fprintf(file, ">");
}
