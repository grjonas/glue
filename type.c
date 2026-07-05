#include "type.h"

// Type* resolver_resolve_type_expr(Resolver* resolver, TypeExpr* type_expr)
// {
//     assert(false);
//     Type   type;
//     Type*  type_ptr;
//     Type** application_argv = NULL;
//     TypeExpr** type_expr_argv = NULL;
//     TypeStructField** fields = NULL;
//     char* identifier = NULL;
//     int argc = 0;
//     TypeExprStructField** argv = NULL;
//     bool should_push_to_arena = true;
//     Decl* decl = NULL;
// 
//     if (type_expr == NULL)
//     {
//         return resolver_create_type_variable(resolver);
//     }
// 
//     switch (type_expr->kind)
//     {
//         case TYPE_EXPR_NIL:
//             type = (Type)
//             {
//                 .kind      = TYPE_NIL,
//                 .type.none = NULL    ,
//             };
//             break;
// 
//         case TYPE_EXPR_BOOL:
//             type = (Type)
//             {
//                 .kind      = TYPE_BOOL,
//                 .type.none = NULL     ,
//             };
//             break;
// 
//         case TYPE_EXPR_INT:
//             type = (Type)
//             {
//                 .kind      = TYPE_INT ,
//                 .type.none = NULL     ,
//             };
//             break;
// 
//         case TYPE_EXPR_REAL:
//             type = (Type)
//             {
//                 .kind      = TYPE_REAL,
//                 .type.none = NULL     ,
//             };
//             break;
// 
//         case TYPE_EXPR_STRING:
//             type = (Type)
//             {
//                 .kind      = TYPE_STRING,
//                 .type.none = NULL       ,
//             };
//             break;
// 
//         case TYPE_EXPR_LIST:
//             type = (Type)
//             {
//                 .kind = TYPE_LIST,
//                 .type.list = (TypeList)
//                 {
//                     .type = resolver_resolve_type_expr(resolver, type_expr->type_expr.list.type),
//                 }
//             };
//             break;
// 
//         case TYPE_EXPR_STRUCT:
//             argc = type_expr->type_expr.structt.argc;
//             argv = type_expr->type_expr.structt.argv;
// 
//             for (int i = 0; i < argc; ++i) 
//             {
//                 TypeExprStructField* f = argv[i];
//                 TypeStructField converted_field = (TypeStructField)
//                 {
//                     .key   = f->key,
//                     .value = resolver_resolve_type_expr(resolver, f->value),
//                 };
//                 TypeStructField* new_field = (TypeStructField*) arena_push(&resolver->arena, &converted_field, sizeof(TypeStructField));
//                 arrput(fields, new_field);
//             }
// 
//             type = (Type)
//             {
//                 .kind = TYPE_STRUCT,
//                 .type.structt = (TypeStruct)
//                 {
//                     .field_num = argc,
//                     .fields    = (TypeStructField**) arena_push(&resolver->arena, fields, argc * sizeof(TypeStructField*)),
//                 }
//             };
// 
//             arrfree(fields);
//             fields = NULL;
//             break;
// 
//         case TYPE_EXPR_FN:
//             type = (Type)
//             {
//                 .kind = TYPE_FN,
//                 .type.fn = (TypeFn)
//                 {
//                     .left  = resolver_resolve_type_expr(resolver, type_expr->type_expr.fn.left ),
//                     .right = resolver_resolve_type_expr(resolver, type_expr->type_expr.fn.right),
//                 }
//             };
//             break;
// 
//         case TYPE_EXPR_IDENTIFIER:
//             // In context, search for most recent declaration with said identifier
//             // If a declaration is found, get it's respective type variable, and assign it to type.
//             // Else, we create a new type variable declaration.
//             identifier = type_expr->type_expr.identifier.identifier;
// 
//             decl = resolver_get_decl_by_identifier(resolver, identifier);
//             if (decl != NULL)
//             {
//                 if
//                 (
//                     decl->kind == DECL_TYPE_VARIABLE
//                     || decl->kind == DECL_ALIAS
//                     || (decl->kind == DECL_TYPE && type_get_polymorphic_parameter_num(decl->type) == 0)
//                 )
//                 {
//                     type_ptr = decl->type;
//                 }
//                 else
//                 {
//                     resolver_throw_compiler_error(resolver, (CompileError)
//                     {
//                         .kind   = ERROR_ERROR      ,
//                         .line   = type_expr->line  ,
//                         .column = type_expr->column,
//                         .length = type_expr->length,
//                         .msg    = "Type resolution: Type variable can only be a type variable, an alias or a new type with no parameters.",
//                     });
//                     return NULL;
//                 }
//             }
//             else
//             {
//                 decl = resolver_declare_type_variable(resolver, identifier);
//                 resolver_push_decl_to_context(resolver, decl);
//                 type_ptr = decl->type;
//             }
//             should_push_to_arena = false;
// 
//             break;
// 
//         case TYPE_EXPR_INSTANCE:
//             identifier       = type_expr->type_expr.instance.caller;
//             argc             = type_expr->type_expr.instance.argc  ;
//             type_expr_argv   = type_expr->type_expr.instance.argv  ;
// 
//             decl = resolver_get_decl_by_identifier(resolver, identifier);
//             if
//                 (
//                     decl != NULL
//                     && decl->kind == DECL_TYPE
//                     && type_get_polymorphic_parameter_num(decl->type) == argc
//                 )
//             {
//                 for (int i = 0; i < argc; ++i)
//                 {
//                     TypeExpr* te = type_expr_argv[i];
//                     Type*     t  = resolver_resolve_type_expr(resolver, te);
//                     arrput(application_argv, t);
//                 }
//                 Type** tmp_ptr = application_argv;
//                 application_argv = (Type**) arena_push(&resolver->arena, application_argv, argc * sizeof(Type*));
//                 arrfree(tmp_ptr);
// 
//                 type = (Type)
//                 {
//                     .kind = TYPE_APPLICATION,
//                     .type.application = (TypeApplication)
//                     {
//                         .abstraction = decl->type,
//                         .argc        = argc               ,
//                         .argv        = application_argv   ,
//                     }
//                 };
//             }
//             else
//             {
//                 resolver_throw_compiler_error(resolver, (CompileError)
//                 {
//                     .kind   = ERROR_ERROR      ,
//                     .line   = type_expr->line  ,
//                     .column = type_expr->column,
//                     .length = type_expr->line  ,
//                     .msg    = "Type resolution: Could not find declaration of new type that also has the required number of parameters.",
//                 });
//                 return NULL;
//             }
// 
//             break;
// 
//         default:
//             assert(false);
//     }
// 
//     if (should_push_to_arena)
//     {
//         type_ptr = (Type*) arena_push(&resolver->arena, &type, sizeof(Type));
//         // arrput(resolver->types, type_ptr);
//         return type_ptr;
//     }
//     else
//     {
//         return type_ptr;
//     }
// }


