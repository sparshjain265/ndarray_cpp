/**
 * MIT License
 *
 * Copyright (c) 2025 Sparsh Jain
 *
 */

#ifndef INCLUDE_NDARRAY_CPP_SHARED_NDARRAY_HPP
#define INCLUDE_NDARRAY_CPP_SHARED_NDARRAY_HPP

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <concepts>
#include <memory>
#include <numeric>

namespace ND {

    /**************************************************************************/

    // Concepts to Enforce Safety

    template <typename... Ts>
    concept AllIntegral = (std::integral<Ts> && ...);

    template <typename... Ts>
    concept AllSigned = (std::is_signed_v<Ts> && ...);

    template <typename... Ts>
    concept AllUnsigned = (std::is_unsigned_v<Ts> && ...);

    /**************************************************************************/

    using size_type = std::size_t;

    template <size_type NDim>
    using Shape = std::array<size_type, NDim>;

    template <size_type NDim>
    using Stride = std::array<size_type, NDim>;

    // N-Dimensional Array Class
    // Assumes contiguous storage in row-major order
    // Marked as final to prevent inheritance
    // If you want to inherit, make sure you follow the rule of 5
    // and ensure proper cleanup of resources
    template <typename T, size_type NDim>
    class NDArray final {
    public:
        using value_type       = T;
        using size_type        = ND::size_type;
        using shape_size_type  = Shape<NDim>::size_type;
        using stride_size_type = Stride<NDim>::size_type;

    protected:
        std::shared_ptr<T[]> m_owned_data{nullptr};
        T                   *m_data{nullptr};

        Shape<NDim>  m_shape{};
        Stride<NDim> m_strides{};
        size_type    m_size{0};

        template <std::integral I>
        inline constexpr size_type stride(I index) const {
            if constexpr (std::signed_integral<I>)
                assert(0 <= index && "Negative Index");

            assert(index < NDim && "Index out of bounds");
            return m_strides[static_cast<stride_size_type>(index)];
        }

        // Protected Owning Constructor
        explicit NDArray(std::shared_ptr<T[]> owned_data, Shape<NDim> shape)
            : NDArray(owned_data.get(), shape) {
            m_owned_data = owned_data;
        }

    public:
        // Since we may own resources, we need to follow rule of 5

        // Destructor to ensure proper cleanup
        // Non-virtual as class is marked final
        // Nothing extra needed here since we are using std::shared_ptr
        ~NDArray() = default;

        // Copy Constructor
        // Nothing extra needed here since we only do shallow copy
        NDArray(const NDArray& other) noexcept = default;

        // Copy Assignment
        // Nothing extra needed here since we only do shallow copy
        NDArray& operator=(const NDArray& other) noexcept = default;

        // Move Constructor
        // Nothing extra needed here since we only do shallow move
        NDArray(NDArray&& other) noexcept = default;

        // Move Assignment
        // Nothing extra needed here since we only do shallow move
        NDArray& operator=(NDArray&& other) noexcept = default;

        // Public Non-Owning Constructor
        explicit NDArray(T *data, Shape<NDim> shape)
            : m_data(data)
            , m_shape(shape)
            , m_size(1) {
            assert(data != nullptr && "Null pointer");
            for (size_type i = NDim; i > 0; --i) {
                m_strides[i - 1]  = m_size;
                m_size           *= shape[i - 1];
            }
        }

        // Public Owning Constructor only for 1D Array
        explicit NDArray(std::initializer_list<T> init)
        requires(NDim == 1)
            : NDArray(std::make_shared<T[]>(init.size()), {init.size()}) {
            std::copy(init.begin(), init.end(), m_data);
        }

        // Factory Functions to create owning NDArray
        static NDArray<T, NDim> Empty(Shape<NDim> shape) {
            auto owned_data = std::make_shared<T[]>(std::reduce(
                shape.begin(), shape.end(), static_cast<size_type>(1),
                std::multiplies<size_type>{}));

            return NDArray<T, NDim>(owned_data, shape);
        }

        static NDArray<T, NDim> Full(Shape<NDim> shape, T value) {
            auto arr = Empty(shape);
            std::fill(arr.m_data, arr.m_data + arr.m_size, value);
            return arr;
        }

        static NDArray<T, NDim> Zeros(Shape<NDim> shape) {
            return Full(shape, 0);
        }

        static NDArray<T, NDim> Ones(Shape<NDim> shape) {
            return Full(shape, 1);
        }

