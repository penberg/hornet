#include <hornet/vm.hh>

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

}