// Type* resolver_resolve_stmt_fn_type(Resolver* resolver, StmtFn fn)
// {
//     Type* return_type = NULL;
//     Type* node_ptr    = NULL;
//     Type  type;
// 
//     TypeExpr* te = fn.return_type;
//     Type    * t  = te != NULL
//         ? resolver_resolve_type_expr(resolver, te)
//         : resolver_create_type_variable(resolver);
// 
//     type = (Type)
//     {
//         .kind    = TYPE_FN,
//         .type.fn = (TypeFn)
//         {
//             .left  = NULL,
//             .right = t   ,
//         }
//     };
//     return_type = (Type*) arena_push(&resolver->arena, &type, sizeof(Type));
//     node_ptr = return_type;
// 
//     for (int i = 0; i < fn.argc; ++i)
//     {
//         TypeExpr* te = fn.argv[i]->type;
//         Type    * t  = te != NULL
//             ? resolver_resolve_type_expr(resolver, te)
//             : resolver_create_type_variable(resolver);
// 
//         if (i == 0)
//         {
//             node_ptr->type.fn.left = t;
//         }
//         else
//         {
//             type = (Type)
//             {
//                 .kind    = TYPE_FN,
//                 .type.fn = (TypeFn)
//                 {
//                     .left  = t,
//                     .right = node_ptr->type.fn.right,
//                 }
//             };
// 
//             Type* tmp = (Type*) arena_push(&resolver->arena, &type, sizeof(Type));
//             node_ptr->type.fn.right = tmp;
//             node_ptr = tmp;
//         }
//     }
// 
//     return return_type;
// }

// void resolver_declare_fn_params(Resolver* resolver, StmtFn fn, Type* fn_type)
// {
//     assert(fn_type->kind == TYPE_FN);
//     assert(fn.argc == 0 ? fn_type->type.fn.left == NULL : true);
// 
//     char* identifier = NULL;
//     Type* type       = NULL;
//     Type* node       = fn_type;
//     Decl* decl       = NULL;
// 
//     for (int i = 0; i < fn.argc; ++i)
//     {
//         assert(node->kind == TYPE_FN);
// 
//         identifier = fn.argv[i]->identifier;
// 
//         type = node->type.fn.left;
//         decl = resolver_declare_variable(resolver, identifier, type);
//         fn.argv[i]->decl = decl;
//         resolver_push_decl_to_context(resolver, decl);
//         node = node->type.fn.right;
//     }
// }

// int type_get_polymorphic_parameter_num(Type* type)
// {
//     assert(type->kind == TYPE_ABSTRACTION);
// 
//     return type->type.abstraction.parameter_num;
// }
// 
