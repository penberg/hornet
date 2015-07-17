#include <hornet/vm.hh>

#include <hornet/java.hh>

#include <unordered_map>
#include <mutex>

namespace hornet {

bool bootstrap_done = false;
std::shared_ptr<klass> java_lang_Class;
std::shared_ptr<klass> java_lang_String;
jvm *_jvm;

void jvm::init()
{
    prim_pre_init();
    java_lang_Class = hornet::system_loader()->load_class("java/lang/Class");
    if (!java_lang_Class) {
        throw std::runtime_error("Unable to look up java/lang/Class");
    }
    java_lang_String = hornet::system_loader()->load_class("java/lang/String");
    if (!java_lang_String) {
        throw std::runtime_error("Unable to look up java/lang/String");
    }
    bootstrap_done = true;
    java_lang_Class->link();
    java_lang_String->link();
    prim_post_init();
}

std::shared_ptr<klass> jvm::lookup_class(std::string name)
{
    auto it = _classes.find(name);
    if (it != _classes.end()) {
        return it->second;
    }
    return nullptr;
}

void jvm::register_class(std::shared_ptr<klass> klass)
{
    _classes.insert({klass->name, klass});
}

string* jvm::intern_string(std::string str)
{
    std::lock_guard<std::mutex> lock(_intern_mutex);
    auto it = _intern.find(str);
    if (it == _intern.end()) {
        auto intern = std::make_shared<string>(str.c_str());
        _intern.insert({str, intern});
        return intern.get();
    }
    return it->second.get();
}

}
