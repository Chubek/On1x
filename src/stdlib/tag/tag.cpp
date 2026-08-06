#include "api/api_common.hpp"
#include "core/list.hpp"
#include "core/optional.hpp"
#include "core/tag_table.hpp"
#include "core/tagged_list.hpp"
#include "gc/roots.hpp"
#include "on1x/on1x_module.h"
#include "on1x/on1x_capability.h"
#include "runtime/state.hpp"
#include "stdlib/args.hpp"
#include <string>
namespace on1x::stdlib { namespace {
On1x_Status bad(On1x_State*s,const char*m)noexcept{return push_api_error(s,m);}
On1x_Status name(On1x_State*s,int a)noexcept{if(!require_arity(s,a,1,"Tag.Name"))return ON1X_ERR;Value v;if(!read_argument(s,1,v)||v.kind()!=Value::Kind::Tag)return bad(s,"Tag.Name");auto*t=new_string(&s->gc,tag_text(as_tag_const(v)));return stack_push(s,value_from_object(t))?ON1X_OK:ON1X_ERR;}
On1x_Status from(On1x_State*s,int a)noexcept{if(!require_arity(s,a,1,"Tag.FromString"))return ON1X_ERR;Value v;if(!read_argument(s,1,v)||v.kind()!=Value::Kind::String)return bad(s,"Tag.FromString");auto*t=s->tags.intern(&s->gc,string_view(as_string_const(v)));return stack_push(s,value_from_object(t))?ON1X_OK:ON1X_ERR;}
On1x_Status attach(On1x_State*s,int a)noexcept{if(!require_arity(s,a,2,"Tag.Attach"))return ON1X_ERR;Value t,p;if(!read_argument(s,1,t)||!read_argument(s,2,p)||t.kind()!=Value::Kind::Tag||p.kind()!=Value::Kind::List)return bad(s,"Tag.Attach");auto*l=as_list_const(p);auto*out=new_list(&s->gc,l->length);GcRoot r(out);out->constructor=as_tag(t);for(size_t i=0;i<l->length;i++){Value x;list_get(l,i,x);list_push(&s->gc,out,x);}return stack_push(s,value_from_object(out))?ON1X_OK:ON1X_ERR;}
On1x_Status detach(On1x_State*s,int a)noexcept{if(!require_arity(s,a,1,"Tag.Detach"))return ON1X_ERR;Value v;if(!read_argument(s,1,v)||v.kind()!=Value::Kind::List)return bad(s,"Tag.Detach");auto*l=as_list_const(v);if(!l->constructor)return stack_push(s,make_none(&s->gc,s->reserved))?ON1X_OK:ON1X_ERR;auto*out=new_list(&s->gc,2);GcRoot r(out);list_push(&s->gc,out,value_from_object(l->constructor));auto*p=new_list(&s->gc,l->length);GcRoot pr(p);for(size_t i=0;i<l->length;i++){Value x;list_get(l,i,x);list_push(&s->gc,p,x);}list_push(&s->gc,out,value_from_object(p));return stack_push(s,make_some(&s->gc,s->reserved,value_from_object(out)))?ON1X_OK:ON1X_ERR;}
On1x_Status is(On1x_State*s,int a)noexcept{if(!require_arity(s,a,2,"Tag.Is"))return ON1X_ERR;Value v,t;if(!read_argument(s,1,v)||!read_argument(s,2,t)||v.kind()!=Value::Kind::List||t.kind()!=Value::Kind::Tag)return bad(s,"Tag.Is");auto*l=as_list_const(v);return stack_push(s,Value::boolean(l&&l->constructor==as_tag(t)))?ON1X_OK:ON1X_ERR;}
const On1x_FnDesc fns[]={{"Name",name},{"FromString",from},{"Attach",attach},{"Detach",detach},{"Is",is}};const On1x_ModuleDesc desc{"Tag",ON1X_CAP_NONE,fns,5};} const On1x_ModuleDesc* tag_module()noexcept{return &desc;} }
