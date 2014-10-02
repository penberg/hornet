#include "hornet/jni.hh"

#include "hornet/java.hh"
#include "hornet/vm.hh"

#include <cassert>
#include <cstring>
#include <sstream>
#include <jni.h>

#define STUB \
    do { \
        fprintf(stderr, "warning: jni: %s: stubbed out\n", __func__); \
    } while (0);

static jint HORNET_JNI(DestroyJavaVM)(JavaVM *vm)
{
    delete hornet::_backend;

    hornet::verifier_stats();

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
    .reserved0 = nullptr,
    .reserved1 = nullptr,
    .reserved2 = nullptr,
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

static bool option_matches(const char* option, const char* expected)
{
    if (strlen(option) != strlen(expected)) {
        return false;
    }
    return !strncmp(option, expected, strlen(option));
}

jint JNI_CreateJavaVM(JavaVM **vm, void **penv, void *args)
{
    auto vm_args = reinterpret_cast<JavaVMInitArgs*>(args);

    // Use current working directory as default classpath.
    std::string classpath(".");

    auto backend = hornet::backend_type::interp;

    for (auto i = 0; i < vm_args->nOptions; i++) {
        const char *opt = vm_args->options[i].optionString;

        if (!strcmp(opt, "-cp") || !strcmp(opt, "-classpath")) {
            classpath = std::string{vm_args->options[++i].optionString};
            continue;
        }
        if (!strcmp(opt, "-verbose:verifier")) {
            hornet::verbose_verifier = true;
            continue;
        }
        if (!strcmp(opt, "-XX:+DynASM")) {
#ifdef CONFIG_HAVE_DYNASM
            backend = hornet::backend_type::dynasm;
            continue;
#else
            fprintf(stderr, "error: DynASM support is not compiled in.\n");
            return JNI_ERR;
#endif
        }
        if (option_matches(opt, "-XX:+LLVM")) {
#ifdef CONFIG_HAVE_LLVM
            backend = hornet::backend_type::llvm;
#else
            fprintf(stderr, "error: LLVM support is not compiled in.\n");
            return JNI_ERR;
#endif
            continue;
        }
        if (option_matches(opt, "-XX:+LLVMDebug")) {
#ifdef CONFIG_HAVE_LLVM
            backend = hornet::backend_type::llvm;
            hornet::llvm_backend::debug = true;
#else
            fprintf(stderr, "error: LLVM support is not compiled in.\n");
            return JNI_ERR;
#endif
            continue;
        }

        fprintf(stderr, "error: Unknown option: '%s'\n", opt);
        return JNI_ERR;
    }

    switch (backend) {
    case hornet::backend_type::interp:
        hornet::_backend = new hornet::interp_backend();
        break;
#ifdef CONFIG_HAVE_DYNASM
    case hornet::backend_type::dynasm:
        hornet::_backend = new hornet::dynasm_backend();
        break;
#endif
#ifdef CONFIG_HAVE_LLVM
    case hornet::backend_type::llvm:
        hornet::_backend = new hornet::llvm_backend();
        break;
#endif
    default:
        assert(0);
    }
    std::istringstream buf(classpath);
    for (std::string entry; getline(buf, entry, ':'); ) {
        hornet::system_loader::get()->register_entry(entry);
    }

    hornet::_jvm = new hornet::jvm();

    auto env = reinterpret_cast<JNIEnv **>(penv);

    *vm	= &HORNET_JNI(JavaVM);
    *env	= &HORNET_JNI(JNIEnv);

    hornet::gc_init();

    hornet::system_loader::init();

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

    auto thread = hornet::thread::current();

    auto frame = thread->make_frame(method->max_locals);

    for (int i = 0; i < method->args_count; i++) {
        frame->locals[i] = va_arg(args, uint64_t);
    }

    hornet::_backend->execute(method, *frame);

    thread->free_frame(frame);
}

static void HORNET_JNI(CallStaticVoidMethod)(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
    va_list args;

    va_start(args, methodID);

    HORNET_JNI(CallStaticVoidMethodV)(env, clazz, methodID, args);

    va_end(args);
}

static jobjectArray
HORNET_JNI(NewObjectArray)(JNIEnv* env, jsize len, jclass clazz, jobject init)
{
    auto klass = hornet::from_jclass(clazz);

    assert(init == nullptr);

    auto array = hornet::gc_new_object_array(klass, len);

    return hornet::to_jobjectArray(array);
}

static jboolean HORNET_JNI(ExceptionCheck)(JNIEnv *env)
{
    hornet::thread *current = hornet::thread::current();

    if (current->exception)
        return JNI_TRUE;

    return JNI_FALSE;
}

#define HORNET_DEFINE_JNI_STUB(name) .name = nullptr

const struct JNINativeInterface_ HORNET_JNI(JNINativeInterface) = {
    .reserved0 = nullptr,
    .reserved1 = nullptr,
    .reserved2 = nullptr,
    .reserved3 = nullptr,
    HORNET_DEFINE_JNI_STUB(GetVersion),
    HORNET_DEFINE_JNI_STUB(DefineClass),
    HORNET_DEFINE_JNI(FindClass),
    HORNET_DEFINE_JNI_STUB(FromReflectedMethod),
    HORNET_DEFINE_JNI_STUB(FromReflectedField),
    HORNET_DEFINE_JNI_STUB(ToReflectedMethod),
    HORNET_DEFINE_JNI_STUB(GetSuperclass),
    HORNET_DEFINE_JNI_STUB(IsAssignableFrom),
    HORNET_DEFINE_JNI_STUB(ToReflectedField),
    HORNET_DEFINE_JNI_STUB(Throw),
    HORNET_DEFINE_JNI_STUB(ThrowNew),
    HORNET_DEFINE_JNI_STUB(ExceptionOccurred),
    HORNET_DEFINE_JNI_STUB(ExceptionDescribe),
    HORNET_DEFINE_JNI_STUB(ExceptionClear),
    HORNET_DEFINE_JNI_STUB(FatalError),
    HORNET_DEFINE_JNI_STUB(PushLocalFrame),
    HORNET_DEFINE_JNI_STUB(PopLocalFrame),
    HORNET_DEFINE_JNI_STUB(NewGlobalRef),
    HORNET_DEFINE_JNI_STUB(DeleteGlobalRef),
    HORNET_DEFINE_JNI_STUB(DeleteLocalRef),
    HORNET_DEFINE_JNI_STUB(IsSameObject),
    HORNET_DEFINE_JNI_STUB(NewLocalRef),
    HORNET_DEFINE_JNI_STUB(EnsureLocalCapacity),
    HORNET_DEFINE_JNI_STUB(AllocObject),
    HORNET_DEFINE_JNI_STUB(NewObject),
    HORNET_DEFINE_JNI_STUB(NewObjectV),
    HORNET_DEFINE_JNI_STUB(NewObjectA),
    HORNET_DEFINE_JNI_STUB(GetObjectClass),
    HORNET_DEFINE_JNI_STUB(IsInstanceOf),
    HORNET_DEFINE_JNI_STUB(GetMethodID),
    HORNET_DEFINE_JNI_STUB(CallObjectMethod),
    HORNET_DEFINE_JNI_STUB(CallObjectMethodV),
    HORNET_DEFINE_JNI_STUB(CallObjectMethodA),
    HORNET_DEFINE_JNI_STUB(CallBooleanMethod),
    HORNET_DEFINE_JNI_STUB(CallBooleanMethodV),
    HORNET_DEFINE_JNI_STUB(CallBooleanMethodA),
    HORNET_DEFINE_JNI_STUB(CallByteMethod),
    HORNET_DEFINE_JNI_STUB(CallByteMethodV),
    HORNET_DEFINE_JNI_STUB(CallByteMethodA),
    HORNET_DEFINE_JNI_STUB(CallCharMethod),
    HORNET_DEFINE_JNI_STUB(CallCharMethodV),
    HORNET_DEFINE_JNI_STUB(CallCharMethodA),
    HORNET_DEFINE_JNI_STUB(CallShortMethod),
    HORNET_DEFINE_JNI_STUB(CallShortMethodV),
    HORNET_DEFINE_JNI_STUB(CallShortMethodA),
    HORNET_DEFINE_JNI_STUB(CallIntMethod),
    HORNET_DEFINE_JNI_STUB(CallIntMethodV),
    HORNET_DEFINE_JNI_STUB(CallIntMethodA),
    HORNET_DEFINE_JNI_STUB(CallLongMethod),
    HORNET_DEFINE_JNI_STUB(CallLongMethodV),
    HORNET_DEFINE_JNI_STUB(CallLongMethodA),
    HORNET_DEFINE_JNI_STUB(CallFloatMethod),
    HORNET_DEFINE_JNI_STUB(CallFloatMethodV),
    HORNET_DEFINE_JNI_STUB(CallFloatMethodA),
    HORNET_DEFINE_JNI_STUB(CallDoubleMethod),
    HORNET_DEFINE_JNI_STUB(CallDoubleMethodV),
    HORNET_DEFINE_JNI_STUB(CallDoubleMethodA),
    HORNET_DEFINE_JNI_STUB(CallVoidMethod),
    HORNET_DEFINE_JNI_STUB(CallVoidMethodV),
    HORNET_DEFINE_JNI_STUB(CallVoidMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualObjectMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualObjectMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualObjectMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualBooleanMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualBooleanMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualBooleanMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualByteMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualByteMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualByteMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualCharMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualCharMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualCharMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualShortMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualShortMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualShortMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualIntMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualIntMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualIntMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualLongMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualLongMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualLongMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualFloatMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualFloatMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualFloatMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualDoubleMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualDoubleMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualDoubleMethodA),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualVoidMethod),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualVoidMethodV),
    HORNET_DEFINE_JNI_STUB(CallNonvirtualVoidMethodA),
    HORNET_DEFINE_JNI_STUB(GetFieldID),
    HORNET_DEFINE_JNI_STUB(GetObjectField),
    HORNET_DEFINE_JNI_STUB(GetBooleanField),
    HORNET_DEFINE_JNI_STUB(GetByteField),
    HORNET_DEFINE_JNI_STUB(GetCharField),
    HORNET_DEFINE_JNI_STUB(GetShortField),
    HORNET_DEFINE_JNI_STUB(GetIntField),
    HORNET_DEFINE_JNI_STUB(GetLongField),
    HORNET_DEFINE_JNI_STUB(GetFloatField),
    HORNET_DEFINE_JNI_STUB(GetDoubleField),
    HORNET_DEFINE_JNI_STUB(SetObjectField),
    HORNET_DEFINE_JNI_STUB(SetBooleanField),
    HORNET_DEFINE_JNI_STUB(SetByteField),
    HORNET_DEFINE_JNI_STUB(SetCharField),
    HORNET_DEFINE_JNI_STUB(SetShortField),
    HORNET_DEFINE_JNI_STUB(SetIntField),
    HORNET_DEFINE_JNI_STUB(SetLongField),
    HORNET_DEFINE_JNI_STUB(SetFloatField),
    HORNET_DEFINE_JNI_STUB(SetDoubleField),
    HORNET_DEFINE_JNI(GetStaticMethodID),
    HORNET_DEFINE_JNI_STUB(CallStaticObjectMethod),
    HORNET_DEFINE_JNI_STUB(CallStaticObjectMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticObjectMethodA),
    HORNET_DEFINE_JNI_STUB(CallStaticBooleanMethod),
    HORNET_DEFINE_JNI_STUB(CallStaticBooleanMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticBooleanMethodA),
    HORNET_DEFINE_JNI_STUB(CallStaticByteMethod),
    HORNET_DEFINE_JNI_STUB(CallStaticByteMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticByteMethodA),
    HORNET_DEFINE_JNI_STUB(CallStaticCharMethod),
    HORNET_DEFINE_JNI_STUB(CallStaticCharMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticCharMethodA),
    HORNET_DEFINE_JNI_STUB(CallStaticShortMethod),
    HORNET_DEFINE_JNI_STUB(CallStaticShortMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticShortMethodA),
    HORNET_DEFINE_JNI_STUB(CallStaticIntMethod),
    HORNET_DEFINE_JNI_STUB(CallStaticIntMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticIntMethodA),
    HORNET_DEFINE_JNI_STUB(CallStaticLongMethod),
    HORNET_DEFINE_JNI_STUB(CallStaticLongMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticLongMethodA),
    HORNET_DEFINE_JNI_STUB(CallStaticFloatMethod),
    HORNET_DEFINE_JNI_STUB(CallStaticFloatMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticFloatMethodA),
    HORNET_DEFINE_JNI_STUB(CallStaticDoubleMethod),
    HORNET_DEFINE_JNI_STUB(CallStaticDoubleMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticDoubleMethodA),
    HORNET_DEFINE_JNI(CallStaticVoidMethod),
    HORNET_DEFINE_JNI(CallStaticVoidMethodV),
    HORNET_DEFINE_JNI_STUB(CallStaticVoidMethodA),
    HORNET_DEFINE_JNI_STUB(GetStaticFieldID),
    HORNET_DEFINE_JNI_STUB(GetStaticObjectField),
    HORNET_DEFINE_JNI_STUB(GetStaticBooleanField),
    HORNET_DEFINE_JNI_STUB(GetStaticByteField),
    HORNET_DEFINE_JNI_STUB(GetStaticCharField),
    HORNET_DEFINE_JNI_STUB(GetStaticShortField),
    HORNET_DEFINE_JNI_STUB(GetStaticIntField),
    HORNET_DEFINE_JNI_STUB(GetStaticLongField),
    HORNET_DEFINE_JNI_STUB(GetStaticFloatField),
    HORNET_DEFINE_JNI_STUB(GetStaticDoubleField),
    HORNET_DEFINE_JNI_STUB(SetStaticObjectField),
    HORNET_DEFINE_JNI_STUB(SetStaticBooleanField),
    HORNET_DEFINE_JNI_STUB(SetStaticByteField),
    HORNET_DEFINE_JNI_STUB(SetStaticCharField),
    HORNET_DEFINE_JNI_STUB(SetStaticShortField),
    HORNET_DEFINE_JNI_STUB(SetStaticIntField),
    HORNET_DEFINE_JNI_STUB(SetStaticLongField),
    HORNET_DEFINE_JNI_STUB(SetStaticFloatField),
    HORNET_DEFINE_JNI_STUB(SetStaticDoubleField),
    HORNET_DEFINE_JNI_STUB(NewString),
    HORNET_DEFINE_JNI_STUB(GetStringLength),
    HORNET_DEFINE_JNI_STUB(GetStringChars),
    HORNET_DEFINE_JNI_STUB(ReleaseStringChars),
    HORNET_DEFINE_JNI_STUB(NewStringUTF),
    HORNET_DEFINE_JNI_STUB(GetStringUTFLength),
    HORNET_DEFINE_JNI_STUB(GetStringUTFChars),
    HORNET_DEFINE_JNI_STUB(ReleaseStringUTFChars),
    HORNET_DEFINE_JNI_STUB(GetArrayLength),
    HORNET_DEFINE_JNI(NewObjectArray),
    HORNET_DEFINE_JNI_STUB(GetObjectArrayElement),
    HORNET_DEFINE_JNI_STUB(SetObjectArrayElement),
    HORNET_DEFINE_JNI_STUB(NewBooleanArray),
    HORNET_DEFINE_JNI_STUB(NewByteArray),
    HORNET_DEFINE_JNI_STUB(NewCharArray),
    HORNET_DEFINE_JNI_STUB(NewShortArray),
    HORNET_DEFINE_JNI_STUB(NewIntArray),
    HORNET_DEFINE_JNI_STUB(NewLongArray),
    HORNET_DEFINE_JNI_STUB(NewFloatArray),
    HORNET_DEFINE_JNI_STUB(NewDoubleArray),
    HORNET_DEFINE_JNI_STUB(GetBooleanArrayElements),
    HORNET_DEFINE_JNI_STUB(GetByteArrayElements),
    HORNET_DEFINE_JNI_STUB(GetCharArrayElements),
    HORNET_DEFINE_JNI_STUB(GetShortArrayElements),
    HORNET_DEFINE_JNI_STUB(GetIntArrayElements),
    HORNET_DEFINE_JNI_STUB(GetLongArrayElements),
    HORNET_DEFINE_JNI_STUB(GetFloatArrayElements),
    HORNET_DEFINE_JNI_STUB(GetDoubleArrayElements),
    HORNET_DEFINE_JNI_STUB(ReleaseBooleanArrayElements),
    HORNET_DEFINE_JNI_STUB(ReleaseByteArrayElements),
    HORNET_DEFINE_JNI_STUB(ReleaseCharArrayElements),
    HORNET_DEFINE_JNI_STUB(ReleaseShortArrayElements),
    HORNET_DEFINE_JNI_STUB(ReleaseIntArrayElements),
    HORNET_DEFINE_JNI_STUB(ReleaseLongArrayElements),
    HORNET_DEFINE_JNI_STUB(ReleaseFloatArrayElements),
    HORNET_DEFINE_JNI_STUB(ReleaseDoubleArrayElements),
    HORNET_DEFINE_JNI_STUB(GetBooleanArrayRegion),
    HORNET_DEFINE_JNI_STUB(GetByteArrayRegion),
    HORNET_DEFINE_JNI_STUB(GetCharArrayRegion),
    HORNET_DEFINE_JNI_STUB(GetShortArrayRegion),
    HORNET_DEFINE_JNI_STUB(GetIntArrayRegion),
    HORNET_DEFINE_JNI_STUB(GetLongArrayRegion),
    HORNET_DEFINE_JNI_STUB(GetFloatArrayRegion),
    HORNET_DEFINE_JNI_STUB(GetDoubleArrayRegion),
    HORNET_DEFINE_JNI_STUB(SetBooleanArrayRegion),
    HORNET_DEFINE_JNI_STUB(SetByteArrayRegion),
    HORNET_DEFINE_JNI_STUB(SetCharArrayRegion),
    HORNET_DEFINE_JNI_STUB(SetShortArrayRegion),
    HORNET_DEFINE_JNI_STUB(SetIntArrayRegion),
    HORNET_DEFINE_JNI_STUB(SetLongArrayRegion),
    HORNET_DEFINE_JNI_STUB(SetFloatArrayRegion),
    HORNET_DEFINE_JNI_STUB(SetDoubleArrayRegion),
    HORNET_DEFINE_JNI_STUB(RegisterNatives),
    HORNET_DEFINE_JNI_STUB(UnregisterNatives),
    HORNET_DEFINE_JNI_STUB(MonitorEnter),
    HORNET_DEFINE_JNI_STUB(MonitorExit),
    HORNET_DEFINE_JNI_STUB(GetJavaVM),
    HORNET_DEFINE_JNI_STUB(GetStringRegion),
    HORNET_DEFINE_JNI_STUB(GetStringUTFRegion),
    HORNET_DEFINE_JNI_STUB(GetPrimitiveArrayCritical),
    HORNET_DEFINE_JNI_STUB(ReleasePrimitiveArrayCritical),
    HORNET_DEFINE_JNI_STUB(GetStringCritical),
    HORNET_DEFINE_JNI_STUB(ReleaseStringCritical),
    HORNET_DEFINE_JNI_STUB(NewWeakGlobalRef),
    HORNET_DEFINE_JNI_STUB(DeleteWeakGlobalRef),
    HORNET_DEFINE_JNI(ExceptionCheck),
    HORNET_DEFINE_JNI_STUB(NewDirectByteBuffer),
    HORNET_DEFINE_JNI_STUB(GetDirectBufferAddress),
    HORNET_DEFINE_JNI_STUB(GetDirectBufferCapacity),
    HORNET_DEFINE_JNI_STUB(GetObjectRefType),
};
