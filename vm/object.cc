#include "hornet/vm.hh"

#include "hornet/java.hh"

namespace hornet {

object::object(struct klass* klass_)
    : fwd(nullptr)
    , klass(klass_)
    , instance_values(klass_ ? klass_->nr_fields : 0)
{
}

object::~object()
{
}

}
