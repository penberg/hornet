#include <hornet/vm.hh>

namespace hornet {

jvm *_jvm;

void jvm::register_klass(std::shared_ptr<klass> klass)
{
    std::lock_guard<std::mutex> lock(_mutex);

    _klasses.push_back(klass);
}

}
