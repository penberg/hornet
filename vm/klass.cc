#include "hornet/vm.hh"

#include <string>

namespace hornet {

klass::klass(const method_list_type &methods)
    : _methods(methods)
{
}

klass::~klass()
{
}

std::shared_ptr<method> klass::lookup_method(std::string name, std::string descriptor)
{
    for (auto method : _methods) {
        if (method->matches(name, descriptor))
            return method;
    }

    return nullptr;
}

};
