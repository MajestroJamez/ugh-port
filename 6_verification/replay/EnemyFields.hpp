// The fields of the enemies.
#pragma once

#include "enemies/EnemyVisitor.hpp"
#include "replay/Fields.hpp"

namespace ugh::replay {

/** The enemy.N.* fields of an enemy, by its type and state (Visitor of the enemies). */
class EnemyFields : public enemies::EnemyVisitor {
public:
    explicit EnemyFields(Fields& fields) : fields_(fields) {}

    void visit(const enemies::flyer::Flyer& flyer) override;
    void visit(const enemies::walker::Walker& walker) override;
    void visit(const enemies::blower::Blower& blower) override;
    void visit(const enemies::tree::Tree& tree) override;

private:
    Fields& fields_;
};

}  // namespace ugh::replay
