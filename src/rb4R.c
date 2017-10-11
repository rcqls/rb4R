#include <stdio.h>
#include <string.h>

#include <ruby.h>
#include <ruby/version.h>
//#include "ruby.h"
#include <Rdefines.h>
#include <R_ext/PrtUtil.h>

// #ifndef Win32
// #include <R_ext/eventloop.h>
// #endif



static int rb4R_ruby_running=0;

//just testing with require at the beginning!!! (since this fails)
/* void rb4R_initTEST(void)
{
  ruby_init();
  ruby_init_loadpath();
  printf("ruby initialize!!!\n");
  rb_require("cqlsinit");
  rb_require("rbvor/rbvor");
  rb_require("rbvor/graphe");
  rb_eval_string("$vg=CqlsVor::VorGraph.new(100.0,[0.0,0.0])");
  rb_eval_string("$polyloc=CqlsVor::PolyLoc.new($vg)");
  } */

void rb4R_init(void)
{
  if(!rb4R_ruby_running) {
    ruby_init();
    rb4R_ruby_running=1;
    ruby_init_loadpath();
    printf("ruby initialize!!!\n");
  }
}

void rb4R_finalize(void)
{
  if(rb4R_ruby_running) {
    ruby_finalize();
    rb4R_ruby_running=0;
    printf("ruby finalize!!!\n");
  }
}

SEXP rb4R_running(void) {
  SEXP ans;

  PROTECT(ans=allocVector(LGLSXP,1));
  LOGICAL(ans)[0]=rb4R_ruby_running;
  UNPROTECT(1);
  return(ans);
}

//wrapper !!! une classe R permettant de wrapper un objet Ruby !!!
static SEXP makeRubyObject(VALUE rbobj)
{
    SEXP obj;

    obj = R_MakeExternalPtr((void *)rbobj, R_NilValue, R_NilValue);
    
    return obj;
}


int rbIsRVector(VALUE rbobj) {
  VALUE rbobj2;
  int i,n;
  
  if(!rb_obj_is_kind_of(rbobj,rb_cArray)) {
     if(!(rb_obj_is_kind_of(rbobj,rb_cFixnum) || rb_obj_is_kind_of(rbobj,rb_cFloat) || rb_obj_is_kind_of(rbobj,rb_cString) || rb_obj_is_kind_of(rbobj,rb_cTrueClass) || rb_obj_is_kind_of(rbobj,rb_cFalseClass))) 
       return 0;
     rbobj2=rb_ary_new2(1);
     rb_ary_store(rbobj2,0,rbobj);
     rbobj=rbobj2;
     //Seems that the 3 previous lines could be replaced by:
     // return 1;
  }
  n=RARRAY_LEN(rbobj);
  for(i=0;i<n;i++) {
    rbobj2=rb_ary_entry(rbobj,i);
    if(!(rb_obj_is_kind_of(rbobj2,rb_cFixnum) || rb_obj_is_kind_of(rbobj2,rb_cFloat) || rb_obj_is_kind_of(rbobj2,rb_cString) || rb_obj_is_kind_of(rbobj2,rb_cTrueClass) || rb_obj_is_kind_of(rbobj2,rb_cFalseClass))) {
      return 0;
    }
  }
  return 1;
}


SEXP newRbObj(VALUE rbobj) {
  SEXP ans,class;

  ans=(SEXP)makeRubyObject(rbobj);
  if(rbIsRVector(rbobj)) {
    PROTECT(class=allocVector(STRSXP,2));
    SET_STRING_ELT(class,0, mkChar("rbRVector"));
    SET_STRING_ELT(class,1, mkChar("rbObj"));
  } else {
    PROTECT(class=allocVector(STRSXP,1));
    SET_STRING_ELT(class,0, mkChar("rbObj"));
  }
  //classgets(ans,class);
  SET_CLASS(ans,class);
  UNPROTECT(1);
  return ans;
}


SEXP dotRb(SEXP args)
{
  SEXP ans;
  char *cmd;
  VALUE val; //non utilisé pour l'instant!!!
  int state;

  if(!isValidString(CADR(args)))
    error("invalid argument");
    cmd = (char*)CHAR(STRING_ELT(CADR(args), 0));
//printf("instruction à executer %s\n",cmd);
  val=rb_eval_string_protect(cmd,&state);
  //printf("state ->%d\n",state);
  if(state) {
    //rb_p(state);
    printf("Ruby error (state=%d)!!!\n",state);
    printf("in executing : %s\n",cmd);
    return R_NilValue;
  } else {
    ans=(SEXP)newRbObj(val);
    //ans=(SEXP)rbObj_to_vector((VALUE)val);
    return ans;
  }
}


static VALUE rb4R_require(char *file) {
  rb_require(file);
  return Qnil;
}


