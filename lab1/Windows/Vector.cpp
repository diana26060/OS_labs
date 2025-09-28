#include "Vector.h"
#include <cmath>

Vector::Vector(Number x, Number y) : x(x), y(y) {}

Number Vector::getX() const {
    return x;
}

Number Vector::getY() const {
    return y;
}

void Vector::setX(Number x) {
    this->x = x;
}

void Vector::setY(Number y) {
    this->y = y;
}

Number Vector::getR() const {
    return createNumber(sqrt(x.getValue() * x.getValue() + y.getValue() * y.getValue()));
}

Number Vector::getPhi() const {
    return createNumber(atan2(y.getValue(), x.getValue()));
}

Vector Vector::operator+(const Vector& other) const {
    return Vector(x + other.x, y + other.y);
}

const Vector zeroVector(zero, zero);
const Vector oneVector(one, one);