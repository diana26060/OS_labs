#pragma once




#include "Number.h"

class Vector {
private:
    Number x;
    Number y;
public:
    Vector(Number x, Number y);
    Number getX() const;
    Number getY() const;
    void setX(Number x);
    void setY(Number y);

    Number getR() const;   
    Number getPhi() const; 

    
    Vector operator+(const Vector& other) const;
};

extern const Vector zeroVector;
extern const Vector oneVector;