SEXP dotRbRequire(SEXP args)
{
  //SEXP ans;
    char *cmd;
    //VALUE val; //non utilisé pour l'instant!!!
    int state=0;

    if(!isValidString(CADR(args)))
	error("invalid argument");
    cmd = (char*)CHAR(STRING_ELT(CADR(args), 0));
    Rprintf("instruction à executer %s\n",cmd);
    rb_protect((VALUE (*)(VALUE))(&rb4R_require),(VALUE)cmd,&state);
    if(state) Rprintf("error in dotRbRequire (state=%d)!!!\n",state);
    //val non converti en RObject pour l'instant!!!
    return R_NilValue;//mkString("ok"); //retour arbitraire
}

// SEXP dotRbLoad(SEXP args)
// {
//   //SEXP ans;
//     char *cmd;
//     //VALUE val; //non utilisé pour l'instant!!!
//     int status;

//     if(!isValidString(CADR(args)))
// 	error("invalid argument");
//     cmd = (char*)CHAR(STRING_ELT(CADR(args), 0));
//     Rprintf("instruction à loader %s\n",cmd);
//     rb_protect ((VALUE (*) ()) rb_load_file, (VALUE) cmd, &status);
//    status = ruby_exec();
//    status = ruby_cleanup(status);
//     //val non converti en RObject pour l'instant!!!
//     return R_NilValue;//mkString("ok"); //retour arbitraire
// }




//convert R Vector in  rbObj
VALUE RVector2rbArray(SEXP vect)
{
  VALUE res;
  char *name;
  int i,n=0;
  Rcomplex cpl;
  VALUE res2; 

  //vect have to be R Vector!!!
  if(!isVector(vect) | isNewList(vect)) return Qnil; 
  n=length(vect);
  if(n>1) {
    res = rb_ary_new2(n);
    switch(TYPEOF(vect)) {
    case REALSXP:
      for(i=0;i<n;i++) {
	      rb_ary_store(res,i,rb_float_new(REAL(vect)[i]));
      }
      break;
    case INTSXP:
      for(i=0;i<n;i++) {
	      rb_ary_store(res,i,INT2FIX(INTEGER(vect)[i]));
      }
      break;
    case LGLSXP:
      for(i=0;i<n;i++) {
	      rb_ary_store(res,i,(INTEGER(vect)[i] ? Qtrue : Qfalse));
      }
      break;
    case STRSXP:
      for(i=0;i<n;i++) {
	      rb_ary_store(res,i,rb_str_new2(CHAR(STRING_ELT(vect,i))));
      }
      break;
    case CPLXSXP:
      rb_require("complex");
      for(i=0;i<n;i++) {
	      cpl=COMPLEX(vect)[i];
	      res2 = rb_eval_string("Complex.new(0,0)");
	      rb_iv_set(res2,"@real",rb_float_new(cpl.r));
	      rb_iv_set(res2,"@image",rb_float_new(cpl.i));
	      rb_ary_store(res,i,res2);
      }
      break;
    }
  } else {
    switch(TYPEOF(vect)) {
    case REALSXP:
      res=rb_float_new(REAL(vect)[0]);
      break;
    case INTSXP:
      res=INT2FIX(INTEGER(vect)[0]);
      break;
    case LGLSXP:
      res=(INTEGER(vect)[0] ? Qtrue : Qfalse);
      break;
    case STRSXP:
      res=rb_str_new2(CHAR(STRING_ELT(vect,0)));
      break;
    case CPLXSXP:
      rb_require("complex");
      cpl=COMPLEX(vect)[0];
      res= rb_eval_string("Complex.new(0,0)");
      rb_iv_set(res,"@real",rb_float_new(cpl.r));
      rb_iv_set(res,"@image",rb_float_new(cpl.i));
      break;
    }
  }
  return res;
}

SEXP rb4R_as_rbRvector(SEXP args)
{
  VALUE val;
  SEXP ans; 
  val=(VALUE)RVector2rbArray(CADR(args));
  ans=(SEXP)newRbObj(val);
  return(ans);
}



