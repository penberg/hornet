#include <hornet/vm.hh>

namespace hornet {

method::method()
{
}

method::~method()
{
    delete[] code;
}

bool method::matches(std::string n, std::string d)
{
    return name == n && descriptor == d;
}

}
