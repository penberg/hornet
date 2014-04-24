#include "hornet/vm.hh"

#include "hornet/java.hh"

#include <string>

namespace hornet {

klass::klass(std::shared_ptr<constant_pool> const_pool)
    : object(nullptr)
    , _const_pool(const_pool)
{
}

klass::~klass()
{
}

void klass::add(std::shared_ptr<method> method)
{
    _methods.push_back(method);
}

std::shared_ptr<method> klass::lookup_method(std::string name, std::string descriptor)
{
    klass* klass = this;
    while (klass) {
        for (auto method : klass->_methods) {
            if (method->matches(name, descriptor))
                return method;
        }
        klass = klass->super;
    }
    return nullptr;
}

bool klass::verify()
{
    for (auto method : _methods) {
        if (!verify_method(method))
            return false;
    }
    return true;
}

};
