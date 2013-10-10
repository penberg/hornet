#include "hornet/jni.hh"
#include "hornet/vm.hh"

#include <cassert>
#include <jni.h>

static jint HORNET_JNI(DestroyJavaVM)(JavaVM *vm)
{
    delete hornet::_jvm;

    return JNI_OK;
}

static jint HORNET_JNI(AttachCurrentThread)(JavaVM *vm, void **penv, void *args)
{
    assert(0);
}

static jint HORNET_JNI(DetachCurrentThread)(JavaVM *vm)
{
    assert(0);
}

static jint HORNET_JNI(GetEnv)(JavaVM *vm, void **penv, jint version)
{
    assert(0);
}

static jint HORNET_JNI(AttachCurrentThreadAsDaemon)(JavaVM *vm, void **penv, void *args)
{
    assert(0);
}

static const struct JNIInvokeInterface_ HORNET_JNI(JNIInvokeInterface) = {
    HORNET_DEFINE_JNI(DestroyJavaVM),
    HORNET_DEFINE_JNI(AttachCurrentThread),
    HORNET_DEFINE_JNI(DetachCurrentThread),
    HORNET_DEFINE_JNI(GetEnv),
    HORNET_DEFINE_JNI(AttachCurrentThreadAsDaemon),
};

jint JNI_GetDefaultJavaVMInitArgs(void *args)
{
    auto vm_args = reinterpret_cast<JavaVMInitArgs*>(args);

    switch (vm_args->version) {
    case JNI_VERSION_1_6: {

        vm_args->ignoreUnrecognized	= JNI_FALSE;
        vm_args->options		= nullptr;
        vm_args->nOptions		= 0;

        return JNI_OK;
    }
    default:
        return JNI_ERR;
    }
}

static JNIEnv HORNET_JNI(JNIEnv) = {
    &HORNET_JNI(JNINativeInterface),
};

static JavaVM HORNET_JNI(JavaVM) = {
    &HORNET_JNI(JNIInvokeInterface),
};

jint JNI_CreateJavaVM(JavaVM **vm, void **penv, void *args)
{
    hornet::_jvm = new hornet::jvm();

    auto env = reinterpret_cast<JNIEnv **>(penv);

    *vm	= &HORNET_JNI(JavaVM);
    *env	= &HORNET_JNI(JNIEnv);

    return JNI_OK;
}

static jclass HORNET_JNI(FindClass)(JNIEnv *env, const char *name)
{
    auto loader = hornet::system_loader();

    auto klass = loader->load_class(name);
    if (!klass) {
        return nullptr;
    }

    return hornet::to_jclass(klass.get());
}

static jmethodID
HORNET_JNI(GetStaticMethodID)(JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
    auto *klass = hornet::from_jclass(clazz);

    auto method = klass->lookup_method(name, sig);

    if (!method) {
        hornet::throw_exception(java_lang_NoSuchMethodError);
        return nullptr;
    }

    return hornet::to_jmethodID(method.get());
}

static void HORNET_JNI(CallStaticVoidMethodV)(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
    auto* method = hornet::from_jmethodID(methodID);

    hornet::_jvm->invoke(method);
}

static void HORNET_JNI(CallStaticVoidMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
    va_list args;

    HORNET_JNI(CallStaticVoidMethodV)(env, clazz, methodID, args);
}

static jboolean HORNET_JNI(ExceptionCheck)(JNIEnv *env)
{
    hornet::thread *current = hornet::thread::current();

    if (current->exception)
        return JNI_TRUE;

    return JNI_FALSE;
}

const struct JNINativeInterface_ HORNET_JNI(JNINativeInterface) = {
    HORNET_DEFINE_JNI(FindClass),
    HORNET_DEFINE_JNI(GetStaticMethodID),
    HORNET_DEFINE_JNI(CallStaticVoidMethod),
    HORNET_DEFINE_JNI(CallStaticVoidMethodV),
    HORNET_DEFINE_JNI(ExceptionCheck),
};
