/************************************************************************************
*                                                                                   *
*   Copyright (c) 2014 Axel Menzel <info@axelmenzel.de>                             *
*                                                                                   *
*   This file is part of RTTR (Run Time Type Reflection)                            *
*   License: MIT License                                                            *
*                                                                                   *
*   Permission is hereby granted, free of charge, to any person obtaining           *
*   a copy of this software and associated documentation files (the "Software"),    *
*   to deal in the Software without restriction, including without limitation       *
*   the rights to use, copy, modify, merge, publish, distribute, sublicense,        *
*   and/or sell copies of the Software, and to permit persons to whom the           *
*   Software is furnished to do so, subject to the following conditions:            *
*                                                                                   *
*   The above copyright notice and this permission notice shall be included in      *
*   all copies or substantial portions of the Software.                             *
*                                                                                   *
*   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
*   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
*   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
*   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
*   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
*   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
*   SOFTWARE.                                                                       *
*                                                                                   *
*************************************************************************************/

#ifndef RTTR_REFLECTION_DATABASE_P_H_
#define RTTR_REFLECTION_DATABASE_P_H_

#include "rttr/type.h"
#include "rttr/detail/metadata_container.h"

#include <unordered_map>
#include <vector>
#include <string>
#include <memory>
#include <mutex>

#define RTTR_MAX_TYPE_COUNT 32767
#define RTTR_MAX_INHERIT_TYPES_COUNT 50

namespace rttr
{
namespace detail
{

/*!
 * This class holds all reflection and type data.
 * It is not part of the rttr API
 */
class RTTR_LOCAL reflection_database
{
    public:
        reflection_database() {};
        static reflection_database& instance() { static reflection_database obj; return obj; }

        typedef void*(*rttr_cast_func)(void*);
        typedef derived_info(*get_derived_info_func)(void*);


        struct hash_char
        {
            RTTR_INLINE std::size_t operator() (const char* ptr) const
            {
                const std::size_t  magic_prime = 0x01000193;
                std::size_t        hash        = 0xcbf29ce4;

                for (; *ptr; ++ptr)
                  hash = (hash ^ *ptr) * magic_prime;

                return hash;
            }
        };

        struct comparison_char
        {
            RTTR_INLINE bool operator() (const char* str1, const char* str2) const
            {
                for (; *str1 && *str1 == *str2; ++str1, ++str2) {}
                    return (*str1 == *str2);
            }
        };


        typedef std::unordered_map<const char*, const rttr::type::type_id, hash_char, comparison_char>  NameToTag;
        typedef std::vector< std::unique_ptr< property_container_base>>                                 property_container;
        typedef std::vector< std::unique_ptr< constructor_container_base>>                              constructor_container;
        typedef std::vector< std::unique_ptr< constructor_container_base>>                              destructor_container;
        typedef std::vector< std::unique_ptr< method_container_base>>                                   method_container;

        typedef std::vector< std::unique_ptr< constructor_container_base>>                              ctor_list;
        typedef std::unordered_map< std::string, property_container::size_type>                         property_map;
        typedef std::unordered_multimap< std::string, method_container::size_type>                      method_map;
        typedef std::unordered_multimap< type, std::string>                                             custom_name_map;

        /*!
         * \brief This function returns true, when the given types in \p param_list are the same type like in \p args,
         *         otherwise false.
         */
        static bool does_signature_match_arguments(const std::vector<type>& param_list, const std::vector<type>& args);

        /*!
         * \brief This function return from the given list of arguments \p args a list of corresponding type objects.
         */
        static std::vector<type> extract_types(const std::vector<detail::argument>& args);

        /*!
         * \brief This function returns true, when in \p method_container exists already a method with the same name
         *         and signature like the given \p method.
         */
        static bool is_method_already_registered(const std::unique_ptr<detail::method_container_base>& method, const method_map& method_container);