//convert rbObj in RVector (assumed to be possible!!!)
SEXP rbArray2RVector(VALUE rbobj)
{
  SEXP ans;
  VALUE arr,class,expr;
  char *name;
  int n,i;
  
  if(!rb_obj_is_kind_of(rbobj,rb_cArray)) {
    if(!(rb_obj_is_kind_of(rbobj,rb_cFixnum) || rb_obj_is_kind_of(rbobj,rb_cFloat) || rb_obj_is_kind_of(rbobj,rb_cString) || rb_obj_is_kind_of(rbobj,rb_cTrueClass) || rb_obj_is_kind_of(rbobj,rb_cFalseClass))) 
      return R_NilValue;
    n=1;
    arr = rb_ary_new2(1);
    rb_ary_push(arr,rbobj);
  } else {
    arr=rbobj;
    n=RARRAY_LEN(rbobj);  
  }

  //Rprintf("n=%d\n",n);

  //TODO : to improve !!!
  class=rb_class_of(rb_ary_entry(arr,0));

  if(class==rb_cFloat) {
    PROTECT(ans=allocVector(REALSXP,n));
    for(i=0;i<n;i++) {
      REAL(ans)[i]=NUM2DBL(rb_ary_entry(arr,i));
    }
#if RUBY_API_VERSION_CODE >= 20400
  } else if(class==rb_cInteger) {
#else
  } else if(class==rb_cFixnum || class==rb_cBignum) {
#endif
    PROTECT(ans=allocVector(INTSXP,n));
    for(i=0;i<n;i++) {
      INTEGER(ans)[i]=NUM2INT(rb_ary_entry(arr,i));
    }
  } else if(class==rb_cTrueClass || class==rb_cFalseClass) {
    PROTECT(ans=allocVector(LGLSXP,n));
    for(i=0;i<n;i++) {
      LOGICAL(ans)[i]=(rb_class_of(rb_ary_entry(arr,i))==rb_cFalseClass ? FALSE : TRUE);
    }
  } else if(class==rb_cString) {
    PROTECT(ans=allocVector(STRSXP,n));
    for(i=0;i<n;i++) {
      expr = rb_ary_entry(arr,i);
      SET_STRING_ELT(ans,i,mkChar(StringValuePtr(expr)));
    }
  } else ans=R_NilValue;
  UNPROTECT(1);
  return ans; 
}



SEXP rb4R_is_Rvector(SEXP args) {
  SEXP obj,ans;
  VALUE rbobj,rbobj2;
  int vect,i,n;

  obj=CADR(args);
  PROTECT(ans=allocVector(LGLSXP,1));  
  if (!inherits(obj, "rbObj"))  {
    LOGICAL(ans)[0]=FALSE;
    UNPROTECT(1);
    return ans;
  }
  rbobj=(VALUE) R_ExternalPtrAddr(CADR(obj));
  
  if(!rbIsRVector(rbobj)) {
    LOGICAL(ans)[0]=FALSE;
    UNPROTECT(1);
    return ans;
  }
  LOGICAL(ans)[0]=TRUE;
  UNPROTECT(1);
  return ans;
}

//TODO : partially correct : since args is the name of object simple or homogeneous Array!!!
SEXP rb4R_as_Rvector(SEXP args)
{
  SEXP ans;
  VALUE rbobj;

  if (inherits(CADR(args), "rbObj")) {
    rbobj=(VALUE) R_ExternalPtrAddr(CADR(args));
    ans=(SEXP)rbArray2RVector(rbobj);
    return ans; 
  } else return R_NilValue;
}


//Ruby global variable!!!
SEXP rb4R_get_gv(SEXP args) {
  SEXP ans;
  VALUE rbobj;
  char *name;
  
  if(!isValidString(CADR(args)))
    error("invalid argument");
  name = (char*)CHAR(STRING_ELT(CADR(args), 0));
  rbobj=rb_gv_get(name);
  ans=(SEXP) newRbObj(rbobj);
  return ans;
}

SEXP rb4R_set_gv(SEXP args) {
  SEXP vect;
  VALUE rbval;
  char *name;

  if(!isValidString(CADR(args)))
    error("invalid argument");
  name = (char*)CHAR(STRING_ELT(CADR(args), 0));
  vect=CADDR(args);
  rbval=(VALUE)RVector2rbArray(vect);
  rb_gv_set(name,rbval);
  return R_NilValue;
}

static VALUE rbobj_inspect(VALUE rbobj) {
  VALUE expr=rb_inspect(rbobj);
  Rprintf("%s\n",StringValuePtr(expr));
  fflush(stdout);
  return Qnil;
}

//inspect
SEXP rb4R_inspect(SEXP args) {
  VALUE rbobj,rbval;
  int state=0;

  if (inherits(CADR(args), "rbObj")) {
    rbobj=(VALUE) R_ExternalPtrAddr(CADR(args));
    //sometimes, it bugs !!! very strange!!!
    rb_protect(&rbobj_inspect,rbobj,&state);
    if(state) Rprintf("error in rb4R_inspect (state=%d)!!!\n",state);
  }
  return R_NilValue;
  
}


SEXP rb4R_get_iv(SEXP args) {
  SEXP ans;
  VALUE  rbobj;
  char *var;

  if(!inherits(CADR(args),"rbObj")) error("invalid first argument");
  rbobj=(VALUE) R_ExternalPtrAddr(CADR(args));
  if(!isValidString(CADDR(args))) error("invalid second argument");
  var = (char*)CHAR(STRING_ELT(CADDR(args), 0));
  ans=(SEXP) newRbObj(rb_iv_get(rbobj,var));
  return ans;
}

