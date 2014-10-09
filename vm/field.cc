#include <hornet/vm.hh>

namespace hornet {

field::field(struct klass* klass_)
    : klass(klass_)
{
}

field::~field()
{
}

bool field::matches(std::string n, std::string d)
{
    return name == n && descriptor == d;
}

}
