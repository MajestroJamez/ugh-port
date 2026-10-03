// The enemies by type.
#pragma once

namespace ugh::enemies {

namespace flyer {
class Flyer;
}
namespace walker {
class Walker;
}
namespace blower {
class Blower;
}
namespace tree {
class Tree;
}

/** Visitor of the enemies by their type (the replay fields, a renderer). */
class EnemyVisitor {
public:
    virtual ~EnemyVisitor() = default;
    virtual void visit(const flyer::Flyer& enemy) = 0;
    virtual void visit(const walker::Walker& enemy) = 0;
    virtual void visit(const blower::Blower& enemy) = 0;
    virtual void visit(const tree::Tree& enemy) = 0;
};

}  // namespace ugh::enemies
