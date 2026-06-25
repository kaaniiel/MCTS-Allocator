#ifndef IPOLITIC_HPP
#define IPOLITIC_HPP

class IPolitic
{
public:
    virtual ~IPolitic() = default;

    virtual double get_ratio(const int &height) = 0;
};
#endif // IPOLITIC_HPP