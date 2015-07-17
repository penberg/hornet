#include "hornet/vm.hh"

#include "hornet/java.hh"

namespace hornet {

object::object(struct klass* klass_)
    : fwd(nullptr)
    , klass(klass_)
    , _fields()
{
    assert(klass_ != nullptr);
    _fields.resize(klass_->nr_object_fields());
}

object::~object()
{
}

}
