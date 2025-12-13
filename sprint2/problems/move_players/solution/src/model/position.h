#pragma once

#include <cstdint>

namespace model {

struct Position {
    double x;
    double y;
    
    Position() : x(0.0), y(0.0) {}
    Position(double x, double y) : x(x), y(y) {}
};

struct Speed {
    double vx;
    double vy;
    
    Speed() : vx(0.0), vy(0.0) {}
    Speed(double vx, double vy) : vx(vx), vy(vy) {}
};

enum class Direction {
    NORTH,  // U - вверх
    SOUTH,  // D - вниз
    WEST,   // L - влево
    EAST    // R - вправо
};

} // namespace model