        static NDArray<T, NDim>
            FromData(std::initializer_list<std::initializer_list<T>> data)
        requires(NDim == 2)
        {
            if (std::empty(data)) { return Empty({0, 0}); }

            const auto rows = data.size();
            const auto cols = data.begin()->size();

            assert(std::all_of(data.begin(), data.end(),
                               [cols](const std::initializer_list<T>& it) {
                                   return it.size() == cols;
                               }));

            auto      arr = Empty({rows, cols});
            size_type i{0};
            for (const auto& row : data) {
                for (const T val : row) {
                    arr[i] = val;
                    ++i;
                }
            }

            return arr;
        }

        // Queries
        inline constexpr size_type ndim() const { return NDim; }

        inline constexpr size_type size() const { return m_size; }

        inline constexpr Shape<NDim> shape() const { return m_shape; }

        // Access
        inline T *data() { return m_data; }

        inline const T *data() const { return m_data; }

        template <AllIntegral... Idx>
        requires(sizeof...(Idx) == NDim)
        inline constexpr bool ValidIndex(Idx... idx) const {
            using SizeType = std::conditional_t<AllUnsigned<Idx...>, size_type,
                                                std::ptrdiff_t>;

            const std::array<SizeType, NDim> idxs{
                static_cast<SizeType>(idx)...};

            for (size_type i = 0; i < NDim; ++i) {
                if constexpr (std::signed_integral<SizeType>) {
                    if (idxs[i] < 0) return false;
                }

                if (static_cast<size_type>(idxs[i]) >= m_shape[i]) return false;
            }

            return true;
        }

        template <typename... Idx>
        inline constexpr size_type Ravel(Idx... idx) const {
            assert(ValidIndex(idx...) && "Invalid index");

            const std::array<size_type, NDim> idxs{
                static_cast<size_type>(idx)...};

            size_type offset{0};
            for (size_type i = 0; i < NDim; ++i) {
                offset += idxs[i] * stride(i);
            }

            return offset;
        }

        inline T& operator[](size_type idx)
        requires(!std::is_const_v<T>)
        {
            assert(idx < m_size && "Index out of bounds");
            return m_data[idx];
        }

        inline const T& operator[](size_type idx) const {
            assert(idx < m_size && "Index out of bounds");
            return m_data[idx];
        }

        template <typename... Idx>
        inline T& operator()(Idx... idx)
        requires(!std::is_const_v<T>)
        {
            return m_data[Ravel(idx...)];
        }

        template <typename... Idx>
        inline const T& operator()(Idx... idx) const {
            return m_data[Ravel(idx...)];
        }

        // Copying
        NDArray<T, NDim> Copy() const {
            auto arr = Empty(m_shape);
            std::copy(m_data, m_data + m_size, arr.m_data);
            return arr;
        }

        static NDArray<T, NDim> Copy(const NDArray<T, NDim>& other) {
            return other.Copy();
        }

        // Filling
        void fill(T value) { std::fill(m_data, m_data + m_size, value); }

        void fill(const NDArray<T, NDim>& other) {
            assert(other.shape() == shape() && "Shape Mismatch");
            std::copy(other.m_data, other.m_data + m_size, m_data);
        }
    };

