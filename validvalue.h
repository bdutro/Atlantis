#ifndef VALIDVALUE_CLASS
#define VALIDVALUE_CLASS

#include <stdexcept>

template<typename T>
class ValidValue
{
    private:
        bool valid_;

    protected:
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

        constexpr ValidValue(const ValidValue& rhs) = default;

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

        virtual bool isValid() const
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

        bool operator==(const ValidValue& rhs) const
        {
            return (valid_ == rhs.valid_) && (val_ == rhs.val_);
        }

        bool operator!=(const ValidValue& rhs) const
        {
            return !(*this == rhs);
        }

        ValidValue& operator++()
        {
            if(!isValid())
            {
                throw std::runtime_error("Attempted to use an invalid ValidValue");
            }
            ++val_;
            return *this;
        }

        ValidValue operator++(int)
        {
            ValidValue tmp = *this;
            ++(*this);
            return tmp;
        }
};

#endif
