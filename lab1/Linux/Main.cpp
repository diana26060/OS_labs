#include <iostream>
#include "Number.h"
#include "Vector.h"

int main() {
    //работа с числами
    Number a = createNumber(5.0);
    Number b = createNumber(3.0);

    Number sum = a + b;
    Number diff = a - b;
    Number prod = a * b;
    Number quot = a / b;

    std::cout << "a = " << a.getValue() << ", b = " << b.getValue() << std::endl;
    std::cout << "a + b = " << sum.getValue() << std::endl;
    std::cout << "a - b = " << diff.getValue() << std::endl;
    std::cout << "a * b = " << prod.getValue() << std::endl;
    std::cout << "a / b = " << quot.getValue() << std::endl;

    // работа с векторами
    Vector v1(createNumber(1.0), createNumber(0.0));
    Vector v2(createNumber(0.0), createNumber(1.0));
    Vector v3 = v1 + v2;

    std::cout << "v1: (" << v1.getX().getValue() << ", " << v1.getY().getValue() << ")" << std::endl;
    std::cout << "v2: (" << v2.getX().getValue() << ", " << v2.getY().getValue() << ")" << std::endl;
    std::cout << "v1 + v2: (" << v3.getX().getValue() << ", " << v3.getY().getValue() << ")" << std::endl;

    std::cout << "Polar coordinates of v3:" << std::endl;
    std::cout << "r = " << v3.getR().getValue() << ", phi = " << v3.getPhi().getValue() << std::endl;

    // глобальные переменные
    std::cout << "Global zero vector: (" << zeroVector.getX().getValue() << ", " << zeroVector.getY().getValue() << ")" << std::endl;
    std::cout << "Global one vector: (" << oneVector.getX().getValue() << ", " << oneVector.getY().getValue() << ")" << std::endl;

    return 0;
}