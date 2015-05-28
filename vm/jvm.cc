#include <hornet/vm.hh>

#include <unordered_map>
#include <mutex>

namespace hornet {

jvm *_jvm;

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
