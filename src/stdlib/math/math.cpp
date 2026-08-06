#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_capability.h"
#include "on1x/on1x_module.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"

#include <cmath>
#include <cstring>
#include <cstdint>
#include <limits>

namespace on1x::stdlib {
namespace {
bool number(Value v) noexcept { return v.is_int() || v.is_float(); }
double real(Value v) noexcept { return v.is_float() ? v.as_float() : static_cast<double>(v.as_int()); }
On1x_Status bad(On1x_State* s, const char* m) noexcept { return push_api_error(s, m); }
On1x_Status unary_real(On1x_State* s, int argc, const char* name, double (*fn)(double)) noexcept {
    if (!require_arity(s, argc, 1, name)) return ON1X_ERR;
    Value v; if (!read_argument(s, 1, v) || !number(v)) return bad(s, name);
    return stack_push(s, Value::floating(fn(real(v)))) ? ON1X_OK : ON1X_ERR;
}
On1x_Status math_abs(On1x_State* s, int argc) noexcept {
    if (!require_arity(s, argc, 1, "Math.Abs")) return ON1X_ERR;
    Value v;
    if (!read_argument(s,1,v) || !number(v)) return bad(s,"Math.Abs");
    if (v.is_int()) return stack_push(s, Value::integer(&s->gc, v.as_int() == INT64_MIN ? INT64_MIN : (v.as_int() < 0 ? -v.as_int() : v.as_int()))) ? ON1X_OK : ON1X_ERR;
    return stack_push(s, Value::floating(std::fabs(v.as_float()))) ? ON1X_OK : ON1X_ERR;
}
On1x_Status minmax(On1x_State* s,int argc,bool min) noexcept {
    if (!require_arity(s,argc,2,min?"Math.Min":"Math.Max")) return ON1X_ERR;
    Value a,b;
    if(!read_argument(s,1,a)||!read_argument(s,2,b)||!number(a)||!number(b)) return bad(s,min?"Math.Min":"Math.Max");
    if (a.is_int()&&b.is_int()) return stack_push(s,Value::integer(&s->gc,min?std::min(a.as_int(),b.as_int()):std::max(a.as_int(),b.as_int())))?ON1X_OK:ON1X_ERR;
    return stack_push(s,Value::floating(min?std::min(real(a),real(b)):std::max(real(a),real(b))))?ON1X_OK:ON1X_ERR;
}
On1x_Status clamp(On1x_State*s,int argc) noexcept {
    if(!require_arity(s,argc,3,"Math.Clamp"))return ON1X_ERR;
    Value x,lo,hi;
    if(!read_argument(s,1,x)||!read_argument(s,2,lo)||!read_argument(s,3,hi)||!number(x)||!number(lo)||!number(hi))return bad(s,"Math.Clamp");
    if(x.is_int()&&lo.is_int()&&hi.is_int()) return stack_push(s,Value::integer(&s->gc,std::max(lo.as_int(),std::min(x.as_int(),hi.as_int()))))?ON1X_OK:ON1X_ERR;
    return stack_push(s,Value::floating(std::max(real(lo),std::min(real(x),real(hi)))))?ON1X_OK:ON1X_ERR;
}
On1x_Status sign(On1x_State*s,int argc) noexcept { if(!require_arity(s,argc,1,"Math.Sign"))return ON1X_ERR;Value v;if(!read_argument(s,1,v)||!number(v))return bad(s,"Math.Sign");double x=real(v);return stack_push(s,Value::integer(&s->gc,x<0?-1:x>0?1:0))?ON1X_OK:ON1X_ERR; }
On1x_Status domain_unary(On1x_State*s,int argc,const char*n,double(*fn)(double),bool positive) noexcept { if(!require_arity(s,argc,1,n))return ON1X_ERR;Value v;if(!read_argument(s,1,v)||!number(v))return bad(s,n);double x=real(v);bool invalid=(positive&&x<=0);if(std::strcmp(n,"Math.Sqrt")==0)invalid=x<0;if(std::strcmp(n,"Math.Asin")==0||std::strcmp(n,"Math.Acos")==0)invalid=x<-1||x>1;if(invalid)return stack_push(s,make_none(&s->gc,s->reserved))?ON1X_OK:ON1X_ERR;return stack_push(s,make_some(&s->gc,s->reserved,Value::floating(fn(x))))?ON1X_OK:ON1X_ERR; }
On1x_Status sqrt_(On1x_State*s,int a)noexcept{return domain_unary(s,a,"Math.Sqrt",std::sqrt,false);}
On1x_Status log_(On1x_State*s,int a)noexcept{return domain_unary(s,a,"Math.Log",std::log,true);}
On1x_Status log2_(On1x_State*s,int a)noexcept{return domain_unary(s,a,"Math.Log2",std::log2,true);}
On1x_Status log10_(On1x_State*s,int a)noexcept{return domain_unary(s,a,"Math.Log10",std::log10,true);}
On1x_Status asin_(On1x_State*s,int a)noexcept{return domain_unary(s,a,"Math.Asin",std::asin,false);}
On1x_Status acos_(On1x_State*s,int a)noexcept{return domain_unary(s,a,"Math.Acos",std::acos,false);}
On1x_Status trig(On1x_State*s,int a,const char*n,double(*f)(double))noexcept{return unary_real(s,a,n,f);}
On1x_Status pow_(On1x_State*s,int a)noexcept{if(!require_arity(s,a,2,"Math.Pow"))return ON1X_ERR;Value x,y;if(!read_argument(s,1,x)||!read_argument(s,2,y)||!number(x)||!number(y))return bad(s,"Math.Pow");return stack_push(s,Value::floating(std::pow(real(x),real(y))))?ON1X_OK:ON1X_ERR;}
On1x_Status atan2_(On1x_State*s,int a)noexcept{if(!require_arity(s,a,2,"Math.Atan2"))return ON1X_ERR;Value y,x;if(!read_argument(s,1,y)||!read_argument(s,2,x)||!number(y)||!number(x))return bad(s,"Math.Atan2");return stack_push(s,Value::floating(std::atan2(real(y),real(x))))?ON1X_OK:ON1X_ERR;}
On1x_Status round_op(On1x_State*s,int a,const char*n,double(*f)(double))noexcept{return unary_real(s,a,n,f);}
On1x_Status to_int(On1x_State*s,int a)noexcept{if(!require_arity(s,a,1,"Math.ToInt"))return ON1X_ERR;Value v;if(!read_argument(s,1,v)||!v.is_float()||!std::isfinite(v.as_float())||v.as_float()<static_cast<double>(INT64_MIN)||v.as_float()>static_cast<double>(INT64_MAX))return stack_push(s,make_none(&s->gc,s->reserved))?ON1X_OK:ON1X_ERR;return stack_push(s,make_some(&s->gc,s->reserved,Value::integer(&s->gc,static_cast<std::int64_t>(v.as_float()))))?ON1X_OK:ON1X_ERR;}
On1x_Status divmod(On1x_State*s,int a)noexcept{if(!require_arity(s,a,2,"Math.DivMod"))return ON1X_ERR;Value x,y;if(!read_argument(s,1,x)||!read_argument(s,2,y)||!x.is_int()||!y.is_int())return bad(s,"Math.DivMod");if(y.as_int()==0)return stack_push(s,make_none(&s->gc,s->reserved))?ON1X_OK:ON1X_ERR;auto*l=new_list(&s->gc,2);GcRoot r(l);list_push(&s->gc,l,Value::integer(&s->gc,x.as_int()/y.as_int()));list_push(&s->gc,l,Value::integer(&s->gc,x.as_int()%y.as_int()));return stack_push(s,make_some(&s->gc,s->reserved,value_from_object(l)))?ON1X_OK:ON1X_ERR;}
On1x_Status gcd_(On1x_State*s,int a)noexcept{if(!require_arity(s,a,2,"Math.Gcd"))return ON1X_ERR;Value x,y;if(!read_argument(s,1,x)||!read_argument(s,2,y)||!x.is_int()||!y.is_int())return bad(s,"Math.Gcd");std::int64_t aa=x.as_int(),bb=y.as_int();while(bb){auto t=aa%bb;aa=bb;bb=t;}return stack_push(s,Value::integer(&s->gc,aa<0?-aa:aa))?ON1X_OK:ON1X_ERR;}
On1x_Status isnan_(On1x_State*s,int a,bool inf)noexcept{if(!require_arity(s,a,1,inf?"Math.IsInf":"Math.IsNaN"))return ON1X_ERR;Value v;if(!read_argument(s,1,v)||!v.is_float())return bad(s,inf?"Math.IsInf":"Math.IsNaN");return stack_push(s,Value::boolean(inf?std::isinf(v.as_float()):std::isnan(v.as_float())))?ON1X_OK:ON1X_ERR;}
#define U(NAME,F) [](On1x_State*s,int a)noexcept{return unary_real(s,a,"Math." NAME,F);}
const On1x_FnDesc fns[]={{"Abs",math_abs},{"Min",[](On1x_State*s,int a)noexcept{return minmax(s,a,true);}},{"Max",[](On1x_State*s,int a)noexcept{return minmax(s,a,false);}},{"Clamp",clamp},{"Sign",sign},{"Sqrt",sqrt_},{"Log",log_},{"Log2",log2_},{"Log10",log10_},{"Pow",pow_},{"Exp",U("Exp",std::exp)},{"Sin",U("Sin",std::sin)},{"Cos",U("Cos",std::cos)},{"Tan",U("Tan",std::tan)},{"Asin",asin_},{"Acos",acos_},{"Atan",U("Atan",std::atan)},{"Atan2",atan2_},{"Floor",U("Floor",std::floor)},{"Ceil",U("Ceil",std::ceil)},{"Trunc",U("Trunc",std::trunc)},{"Round",U("Round",[](double x){return std::round(x);})},{"ToInt",to_int},{"DivMod",divmod},{"Gcd",gcd_},{"IsNaN",[](On1x_State*s,int a)noexcept{return isnan_(s,a,false);}},{"IsInf",[](On1x_State*s,int a)noexcept{return isnan_(s,a,true);}}};
const On1x_ModuleDesc desc{"Math",ON1X_CAP_NONE,fns,sizeof(fns)/sizeof(*fns)};
}
const On1x_ModuleDesc* math_module() noexcept{return &desc;}
}
