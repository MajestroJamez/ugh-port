// The placements of enemies by type.
#pragma once

namespace ugh::data {

struct FlyerPlacement;
struct WalkerPlacement;
struct BlowerPlacement;
struct TreePlacement;

/** Visitor of the enemy placements of a level (a factory makes the right enemy of each). */
class EnemyPlacementVisitor {
public:
    virtual ~EnemyPlacementVisitor() = default;
    virtual void visit(const FlyerPlacement& placement) = 0;
    virtual void visit(const WalkerPlacement& placement) = 0;
    virtual void visit(const BlowerPlacement& placement) = 0;
    virtual void visit(const TreePlacement& placement) = 0;
};

}  // namespace ugh::data