    template <typename T, typename U, size_type NDim>
    auto operator+(const NDArray<T, NDim>& a, const NDArray<U, NDim>& b) {
        using ResultType = decltype(std::declval<T>() + std::declval<U>());

        assert(a.shape() == b.shape() && "Shape Mismatch");

        auto result = NDArray<ResultType, NDim>::Empty(a.shape());

        for (size_type i{0}; i < a.size(); ++i) { result[i] = a[i] + b[i]; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator-(const NDArray<T, NDim>& a, const NDArray<U, NDim>& b) {
        using ResultType = decltype(std::declval<T>() - std::declval<U>());

        assert(a.shape() == b.shape() && "Shape Mismatch");

        auto result = NDArray<ResultType, NDim>::Empty(a.shape());

        for (size_type i{0}; i < a.size(); ++i) { result[i] = a[i] - b[i]; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator*(const NDArray<T, NDim>& a, const NDArray<U, NDim>& b) {
        using ResultType = decltype(std::declval<T>() * std::declval<U>());

        assert(a.shape() == b.shape() && "Shape Mismatch");

        auto result = NDArray<ResultType, NDim>::Empty(a.shape());

        for (size_type i{0}; i < a.size(); ++i) { result[i] = a[i] * b[i]; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator/(const NDArray<T, NDim>& a, const NDArray<U, NDim>& b) {
        using ResultType = decltype(std::declval<T>() / std::declval<U>());

        assert(a.shape() == b.shape() && "Shape Mismatch");

        auto result = NDArray<ResultType, NDim>::Empty(a.shape());

        for (size_type i{0}; i < a.size(); ++i) { result[i] = a[i] / b[i]; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator+(const NDArray<T, NDim>& a, const U& b) {
        using ResultType = decltype(std::declval<T>() + std::declval<U>());

        auto result = NDArray<ResultType, NDim>::Empty(a.shape());

        for (size_type i{0}; i < a.size(); ++i) { result[i] = a[i] + b; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator-(const NDArray<T, NDim>& a, const U& b) {
        using ResultType = decltype(std::declval<T>() - std::declval<U>());

        auto result = NDArray<ResultType, NDim>::Empty(a.shape());

        for (size_type i{0}; i < a.size(); ++i) { result[i] = a[i] - b; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator*(const NDArray<T, NDim>& a, const U& b) {
        using ResultType = decltype(std::declval<T>() * std::declval<U>());

        auto result = NDArray<ResultType, NDim>::Empty(a.shape());

        for (size_type i{0}; i < a.size(); ++i) { result[i] = a[i] * b; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator/(const NDArray<T, NDim>& a, const U& b) {
        using ResultType = decltype(std::declval<T>() / std::declval<U>());

        auto result = NDArray<ResultType, NDim>::Empty(a.shape());

        for (size_type i{0}; i < a.size(); ++i) { result[i] = a[i] / b; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator+(const T& a, const NDArray<U, NDim>& b) {
        using ResultType = decltype(std::declval<T>() + std::declval<U>());

        auto result = NDArray<ResultType, NDim>::Empty(b.shape());
        for (size_type i{0}; i < b.size(); ++i) { result[i] = a + b[i]; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator-(const T& a, const NDArray<U, NDim>& b) {
        using ResultType = decltype(std::declval<T>() - std::declval<U>());

        auto result = NDArray<ResultType, NDim>::Empty(b.shape());

        for (size_type i{0}; i < b.size(); ++i) { result[i] = a - b[i]; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator*(const T& a, const NDArray<U, NDim>& b) {
        using ResultType = decltype(std::declval<T>() * std::declval<U>());

        auto result = NDArray<ResultType, NDim>::Empty(b.shape());
        for (size_type i{0}; i < b.size(); ++i) { result[i] = a * b[i]; }

        return result;
    }

    template <typename T, typename U, size_type NDim>
    auto operator/(const T& a, const NDArray<U, NDim>& b) {
        using ResultType = decltype(std::declval<T>() / std::declval<U>());

        auto result = NDArray<ResultType, NDim>::Empty(b.shape());
        for (size_type i{0}; i < b.size(); ++i) { result[i] = a / b[i]; }

        return result;
    }

    /**************************************************************************/

    // Structural Concepts
    template <typename A>
    concept NDArrayLike = requires(A a) {
        typename A::value_type;
        { a.ndim() } -> std::convertible_to<size_type>;
        { a.size() } -> std::convertible_to<size_type>;
        { a.shape() };
        { a[0] } -> std::convertible_to<typename A::value_type>;
    };

    template <typename A>
    concept VectorLike
        = NDArrayLike<A> && requires(A a) { requires a.ndim() == 1; };

    template <typename A>
    concept MatrixLike
        = NDArrayLike<A> && requires(A a) { requires a.ndim() == 2; };

    template <VectorLike A, VectorLike B>
    auto dot(const A& a, const B& b) {
        using T          = typename A::value_type;
        using U          = typename B::value_type;
        using ResultType = decltype(std::declval<T>() * std::declval<U>());

        assert(a.shape()[0] == b.shape()[0] && "Shape Mismatch");

        ResultType result{0};
        for (size_type i{0}; i < a.shape()[0]; ++i) { result += a[i] * b[i]; }

        return result;
    }

    template <VectorLike A>
    auto norm(const A& a) {
        return std::sqrt(dot(a, a));
    }

    /**************************************************************************/

    void test();

} // namespace ND

#endif /* INCLUDE_CPP_EIGEN_OPENCV_SHARED_NDARRAY_HPP */
