#pragma once

class Number {
private:
    double value;
public:
    Number(double value = 0.0);
    double getValue() const;
    void setValue(double value);

    Number operator+(const Number& other) const;
    Number operator-(const Number& other) const;
    Number operator*(const Number& other) const;
    Number operator/(const Number& other) const;
};


extern const Number zero;
extern const Number one;

Number createNumber(double value);