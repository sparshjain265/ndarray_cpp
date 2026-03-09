/**
 * MIT License
 *
 * Copyright (c) 2025 Sparsh Jain
 *
 */

#include <iostream>

#include <ndarray_cpp/shared/ndarray.hpp>

namespace ND {

    void test() {
        std::cout << "Running test for NDArray..." << std::endl;

        {
            // Test Shape
            std::cout << "Testing Shape..." << std::endl;
            const Shape<2> shape({3, 4});
            std::cout << "Size: " << shape.size() << std::endl;
            std::cout << "Shape[0]: " << shape[0] << std::endl;
            std::cout << "Shape[1]: " << shape[1] << std::endl;
        }

        {
            // Const Non-Owning NDArray
            const int data[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
            NDArray<const int, 2> array(data, {3, 4});

            // Uncommenting the following line should result in a compile-time
            // error array(0, 0) = 100;

            std::cout << "Array(0, 0): " << array(0, 0) << std::endl;
        }

        {
            // Non-Owning NDArray
            int             data[12] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};
            NDArray<int, 2> array(data, {3, 4});
            array(0, 0) = 100;
            std::cout << "Array(0, 0): " << array(0, 0) << std::endl;
        }

        {
            // Owning NDArray
            auto array  = NDArray<int, 2>::Zeros({3, 4});
            array(0, 0) = 100;
            std::cout << "Array(0, 0): " << array(0, 0) << std::endl;
        }

        {
            // Test Ravel
            auto a = ND::NDArray<int, 2>::FromData({
                {1, 2,  3,  4 },
                {5, 6,  7,  8 },
                {9, 10, 11, 12}
            });
            assert(a(0, 0) == 1);
            assert(a(0, 3) == 4);
            assert(a(1, 0) == 5);
            assert(a(2, 3) == 12);
            assert(a.Ravel(0, 0) == 0);
            assert(a.Ravel(0, 3) == 3);
            assert(a.Ravel(1, 0) == 4);
            assert(a.Ravel(2, 3) == 11);
        }

        {
            // Test Fill
            auto x = ND::NDArray<int, 2>::FromData({
                {1, 2},
                {3, 4}
            });
            auto y = ND::NDArray<int, 2>::Zeros({2, 2});
            y.fill(x);
            assert(y(0, 0) == 1);
            assert(y(0, 1) == 2);
            assert(y(1, 0) == 3);
            assert(y(1, 1) == 4);
        }
    }

} // namespace ND
