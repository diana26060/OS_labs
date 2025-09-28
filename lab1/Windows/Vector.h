#pragma once

#ifdef VECTOR_EXPORTS
#define VECTOR_API __declspec(dllexport)
#else
#define VECTOR_API __declspec(dllimport)
#endif


#include "Number.h"

class VECTOR_API Vector {
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

extern VECTOR_API const Vector zeroVector;
extern VECTOR_API const Vector oneVector;