SEXP rb4R_set_iv(SEXP args) {
  SEXP vect;
  VALUE  rbobj,rbval;
  char *var;

  if(!inherits(CADR(args),"rbObj")) error("invalid first argument");
  rbobj=(VALUE) R_ExternalPtrAddr(CADR(args));
  if(!isValidString(CADDR(args))) error("invalid second argument");
  var = (char*)CHAR(STRING_ELT(CADDR(args), 0));
  vect=CADDDR(args);
  rbval=(VALUE)RVector2rbArray(vect);
  rb_iv_set(rbobj,var,rbval);
  return R_NilValue;
}


SEXP rb4R_apply(SEXP args)
{
  SEXP ans;
  char *meth;
  VALUE rbobj,rbargs;
  int state,i,nargs;
  
  nargs=length(args)-1;
  if(nargs<2) error("number of arguments greater than 2!!! ");
  args=CDR(args);
  if(!inherits(CAR(args),"rbObj")) error("invalid first argument");
  rbobj=(VALUE) R_ExternalPtrAddr(CAR(args));
  args=CDR(args);
  if(!isValidString(CAR(args))) error("invalid second argument");
  meth = (char*)CHAR(STRING_ELT(CAR(args), 0));
  nargs=nargs-2;
  rbargs = rb_ary_new2(nargs);
  if(nargs > 0) {
    for(i=0;i<nargs;i++) {
      args=CDR(args);
      if(inherits(CAR(args),"rbObj")) {
	      rb_ary_store(rbargs,i,(VALUE) R_ExternalPtrAddr(CAR(args)));
      } else {
	      rb_ary_store(rbargs,i,RVector2rbArray(CAR(args)));
      }
    } 
  }
  ans=(SEXP) newRbObj(rb_apply(rbobj,rb_intern(meth),rbargs));
  return ans;
  
  //strmeth=CHAR(STRING_ELT(meth, 0));
  //printf("%s.%s executed\n",strobj,strmeth);
  //rb_funcall(rb_gv_get(strobj),rb_intern(strmeth),0); 
  //val non converti en RObject pour l'instant!!!
  //return R_NilValue;//mkString("ok"); //retour arbitraire
}


//UNUSED!!!!
SEXP getListElement(SEXP list, char *str) 
{
  int i;
  SEXP elmt=R_NilValue, names=getAttrib(list,R_NamesSymbol);
  for ( i=0; i< length(list); i++)
    if(strcmp(CHAR(STRING_ELT(names,i)),str)==0) {
      elmt= VECTOR_ELT(list,i);
      break;
    }
  return elmt;
}



//Ne fonctionne pas !!!
/*
SEXP requireRuby(SEXP args) {
  const char *m;

  if(!isValidString(CADR(args)))
    error("invalid argument");
  m =  CHAR(STRING_ELT(CADR(args), 0));
  rb_require(m);
  return mkString("ok"); //retour arbitraire
  }
*/


#include <R_ext/Rdynload.h>
static const R_CMethodDef cMethods[] = {
  {"rb4R_init",(DL_FUNC) &rb4R_init,0},
  {"rb4R_finalize",(DL_FUNC) &rb4R_finalize,0},
  {NULL,NULL,0}
};

static const R_ExternalMethodDef externalMethods[] = {
  {"dotRb",(DL_FUNC) &dotRb,-1}, 
  {"dotRbRequire",(DL_FUNC) &dotRbRequire,-1}, 
  //{"dotRbLoad",(DL_FUNC) &dotRbLoad,-1}, 
  {"rb4R_get_gv",(DL_FUNC)&rb4R_get_gv,-1},  
  {"rb4R_set_gv",(DL_FUNC)&rb4R_set_gv,-1}, 
  {"rb4R_get_iv",(DL_FUNC)&rb4R_get_iv,-1},  
  {"rb4R_set_iv",(DL_FUNC)&rb4R_set_iv,-1},
  {"rb4R_as_Rvector",(DL_FUNC)&rb4R_as_Rvector,-1}, 
  {"rb4R_is_Rvector",(DL_FUNC)&rb4R_is_Rvector,-1},
  {"rb4R_as_rbRvector",(DL_FUNC)&rb4R_as_rbRvector,-1},
  {"rb4R_inspect",(DL_FUNC)&rb4R_inspect,-1},  
  {"rb4R_apply",(DL_FUNC)&rb4R_apply,-1},
  {NULL,NULL,0}
};

static const R_CallMethodDef callMethods[] = {
  {"rb4R_running",(DL_FUNC) &rb4R_running,0},
  {NULL,NULL,0}
};

void R_init_rb4R(DllInfo *info) {
  R_registerRoutines(info,cMethods,callMethods,NULL,externalMethods);
}

