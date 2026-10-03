#include "replay/FieldWriter.hpp"

namespace ugh::replay {

void FieldWriter::visit(const char* name, const Field& field) {
    if (std::optional<std::string> text = field.text()) out_.emplace_back(prefix_ + name, *text);
}

}  // namespace ugh::replay
