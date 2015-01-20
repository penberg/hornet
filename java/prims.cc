#include "hornet/java.hh"

#include <memory>

using namespace std;

namespace hornet {

using jboolean_klass = primitive_klass<jboolean>;
using jchar_klass    = primitive_klass<jchar>;
using jfloat_klass   = primitive_klass<jfloat>;
using jdouble_klass  = primitive_klass<jdouble>;
using jbyte_klass    = primitive_klass<jbyte>;
using jshort_klass   = primitive_klass<jshort>;
using jint_klass     = primitive_klass<jint>;
using jlong_klass    = primitive_klass<jlong>;

shared_ptr<jboolean_klass> jvm_jboolean_klass = make_shared<jboolean_klass>("boolean");
shared_ptr<jchar_klass>    jvm_jchar_klass    = make_shared<jchar_klass>("char");
shared_ptr<jfloat_klass>   jvm_jfloat_klass   = make_shared<jfloat_klass>("float");
shared_ptr<jdouble_klass>  jvm_jdouble_klass  = make_shared<jdouble_klass>("double");
shared_ptr<jbyte_klass>    jvm_jbyte_klass    = make_shared<jbyte_klass>("byte");
shared_ptr<jshort_klass>   jvm_jshort_klass   = make_shared<jshort_klass>("short");
shared_ptr<jint_klass>     jvm_jint_klass     = make_shared<jint_klass>("int");
shared_ptr<jlong_klass>    jvm_jlong_klass    = make_shared<jlong_klass>("long");
shared_ptr<void_klass>     jvm_void_klass     = make_shared<void_klass>("void");

shared_ptr<klass> prim_sig_to_klass(char sig)
{
    switch (sig) {
    case 'B': return jvm_jbyte_klass;
    case 'C': return jvm_jchar_klass;
    case 'D': return jvm_jdouble_klass;
    case 'F': return jvm_jfloat_klass;
    case 'I': return jvm_jint_klass;
    case 'J': return jvm_jlong_klass;
    case 'S': return jvm_jshort_klass;
    case 'Z': return jvm_jboolean_klass;
    case 'V': return jvm_void_klass;
    default:  assert(0);
    }
}

shared_ptr<klass> atype_to_klass(uint8_t atype)
{
    switch (atype) {
    case JVM_T_BOOLEAN: return jvm_jboolean_klass;
    case JVM_T_CHAR:    return jvm_jchar_klass;
    case JVM_T_FLOAT:   return jvm_jfloat_klass;
    case JVM_T_DOUBLE:  return jvm_jdouble_klass;
    case JVM_T_BYTE:    return jvm_jbyte_klass;
    case JVM_T_SHORT:   return jvm_jshort_klass;
    case JVM_T_INT:     return jvm_jint_klass;
    case JVM_T_LONG:    return jvm_jlong_klass;
    default:            assert(0);
    }
}

}
