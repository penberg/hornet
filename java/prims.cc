#include "hornet/java.hh"

namespace hornet {

primitive_klass<jboolean> jvm_jboolean_klass;
primitive_klass<jchar>    jvm_jchar_klass;
primitive_klass<jfloat>   jvm_jfloat_klass;
primitive_klass<jdouble>  jvm_jdouble_klass;
primitive_klass<jbyte>    jvm_jbyte_klass;
primitive_klass<jshort>   jvm_jshort_klass;
primitive_klass<jint>     jvm_jint_klass;
primitive_klass<jlong>    jvm_jlong_klass;

klass* atype_to_klass(uint8_t atype)
{
    switch (atype) {
    case JVM_T_BOOLEAN: return &jvm_jboolean_klass;
    case JVM_T_CHAR:    return &jvm_jchar_klass;
    case JVM_T_FLOAT:   return &jvm_jfloat_klass;
    case JVM_T_DOUBLE:  return &jvm_jdouble_klass;
    case JVM_T_BYTE:    return &jvm_jbyte_klass;
    case JVM_T_SHORT:   return &jvm_jshort_klass;
    case JVM_T_INT:     return &jvm_jint_klass;
    case JVM_T_LONG:    return &jvm_jlong_klass;
    default:            assert(0);
    }
}

}
