#include <libgen.h>
#include <cstdlib>
#include <cstring>
#include <jni.h>

static const char *program;

static void usage()
{
    fprintf(stderr,
            "usage: %s [-options] class [args...]\n",
            program
           );

    exit(EXIT_FAILURE);
}

static int vm_run(JavaVM *vm, JNIEnv *env, char *initial_class_name, int argc, char *argv[])
{
    jmethodID main_method;
    jclass initial_class;
    jobjectArray args;

    initial_class = env->FindClass(initial_class_name);

    if (env->ExceptionCheck()) {
        fprintf(stderr, "error: Cannot find or load initial class '%s'.\n", initial_class_name);

        return EXIT_FAILURE;
    }

    main_method = env->GetStaticMethodID(initial_class, "main", "([Ljava/lang/String;)V");

    if (env->ExceptionCheck()) {
        fprintf(stderr, "error: Main method not found in class '%s'.\n", initial_class_name);

        return EXIT_FAILURE;
    }

    jclass string_class = env->FindClass("java/lang/String");

    if (env->ExceptionCheck())
        goto caught_exception;

    args = env->NewObjectArray(argc, string_class, nullptr);

    if (env->ExceptionCheck())
        goto caught_exception;

#if 0
    for (auto i = 0; i < argc; i++) {
        auto value = env->NewStringUTF(argv[i]);

        if (env->ExceptionCheck())
            goto caught_exception;

        env->SetObjectArrayElement(args, i, value);

        if (env->ExceptionCheck())
            goto caught_exception;
    }
#endif

    env->CallStaticVoidMethod(initial_class, main_method, args);
    if (env->ExceptionCheck())
        goto caught_exception;

    return EXIT_SUCCESS;

caught_exception:

    fprintf(stderr, "error: Exception caught during VM startup.\n");

    return EXIT_FAILURE;
}

int main(int argc, char *argv[])
{
    JavaVMInitArgs vm_args;
    JNIEnv *env;
    JavaVM *vm;

    program = basename(argv[0]);

    vm_args.version = JNI_VERSION_1_6;

    if (JNI_GetDefaultJavaVMInitArgs(&vm_args) != JNI_OK) {
        fprintf(stderr, "error: Cannot get default VM init arguments.\n");
        exit(EXIT_FAILURE);
    }

    JavaVMOption options[argc - 1];
    int idx;

    for (idx = 1; idx < argc; idx++) {
        if (*argv[idx] != '-')
            break;

        if (!strcmp(argv[idx], "-classpath") || !strcmp(argv[idx], "-cp")) {
            options[idx-1].optionString = argv[idx];
            idx++;
            if (idx >= argc) {
                break;
            }
            options[idx-1].optionString = argv[idx];
            continue;
        }

        options[idx-1].optionString = argv[idx];
    }

    vm_args.nOptions = idx - 1;
    vm_args.options  = options;

    if (idx == argc)
        usage();

    auto initial_class_name = argv[idx];

    if (JNI_CreateJavaVM(&vm, reinterpret_cast<void **>(&env), &vm_args) != JNI_OK) {
        fprintf(stderr, "error: Cannot create a virtual machine.\n");
        exit(EXIT_FAILURE);
    }

    auto retval = vm_run(vm, env, initial_class_name, argc - 1, argv + 1);

    vm->DestroyJavaVM();

    return retval;
}
