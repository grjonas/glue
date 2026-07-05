#include "inferer.h"

Inferer inferer_init(Resolver* resolver)
{
    arrfree(resolver->context     );
    arrfree(resolver->errs        );

    *resolver = (Resolver)
    {
        .txt              = NULL                    ,
        .tokens           = NULL                    ,
        .stmts            = NULL                    ,
        .arena            = resolver->arena         ,
        .decl_id          = 0                       ,
        .type_variable_id = 0                       ,
        .loop_depth       = 0                       ,
        .inside_function  = false                   ,
        .context          = NULL                    ,
        .declarations     = NULL                    ,
        .identifiers      = NULL                    ,
        .errs             = NULL                    ,
    };

    return (Inferer)
    {
        .txt          = resolver->txt         ,
        .tokens       = resolver->tokens      ,
        .stmts        = resolver->stmts       ,
        .declarations = resolver->declarations,
        .identifiers  = resolver->identifiers ,
        .arena        = resolver->arena       ,
        .errs         = NULL                  ,
    };
}

void    inferer_free(Inferer* inferer  )
{
    free((char*) inferer->txt);
    arrfree(inferer->tokens);
    arrfree(inferer->stmts);
    arrfree(inferer->declarations);
    arrfree(inferer->identifiers);
    arena_free(&inferer->arena);
    arrfree(inferer->errs);

    *inferer = (Inferer)
    {
        .txt          = NULL,
        .tokens       = NULL,
        .stmts        = NULL,
        .declarations = NULL,
        .identifiers  = NULL,
        .errs         = NULL,
    };
}

// Type* inferer_infer_expr(Inferer* infer, TypeEnv* env, Expr* expr)
// {
//     assert(expr != NULL);
// 
//     Type* type = NULL;
// 
//     switch (expr->kind)
//     {
//         case EXPR_PRIMARY:
//             return inferer_infer_expr_primary(inferer, env, expr->expr.primary);
//             break;
// 
//         case EXPR_UNARY  :
//             break;
// 
//         case EXPR_BINARY :
//             break;
// 
//         case EXPR_FN     :
//             break;
//     };
// }
// 
// Type* inferer_infer_expr_primary(Inferer* inferer, TypeEnv* env, ExprPrimary primary)
// {
//     Type*   type   = NULL;
//     Scheme* scheme = NULL;
// 
//     TypeStructField*  type_field  = NULL;
//     TypeStructField** type_struct = NULL;
// 
//     switch (primary.kind)
//     {
//         case EXPR_PRIMARY_UNKNOWN   :
//             fprintf(stderr, "[%s:%d] Type inference: Detected unknown primary expression during inference.\n", __FILE__, __LINE__);
//             exit(1);
// 
//         case EXPR_PRIMARY_NIL       :
//             type = inferer_get_type_nil(inferer);
//             break;
// 
//         case EXPR_PRIMARY_BOOLEAN   :
//             type = inferer_get_type_bool(inferer);
//             break;
// 
//         case EXPR_PRIMARY_STRING    :
//             type = inferer_get_type_string(inferer);
//             break;
// 
//         case EXPR_PRIMARY_NATURAL   :
//             type = inferer_get_type_natural(inferer);
//             break;
// 
//         case EXPR_PRIMARY_INTEGER   :
//             type = inferer_get_type_integer(inferer);
//             break;
// 
//         case EXPR_PRIMARY_REAL      :
//             type = inferer_get_type_real(inferer);
//             break;
// 
//         case EXPR_PRIMARY_STRUCT    :
//             // for (int i = 0; i < primary.primary.structt.argc; ++i)
//             // {
//             //     ExprPrimaryStructField* field = primary.primary.structt.argv[i];
//             //     if (field.value != NULL)
//             //     {
//             //         type = inferer_infer_expr(inferer, env, field.value);
//             //     }
//             // }
//             fprintf(stderr, "[%s:%d] Type inference: Not implemented.\n", __FILE__, __LINE__);
//             exit(1);
// 
//         case EXPR_PRIMARY_FN        :
//             fprintf(stderr, "[%s:%d] Type inference: Not implemented.\n", __FILE__, __LINE__);
//             exit(1);
// 
//         case EXPR_PRIMARY_IDENTIFIER:
//             fprintf(stderr, "[%s:%d] Type inference: Somehow found identifier expression.\n", __FILE__, __LINE__);
//             exit(1);
// 
//         case EXPR_PRIMARY_DECL      :
//             scheme = inferer_type_env_lookup(inferer, env, primary.primary.decl);
//             if (scheme == NULL)
//             {
//                 assert("Error! Replace Later with throw error." == NULL);
//                 return NULL;
//             }
//             type = inferer_scheme_instantiate(inferer, scheme);
//             break;
//     }
// 
//     return type;
// }
// 
// Subst* inferer_unify(Inferer* inferer, Type* left, Type* right)
// {
//     assert(left  != NULL);
//     assert(right != NULL);
// 
//     Subst* subst = NULL;
// 
//     switch (left->kind)
//     {
//     }
// }
