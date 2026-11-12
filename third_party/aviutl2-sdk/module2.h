#pragma once
// AviUtl ExEdit2 Plugin SDK: script module header, Copyright (c) 2025 Kenkun.
// Distributed under the accompanying MIT License (LICENSE.txt).

struct EDIT_SECTION;
struct META_METHOD_FUNCTION;
enum class PARAM_TYPE : int { NONE=-1, NIL=0, BOOLEAN=1, LIGHTUSERDATA=2, NUMBER=3, STRING=4, TABLE=5, FUNCTION=6, USERDATA=7, THREAD=8 };
struct SCRIPT_MODULE_PARAM {
 int (*get_param_num)(); int (*get_param_int)(int); double (*get_param_double)(int); LPCSTR (*get_param_string)(int); void* (*get_param_data)(int);
 int (*get_param_table_int)(int,LPCSTR); double (*get_param_table_double)(int,LPCSTR); LPCSTR (*get_param_table_string)(int,LPCSTR);
 int (*get_param_array_num)(int); int (*get_param_array_int)(int,int); double (*get_param_array_double)(int,int); LPCSTR (*get_param_array_string)(int,int);
 void (*push_result_int)(int); void (*push_result_double)(double); void (*push_result_string)(LPCSTR); void (*push_result_data)(void*);
 void (*push_result_table_int)(LPCSTR*,int*,int); void (*push_result_table_double)(LPCSTR*,double*,int); void (*push_result_table_string)(LPCSTR*,LPCSTR*,int);
 void (*push_result_array_int)(int*,int); void (*push_result_array_double)(double*,int); void (*push_result_array_string)(LPCSTR*,int);
 void (*set_error)(LPCSTR);
 bool (*get_param_boolean)(int); void (*push_result_boolean)(bool); bool (*get_param_table_boolean)(int,LPCSTR); void (*push_result_array_boolean)(bool*,int); void (*push_result_table_boolean)(LPCSTR*,bool*,int);
 EDIT_SECTION* edit; void (*push_result_function)(void (*)(SCRIPT_MODULE_PARAM*),void*); void (*deprecated_push_result_meta_table)(void (*)(SCRIPT_MODULE_PARAM*),void (*)(SCRIPT_MODULE_PARAM*),void*); void* userdata;
 void (*push_result_meta_table)(META_METHOD_FUNCTION*,void*); void* (*get_param_meta_table)(int,META_METHOD_FUNCTION*); PARAM_TYPE (*get_param_type)(int);
};
struct META_METHOD_FUNCTION { LPCSTR method; void (*func)(SCRIPT_MODULE_PARAM*); };
struct SCRIPT_MODULE_FUNCTION { LPCWSTR name; void (*func)(SCRIPT_MODULE_PARAM*); };
struct SCRIPT_MODULE_TABLE { LPCWSTR information; SCRIPT_MODULE_FUNCTION* functions; };
