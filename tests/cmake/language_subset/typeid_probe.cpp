#include <typeinfo>

struct PolymorphicProbe
{
    virtual ~PolymorphicProbe() = default;
};

bool isExactlyPolymorphicProbe(const PolymorphicProbe& probe)
{
    return typeid(probe) == typeid(PolymorphicProbe);
}
