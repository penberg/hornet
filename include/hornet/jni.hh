#ifndef HORNET_JNI_HH
#define HORNET_JNI_HH

#include "hornet/vm.hh"

#include <jni.h>

#define HORNET_JNI(name) Hornet_##name

#define HORNET_DEFINE_JNI(name) .name = HORNET_JNI(name)

extern const struct JNINativeInterface_ HORNET_JNI(JNINativeInterface);

namespace hornet {

inline hornet::object *from_jobject(jobject object) {
    return reinterpret_cast<hornet::object*>(object);
}

inline hornet::array *from_jarray(jarray array) {
    return reinterpret_cast<hornet::array*>(array);
}

inline hornet::klass *from_jclass(jclass klass) {
    return reinterpret_cast<hornet::klass*>(klass);
}

inline hornet::field *from_jfieldID(jfieldID field) {
    return reinterpret_cast<hornet::field*>(field);
}

inline hornet::method *from_jmethodID(jmethodID method) {
    return reinterpret_cast<hornet::method*>(method);
}

inline jobject to_jobject(object *object) {
    return reinterpret_cast<jobject>(object);
}

inline jstring to_jstring(string *string) {
    return reinterpret_cast<jstring>(string);
}

inline jarray to_jarray(array *array) {
    return reinterpret_cast<jarray>(&array->object);
}

inline jobjectArray to_jobjectArray(array *array) {
    return reinterpret_cast<jobjectArray>(&array->object);
}

inline jclass to_jclass(klass *klass) {
    return reinterpret_cast<jclass>(&klass->object);
}

inline jfieldID to_jfieldID(field *field) {
    return reinterpret_cast<jfieldID>(field);
}

inline jmethodID to_jmethodID(method *method) {
    return reinterpret_cast<jmethodID>(method);
}

}

#endif
