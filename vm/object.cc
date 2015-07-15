#include "hornet/vm.hh"

#include "hornet/java.hh"

namespace hornet {

object::object(struct klass* klass_)
    : fwd(nullptr)
    , klass(klass_)
    , _fields(klass_ ? klass_->nr_fields : 0)
{
    if (bootstrap_done)
        assert(klass_ != nullptr);
}

object::~object()
{
}

}
