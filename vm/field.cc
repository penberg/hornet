#include <hornet/vm.hh>

namespace hornet {

field::field(struct klass* klass_)
    : klass(klass_)
    , value(0)
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
