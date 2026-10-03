// A raindrop.
#pragma once

namespace ugh::world {

/** A raindrop: its pixel in the background page, which is 384 px wide (64 px more than the screen). */
struct Raindrop {
    static constexpr int PAGE_WIDTH = 384;

    int x = 0, y = 0;

    /**
     * The drop `step` px further down and `step` px with the wind (`direction` -1 or 1). A drop blown past the edge
     * of the page goes on at the other edge one row further down, as the page is one long row of pixels.
     */
    Raindrop blown(int step, int direction) const {
        int pixel = y * PAGE_WIDTH + x + step * PAGE_WIDTH + step * direction;
        return {pixel % PAGE_WIDTH, pixel / PAGE_WIDTH};
    }
};

}  // namespace ugh::world
