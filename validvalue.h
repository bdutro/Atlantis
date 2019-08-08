#ifndef VALIDVALUE_CLASS
#define VALIDVALUE_CLASS

#include <stdexcept>

template<typename T>
class ValidValue
{
    private:
        bool valid_;
        T val_;

    public:
        ValidValue() :
            valid_(false),
            val_()
        {
        }

        explicit ValidValue(T val) :
            valid_(true),
            val_(val)
        {
        }

        ValidValue& operator=(const ValidValue& rhs)
        {
            valid_ = rhs.valid_;
            val_ = rhs.val_;

            return *this;
        }

        ValidValue& operator=(const T& rhs)
        {
            valid_ = true;
            val_ = rhs;

            return *this;
        }

        void invalidate()
        {
            valid_ = false;
            val_ = T();
        }

        bool isValid() const
        {
            return valid_;
        }

        operator T() const
        {
            if(!isValid())
            {
                throw std::runtime_error("Attempted to use an invalid ValidValue");
            }
            return val_;
        }

        bool equals(const ValidValue& rhs) const
        {
            return (valid_ == rhs.valid_) && (val_ == rhs.val_);
        }
};

#endif
