#include <hornet/vm.hh>

namespace hornet {

field::field()
    : value(0)
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
