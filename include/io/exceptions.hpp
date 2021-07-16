/*
 *
 */
#include <stdexcept>
class IOException: public std::runtime_error
{
    public:
        IOException() : runtime_error("IO Error") {}
};
