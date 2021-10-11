#ifndef FIELD_ENUM
#define FIELD_ENUM

#include <type_traits>
#include "type_utils.h"

template<typename EnumType>
class FieldEnum {
    private:
        using int_t = std::underlying_type_t<EnumType>;
        int_t val_;

        template<typename ... I>
        inline typename std::enable_if<type_utils::are_same<int_t, I...>::value, bool>::type
        anySet_(const I... rhs) const {
            return val_ & (rhs | ...);
        }

        template<typename ... I>
        inline typename std::enable_if<type_utils::are_same<int_t, I...>::value, bool>::type
        allSet_(const I... rhs) const {
            const int_t mask = (rhs | ...);

            if(mask == 0) {
                return val_ == mask;
            }

            return (val_ & mask) == mask;
        }

        template<typename ... I>
        inline typename std::enable_if<type_utils::are_same<int_t, I...>::value>::type
        set_(const I... rhs) {
            val_ |= (rhs | ...);
        }

        explicit FieldEnum(const int_t val) :
            val_(val)
        {
        }

        template<typename ... E, typename = typename std::enable_if<type_utils::are_same<EnumType, E...>::value>::type>
        FieldEnum(const int_t val, const EnumType next_flag, const E... addl_flags) :
            FieldEnum(val | static_cast<int_t>(next_flag), addl_flags...)
        {
        }

    public:
        explicit FieldEnum(const EnumType val) :
            FieldEnum(static_cast<int_t>(val))
        {
        }

        template<typename ... E, typename = typename std::enable_if<type_utils::are_same<EnumType, E...>::value>::type>
        FieldEnum(const EnumType val, const E... addl_flags) :
            FieldEnum(static_cast<int_t>(val), addl_flags...)
        {
        }

        inline bool isSet(const EnumType rhs) const {
            return val_ & static_cast<int_t>(rhs);
        }

        template<typename ... E>
        inline typename std::enable_if<type_utils::are_same<EnumType, E...>::value, bool>::type
        anySet(const E... rhs) const {
            return anySet_(static_cast<int_t>(rhs)...);
        }

        template<typename ... E>
        inline typename std::enable_if<type_utils::are_same<EnumType, E...>::value, bool>::type
        allSet(const E... rhs) const {
            return allSet_(static_cast<int_t>(rhs)...);
        }

        template<typename ... E>
        inline typename std::enable_if<type_utils::are_same<EnumType, E...>::value>::type
        set(const E... flags) {
            set_(static_cast<int_t>(flags)...);
        }

        inline void clear(const EnumType rhs) {
            val_ &= ~static_cast<int_t>(rhs);
        }
};

#endif
