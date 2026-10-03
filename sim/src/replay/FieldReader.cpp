#include "replay/FieldReader.hpp"

namespace ugh::replay {

void FieldReader::visit(const char* name, const Field& field) {
    if (result_ != Result::NotAField || name_ != name) return;
    result_ = field.read(text_) ? Result::Set : Result::BadValue;
}

}  // namespace ugh::replay
