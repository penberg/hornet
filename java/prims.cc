#include "hornet/java.hh"

namespace hornet {

primitive_klass<jboolean> jvm_jboolean_klass{"boolean"};
primitive_klass<jchar>    jvm_jchar_klass{"char"};
primitive_klass<jfloat>   jvm_jfloat_klass{"float"};
primitive_klass<jdouble>  jvm_jdouble_klass{"double"};
primitive_klass<jbyte>    jvm_jbyte_klass{"byte"};
primitive_klass<jshort>   jvm_jshort_klass{"short"};
primitive_klass<jint>     jvm_jint_klass{"int"};
primitive_klass<jlong>    jvm_jlong_klass{"long"};
klass                     jvm_void_klass{"void"};

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

klass* prim_sig_to_klass(const std::string& name)
{
    switch (name[0]) {
    case 'B': return &jvm_jbyte_klass;
    case 'C': return &jvm_jchar_klass;
    case 'F': return &jvm_jfloat_klass;
    case 'D': return &jvm_jdouble_klass;
    case 'I': return &jvm_jint_klass;
    case 'J': return &jvm_jlong_klass;
    case 'S': return &jvm_jshort_klass;
    case 'V': return &jvm_void_klass;
    case 'Z': return &jvm_jboolean_klass;
    default:  assert(0);
    }
}

}
