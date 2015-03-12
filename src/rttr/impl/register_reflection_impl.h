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

#ifndef RTTR_REGISTER_REFLECTION_IMPL_H_
#define RTTR_REGISTER_REFLECTION_IMPL_H_

#include "rttr/detail/constructor_container.h"
#include "rttr/detail/destructor_container.h"
#include "rttr/detail/enumeration_container.h"
#include "rttr/detail/method_container.h"
#include "rttr/detail/property_container.h"
#include "rttr/detail/accessor_type.h"
#include "rttr/detail/misc_type_traits.h"
#include "rttr/detail/utility.h"

#include <type_traits>
#include <memory>

namespace rttr
{

namespace impl
{
// helper method to store the metadata
template<typename T>
void store_metadata(T& obj, std::vector<rttr::metadata> data)
{
    for (auto& item : data)
    {
        auto key    = item.get_key();
        auto value  = item.get_value();
        if (key.is_type<int>())
            obj->set_metadata(key.get_value<int>(), std::move(value));
        else if (key.is_type<std::string>())
            obj->set_metadata(std::move(key.get_value<std::string>()), std::move(value));
    }
}

} // end namespace impl

/////////////////////////////////////////////////////////////////////////////////////////

namespace impl
{

template<typename ClassType, typename... Args>
void constructor_impl(std::vector< rttr::metadata > data)
{
    using namespace std;
    const type type = type::get<ClassType>();
    auto ctor = detail::make_unique<detail::constructor_container<ClassType, Args...>>();
    // register the type with the following call:
    ctor->get_instanciated_type();
    ctor->get_parameter_types();

    impl::store_metadata(ctor, std::move(data));
    impl::register_constructor(type, move(ctor));
    impl::register_destructor(type, detail::make_unique<detail::destructor_container<ClassType>>());
    
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType, typename A, typename Policy>
void property_impl(const std::string& name, A accessor, std::vector<rttr::metadata> data, const Policy&)
{
    using namespace std;
    using getter_policy = typename detail::get_getter_policy<Policy>::type;
    using setter_policy = typename detail::get_setter_policy<Policy>::type;
    using acc_type      = typename detail::property_type<A>::type;

    auto prop = detail::make_unique<detail::property_container<acc_type, A, void, getter_policy, setter_policy>>(name, type::get<ClassType>(), accessor);
    // register the type with the following call:
    prop->get_type();
    impl::store_metadata(prop, std::move(data));
    impl::register_property(type::get<ClassType>(), move(prop));
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType, typename A1, typename A2, typename Policy>
void property_impl(const std::string& name, A1 getter, A2 setter, std::vector<rttr::metadata> data, const Policy&)
{
    using namespace std;
    using getter_policy = typename detail::get_getter_policy<Policy>::type;
    using setter_policy = typename detail::get_setter_policy<Policy>::type;
    using acc_type      = typename detail::property_type<A1>::type;

    auto prop = detail::make_unique<detail::property_container<acc_type, A1, A2, getter_policy, setter_policy>>(name, type::get<ClassType>(), getter, setter);
    // register the type with the following call:
    prop->get_type();
    impl::store_metadata(prop, std::move(data));
    impl::register_property(type::get<ClassType>(), move(prop));
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType, typename A, typename Policy>
void property_readonly_impl(const std::string& name, A accessor, std::vector<rttr::metadata> data, const Policy&)
{
    using namespace std;
    using getter_policy = typename detail::get_getter_policy<Policy>::type;
    using setter_policy = detail::read_only;
    using acc_type      = typename detail::property_type<A>::type;

    auto prop = detail::make_unique<detail::property_container<acc_type, A, void, getter_policy, setter_policy>>(name, type::get<ClassType>(), accessor);
    // register the type with the following call:
    prop->get_type();
    impl::store_metadata(prop, std::move(data));
    impl::register_property(type::get<ClassType>(), move(prop));
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType, typename F, typename Policy>
void method_impl(const std::string& name, F function, std::vector< rttr::metadata > data, const Policy&)
{
    using namespace std;
    using method_policy = typename detail::get_method_policy<Policy>::type;

    auto meth = detail::make_unique<detail::method_container<F, method_policy>>(name, type::get<ClassType>(), function);
    // register the underlying type with the following call:
    meth->get_return_type();
    meth->get_parameter_types();
    impl::store_metadata(meth, std::move(data));
    impl::register_method(type::get<ClassType>(), move(meth));
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType, typename EnumType>
void enumeration_impl(std::vector< std::pair< std::string, EnumType> > enum_data, std::vector<rttr::metadata> data)
{
    using namespace std;
    static_assert(is_enum<EnumType>::value, "No enum type provided, please call this method with an enum type!");
    type declar_type = impl::get_invalid_type();
    if (!std::is_same<ClassType, void>::value)
        declar_type = type::get<ClassType>();

    auto enum_item = detail::make_unique<detail::enumeration_container<EnumType>>(declar_type, move(enum_data));
    // register the underlying type with the following call:
    enum_item->get_type();

    impl::store_metadata(enum_item, std::move(data));
    impl::register_enumeration(type::get<EnumType>(), move(enum_item));
}

}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
void constructor_(std::vector< rttr::metadata > data)
{
    impl::constructor_impl<T>(std::move(data));
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A>
void property_(const std::string& name, A acc)
{
    using namespace std;
    static_assert(is_pointer<A>::value, "No valid property accessor provided!");
    impl::property_impl<void, A>(name, acc, std::vector< rttr::metadata >(),
                                 detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A>
void property_(const std::string& name, A acc, std::vector< rttr::metadata > data)
{
    using namespace std;
    static_assert(is_pointer<A>::value, "No valid property accessor provided!");
    impl::property_impl<void, A>(name, acc, std::move(data), detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A, typename Policy>
void property_(const std::string& name, A acc, 
               const Policy& policy, typename std::enable_if<detail::contains<Policy, detail::policy_list>::value>::type*)
{
    using namespace std;
    static_assert(is_pointer<A>::value, "No valid property accessor provided!");
    impl::property_impl<void, A>(name, acc, std::vector< rttr::metadata >(),
                                 policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A, typename Policy>
void property_(const std::string& name, A acc, std::vector< rttr::metadata > data,
               const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A>::value, "No valid property accessor provided!");
    impl::property_impl<void, A>(name, acc, std::move(data), 
                                 policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A1, typename A2>
void property_(const std::string& name, A1 getter, A2 setter, 
               typename std::enable_if<!detail::contains<A2, detail::policy_list>::value>::type*)
{
    using namespace std;
    static_assert(is_pointer<A1>::value || is_pointer<A2>::value ||
                  detail::is_function_ptr<A1>::value || detail::is_function_ptr<A2>::value ||
                  detail::is_std_function<A1>::value || detail::is_std_function<A2>::value, 
                  "No valid property accessor provided!");
    impl::property_impl<void, A1, A2>(name, getter, setter, std::vector< rttr::metadata >(),
                                      detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A1, typename A2>
void property_(const std::string& name, A1 getter, A2 setter, std::vector< rttr::metadata > data)
{
    using namespace std;
    static_assert(is_pointer<A1>::value || is_pointer<A2>::value ||
                  detail::is_function_ptr<A1>::value || detail::is_function_ptr<A2>::value ||
                  detail::is_std_function<A1>::value || detail::is_std_function<A2>::value, 
                  "No valid property accessor provided!");
    impl::property_impl<void, A1, A2>(name, getter, setter, move(data), 
                                      detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A1, typename A2, typename Policy>
void property_(const std::string& name, A1 getter, A2 setter,
               const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A1>::value || is_pointer<A2>::value ||
                  detail::is_function_ptr<A1>::value || detail::is_function_ptr<A2>::value ||
                  detail::is_std_function<A1>::value || detail::is_std_function<A2>::value, 
                  "No valid property accessor provided!");
    impl::property_impl<void, A1, A2>(name, getter, setter, std::vector< rttr::metadata >(),
                                      policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A1, typename A2, typename Policy>
void property_(const std::string& name, A1 getter, A2 setter, std::vector< rttr::metadata > data,
               const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A1>::value || is_pointer<A2>::value ||
                  detail::is_function_ptr<A1>::value || detail::is_function_ptr<A2>::value ||
                  detail::is_std_function<A1>::value || detail::is_std_function<A2>::value, 
                  "No valid property accessor provided!");
    impl::property_impl<void, A1, A2>(name, getter, setter, std::move(data),
                                      policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A>
void property_readonly_(const std::string& name, A acc)
{
    using namespace std;
    static_assert(is_pointer<A>::value || detail::is_function_ptr<A>::value || detail::is_std_function<A>::value,
                  "No valid property accessor provided!");
    impl::property_readonly_impl<void, A>(name, acc, std::vector< rttr::metadata >(),
                                          detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A>
void property_readonly_(const std::string& name, A acc, std::vector< rttr::metadata > data)
{
    using namespace std;
    static_assert(is_pointer<A>::value || detail::is_function_ptr<A>::value || detail::is_std_function<A>::value,
                  "No valid property accessor provided!");
    impl::property_readonly_impl<void, A>(name, acc, std::move(data),
                                          detail::default_property_policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A, typename Policy>
void property_readonly_(const std::string& name, A acc, const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A>::value || detail::is_function_ptr<A>::value || detail::is_std_function<A>::value,
                  "No valid property accessor provided!");
    impl::property_readonly_impl<void, A>(name, acc, std::vector< rttr::metadata >(),
                                          policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename A, typename Policy>
void property_readonly_(const std::string& name, A acc, std::vector< rttr::metadata > data, const Policy& policy)
{
    using namespace std;
    static_assert(is_pointer<A>::value || detail::is_function_ptr<A>::value || detail::is_std_function<A>::value,
                  "No valid property accessor provided!");
    impl::property_readonly_impl<void, A>(name, acc, std::move(data),
                                          policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename F>
void method_(const std::string& name, F function)
{
    impl::method_impl<void>(name, function, std::vector< rttr::metadata >(), detail::default_invoke());
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename F>
void method_(const std::string& name, F function, std::vector< rttr::metadata > data)
{
    impl::method_impl<void>(name, function, std::move(data), detail::default_invoke());
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename F, typename Policy>
void method_(const std::string& name, F function, const Policy& policy)
{
    impl::method_impl<void>(name, function, std::vector< rttr::metadata >(), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename F, typename Policy>
void method_(const std::string& name, F function, std::vector< rttr::metadata > data, const Policy& policy)
{
    impl::method_impl<void>(name, function, std::move(data), policy);
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename EnumType>
void enumeration_(std::vector< std::pair< std::string, EnumType> > enum_data, std::vector<rttr::metadata> data)
{
    impl::enumeration_impl<void, EnumType>(std::move(enum_data), std::move(data));
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
class_<ClassType>::class_(std::string name, std::vector< rttr::metadata > data)
{
    auto t = type::get<ClassType>();
    
    if (!name.empty())
        impl::register_custom_name(t, std::move(name));

    impl::register_metadata(t, std::move(data));

    static_assert(std::is_class<ClassType>::value, "Reflected type is not a class or struct.");
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
class_<ClassType>::~class_()
{
    // make sure that all base classes are registered
    detail::base_classes<ClassType>::get_types();
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename... Args>
class_<ClassType>& class_<ClassType>::constructor(std::vector<rttr::metadata> data)
{
    impl::constructor_impl<ClassType, Args...>(std::move(data));
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A acc)
{
    static_assert(std::is_member_object_pointer<A>::value ||
                  std::is_pointer<A>::value,
                  "No valid property accessor provided!");

    impl::property_impl<ClassType, A>(name, acc, std::vector<rttr::metadata>(),
                                      detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A acc, std::vector<rttr::metadata> data)
{
    static_assert(std::is_member_object_pointer<A>::value ||
                  std::is_pointer<A>::value,
                  "No valid property accessor provided!");
    impl::property_impl<ClassType, A>(name, acc, std::move(data), 
                                      detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A, typename Policy>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A acc, const Policy& policy, 
                                               typename std::enable_if<detail::contains<Policy, detail::policy_list>::value>::type*)
{
    static_assert(std::is_member_object_pointer<A>::value ||
                  std::is_pointer<A>::value,
                  "No valid property accessor provided!");
    impl::property_impl<ClassType, A>(name, acc, std::vector<rttr::metadata>(), 
                                      policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A, typename Policy>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A acc, std::vector<rttr::metadata> data,
                                               const Policy& policy)
{
    static_assert(std::is_member_object_pointer<A>::value ||
                  std::is_pointer<A>::value,
                  "No valid property accessor provided!");
    impl::property_impl<ClassType, A>(name, acc, std::move(data),
                                      policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A1, typename A2>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A1 getter, A2 setter,
                                               typename std::enable_if<!detail::contains<A2, detail::policy_list>::value>::type*)
{
    impl::property_impl<ClassType, A1, A2>(name, getter, setter, std::vector<rttr::metadata>(),
                                           detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A1, typename A2>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A1 getter, A2 setter, std::vector<rttr::metadata> data)
{
    impl::property_impl<ClassType, A1, A2>(name, getter, setter, std::move(data),
                                           detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A1, typename A2, typename Policy>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A1 getter, A2 setter, const Policy& policy)
{
    impl::property_impl<ClassType, A1, A2>(name, getter, setter, std::vector<rttr::metadata>(), 
                                           policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A1, typename A2, typename Policy>
class_<ClassType>& class_<ClassType>::property(const std::string& name, A1 getter, A2 setter, 
                                               std::vector<rttr::metadata> data, const Policy& policy)
{
    impl::property_impl<ClassType, A1, A2>(name, getter, setter, std::move(data),
                                           policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A>
class_<ClassType>& class_<ClassType>::property_readonly(const std::string& name, A acc)
{
    impl::property_readonly_impl<ClassType, A>(name, acc, std::vector<rttr::metadata>(),
                                               detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A>
class_<ClassType>& class_<ClassType>::property_readonly(const std::string& name, A acc, std::vector<rttr::metadata> data)
{
    impl::property_readonly_impl<ClassType, A>(name, acc, std::move(data),
                                               detail::default_property_policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A, typename Policy>
class_<ClassType>& class_<ClassType>::property_readonly(const std::string& name, A acc, const Policy& policy)
{
    impl::property_readonly_impl<ClassType, A>(name, acc, std::vector<rttr::metadata>(),
                                               policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename A, typename Policy>
class_<ClassType>& class_<ClassType>::property_readonly(const std::string& name, A acc, std::vector<rttr::metadata> data,
                                                        const Policy& policy)
{
    impl::property_readonly_impl<ClassType, A>(name, acc, std::move(data),
                                               policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename F>
class_<ClassType>& class_<ClassType>::method(const std::string& name, F function)
{
    impl::method_impl<ClassType>(name, function, std::vector< rttr::metadata >(), detail::default_invoke());
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename F>
class_<ClassType>& class_<ClassType>::method(const std::string& name, F function, std::vector< rttr::metadata > data)
{
    impl::method_impl<ClassType>(name, function, std::move(data), detail::default_invoke());
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename F, typename Policy>
class_<ClassType>& class_<ClassType>::method(const std::string& name, F function, const Policy& policy)
{
    impl::method_impl<ClassType>(name, function, std::vector< rttr::metadata >(), policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename F, typename Policy>
class_<ClassType>& class_<ClassType>::method(const std::string& name, F function, std::vector< rttr::metadata > data, const Policy& policy)
{
    impl::method_impl<ClassType>(name, function, std::move(data), policy);
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////

template<typename ClassType>
template<typename EnumType>
class_<ClassType>& class_<ClassType>::enumeration(std::vector< std::pair< std::string, EnumType> > enum_data, std::vector<rttr::metadata> data)
{
    impl::enumeration_impl<ClassType, EnumType>(std::move(enum_data), std::move(data));
    return *this;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// register_global

/////////////////////////////////////////////////////////////////////////////////////////


} // end namespace rttr


#define RTTR_REGISTER                                                   \
static void rttr__auto_register_reflection_function__();                \
namespace                                                               \
{                                                                       \
    struct rttr__auto__register__                                       \
    {                                                                   \
        rttr__auto__register__()                                        \
        {                                                               \
            rttr__auto_register_reflection_function__();                \
        }                                                               \
    };                                                                  \
}                                                                       \
static const rttr__auto__register__ RTTR_CAT(auto_register__,__LINE__); \
static void rttr__auto_register_reflection_function__()

#endif // RTTR_REGISTER_REFLECTION_IMPL_H_