        static void register_property(std::unique_ptr<detail::property_container_base> prop, property_map& property_container);
        static void register_method(std::unique_ptr<detail::method_container_base> method, method_map& method_container);

        static detail::property_container_base*     find_property(const std::string& name, property_map& property_container);
        static detail::method_container_base*       find_method(const std::string& name, const method_map& method_container);
        static detail::method_container_base*       find_method(const std::string& name, const std::vector<type>& params, const method_map& method_container);

        struct class_data
        {
            void add_constructor(std::unique_ptr<detail::constructor_container_base> ctor);

            reflection_database::ctor_list       m_ctor_list;
            reflection_database::property_map    m_property_map;
            reflection_database::method_map      m_method_map;
        };

    private:
        reflection_database(const reflection_database& other);
        reflection_database& operator=(const reflection_database& other);

    public:
        type::type_id                                       type_id_counter;    // the global incremented id counter
        NameToTag                                           name_to_id;         // a container for mapping the name of a type to its unique id
        const char*                                         name_list[RTTR_MAX_TYPE_COUNT];
        type::type_id                                       base_class_list[RTTR_MAX_TYPE_COUNT * RTTR_MAX_INHERIT_TYPES_COUNT];        // this list contains for every type its base classes
        type::type_id                                       derived_class_list[RTTR_MAX_TYPE_COUNT * RTTR_MAX_INHERIT_TYPES_COUNT];     // this list contains for every type its derived classes
        rttr_cast_func                                      conversion_list[RTTR_MAX_TYPE_COUNT * RTTR_MAX_INHERIT_TYPES_COUNT];        // this list contains for every type a conversion function to its base classes
        detail::variant_create_func                         variant_create_func_list[RTTR_MAX_TYPE_COUNT];                              // this list contains for every type a create function to a variant
        get_derived_info_func                               get_derived_info_func_list[RTTR_MAX_TYPE_COUNT];
        type::type_id                                       raw_type_list[RTTR_MAX_TYPE_COUNT];
        bool                                                is_class_list[RTTR_MAX_TYPE_COUNT];
        bool                                                is_enum_list[RTTR_MAX_TYPE_COUNT];
        bool                                                is_array_list[RTTR_MAX_TYPE_COUNT];
        bool                                                is_pointer_list[RTTR_MAX_TYPE_COUNT];
        bool                                                is_primitive_list[RTTR_MAX_TYPE_COUNT];
        bool                                                is_function_pointer_list[RTTR_MAX_TYPE_COUNT];
        bool                                                is_member_object_pointer_list[RTTR_MAX_TYPE_COUNT];
        bool                                                is_member_function_pointer_list[RTTR_MAX_TYPE_COUNT];
        metadata_container                                  meta_data_list[RTTR_MAX_TYPE_COUNT];
        std::size_t                                         get_pointer_count_list[RTTR_MAX_TYPE_COUNT];
        std::unique_ptr<class_data>                         class_data_list[RTTR_MAX_TYPE_COUNT];
        std::unique_ptr<constructor_container_base>         constructor_list[RTTR_MAX_TYPE_COUNT];
        std::unique_ptr<destructor_container_base>          destructor_list[RTTR_MAX_TYPE_COUNT];
        std::unique_ptr<enumeration_container_base>         enumeration_list[RTTR_MAX_TYPE_COUNT];
        std::vector<std::unique_ptr<type_converter_base>>   type_converter_list[RTTR_MAX_TYPE_COUNT];
        property_map                                        global_properties;
        method_map                                          global_methods;

        constructor_container                               m_constructor_list;
        destructor_container                                m_destructor_list;
        method_container                                    m_method_list;
        property_container                                  m_property_list;
        custom_name_map                                     m_custom_name_map;

        std::mutex                                          m_register_type_mutex;
        std::mutex                                          m_register_custom_name_mutex;
};

} // end namespace detail
} // end namespace rttr

#endif // RTTR_REFLECTION_DATABASE_P_H_