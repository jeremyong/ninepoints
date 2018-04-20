#pragma once

#include <array>
#include <tuple>
#include <vulkan/vulkan.hpp>

// Usage
// The ShaderSpecialization template class is used to provide specialization constants
// to Vulkan shaders at pipeline creation time in a typesafe manner

// Define a specialization map containing 2 integers and a float
// npts::ShaderSpecialization<int, int, float> sp;

// Assign some values
// sp.set<0>(4);
// sp.set<1>(1);
// sp.set<2>(93.2f);

// Access them if you want
// std::cout << sp.get<0>() << std::endl;

// Use this to create your graphics or compute pipeline
// The data will be mapped to constant ids 0, 1, and 2 respectively
// vk::SpecializationInfo& info = sp.info();

namespace npts
{

template <typename... Ts>
class ShaderSpecialization
{
private:
    template <typename T>
    static constexpr size_t size_helper()
    {
        static_assert(std::is_scalar<T>::value,
                      "Data put into specialization maps must be scalars");
        return sizeof(T);
    }

    template <typename T1, typename T2, typename... More>
    static constexpr size_t size_helper()
    {
        static_assert(std::is_scalar<T1>::value,
                      "Data put into specialization maps must be scalars");
        return sizeof(T1) + size_helper<T2, More...>();
    }

    template <size_t Index, size_t Max, size_t Offset>
    static constexpr size_t offset_of_helper()
    {
        if constexpr (Index == Max)
        {
            return Offset;
        }
        else
        {
            return offset_of_helper<Index + 1, Offset + sizeof(type<Index>)>();
        }
    }

public:
    static constexpr size_t count = sizeof...(Ts);
    static constexpr size_t size = size_helper<Ts...>();

    template <size_t N>
    using type = typename std::tuple_element<N, std::tuple<Ts...>>::type;

    template <size_t N>
    static constexpr size_t offset_of = offset_of_helper<0, N, 0>();

    vk::SpecializationInfo& info()
    {
        return m_info;
    }

    template <size_t N>
    auto get() const
    {
        return *reinterpret_cast<const type<N>*>(m_data.data() + offset_of<N>);
    }

    template <size_t N>
    void set(type<N> value)
    {
        *reinterpret_cast<type<N>*>(m_data.data() + offset_of<N>) = value;
    }

    ShaderSpecialization()
    {
        construct_helper<0>();
        m_info = vk::SpecializationInfo{
            count,                                 // Map entry count
            m_entries.data(),                      // Map entries
            static_cast<vk::DeviceSize>(size),     // Data size
            reinterpret_cast<void*>(m_data.data()) // Data
        };
    }

    ShaderSpecialization(ShaderSpecialization&& other)
        : m_entries{std::move(other.m_entries)}
        , m_data{std::move(other.m_data)}
    {
        m_info = vk::SpecializationInfo{
            count,                                 // Map entry count
            m_entries.data(),                      // Map entries
            static_cast<vk::DeviceSize>(size),     // Data size
            reinterpret_cast<void*>(m_data.data()) // Data
        };
    }

    ShaderSpecialization(const ShaderSpecialization& other)
        : m_entries{other.m_entries}
        , m_data{other.m_data}
    {
        m_info = vk::SpecializationInfo{
            count,                                 // Map entry count
            m_entries.data(),                      // Map entries
            static_cast<vk::DeviceSize>(size),     // Data size
            reinterpret_cast<void*>(m_data.data()) // Data
        };
    }

    ShaderSpecialization& operator=(const ShaderSpecialization& other)
    {
        if (this != &other)
        {
            // We are careful not to copy the info and entry data
            m_data = other.m_data;
        }
        return *this;
    }

    ShaderSpecialization& operator=(ShaderSpecialization&& other)
    {
        if (this != &other)
        {
            // We are careful not to move the info and entry data
            m_data = std::move(other.m_data);
        }
        return *this;
    }

private:
    template <size_t Index>
    void construct_helper()
    {
        if constexpr (Index == count)
        {
            return;
        }
        else
        {
            m_entries[Index] = vk::SpecializationMapEntry{
                Index,              // Constant ID
                offset_of<Index>,   // Offset
                sizeof(type<Index>) // Size
            };
            construct_helper<Index + 1>();
        }
    }

    vk::SpecializationInfo m_info;
    std::array<vk::SpecializationMapEntry, count> m_entries;
    std::array<uint8_t, size> m_data;
};
}

