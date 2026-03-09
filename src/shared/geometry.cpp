/**
 * MIT License
 *
 * Copyright (c) 2025 Sparsh Jain
 *
 */

#include <iostream>
#include <random>

#include <ndarray_cpp/shared/debug.hpp>
#include <ndarray_cpp/shared/geometry.hpp>
#include <ndarray_cpp/shared/ndarray.hpp>

namespace Geometry {
    void testConvexHullInvariants(const NDArray<double, 2>& points) {
        const auto N    = points.shape()[0];
        const auto hull = computeConvexHull(points);
        const auto n    = hull.shape()[0];

        if (n < 3) return; // Trivial hull, no need to test further

        constexpr double eps = 1e-6;
        constexpr auto   equal
            = [](double a, double b) { return std::abs(a - b) < eps; };

        // Hull Points are a subset of input points
        for (size_type i = 0; i < n; ++i) {
            DEBUG_ONLY bool found = false;
            for (size_type j = 0; j < N; ++j) {
                if ((equal(hull(i, 0), points(j, 0)))
                    && (equal(hull(i, 1), points(j, 1)))) {
                    found = true;
                    break;
                }
            }

            assert(found && "Hull point not found in input points");
        }

        // Hull points are convex in counter-clockwise order
        for (size_type i = 0; i < n; ++i) {
            const auto p0 = NDArray<const double, 1>(&hull(i, 0), {2});
            const auto p1
                = NDArray<const double, 1>(&hull((i + 1) % n, 0), {2});
            const auto p2
                = NDArray<const double, 1>(&hull((i + 2) % n, 0), {2});

            const auto v1 = p1 - p0;
            const auto v2 = p2 - p1;

            DEBUG_ONLY const auto crossProduct = cross(v1, v2);
            assert(crossProduct >= -eps
                   && "Hull points not in counter-clockwise order");
        }

        // All points lie inside or on the hull
        for (size_type i = 0; i < N; ++i) {
            const auto      p = NDArray<const double, 1>(&points(i, 0), {2});
            DEBUG_ONLY bool inside = true;
            for (size_type j = 0; j < n; ++j) {
                const auto p0 = NDArray<const double, 1>(&hull(j, 0), {2});
                const auto p1
                    = NDArray<const double, 1>(&hull((j + 1) % n, 0), {2});
                const auto edge         = p1 - p0;
                const auto toPoint      = p - p0;
                const auto crossProduct = cross(edge, toPoint);
                if (crossProduct < -eps) {
                    inside = false;
                    break;
                }
            }
            assert(inside && "Point not inside hull");
        }
    }

    void testMinAreaRectangleInvariants(const NDArray<double, 2>& points) {
        const auto rectangle = minAreaRectangle(points);
        const auto N         = points.shape()[0];

        const double cosA = std::cos(rectangle.angle);
        const double sinA = std::sin(rectangle.angle);

        const auto u = NDArray<double, 1>({cosA, sinA});
        const auto v = NDArray<double, 1>({-sinA, cosA});

        // Check that all points lie within the rectangle
        for (size_type i = 0; i < N; ++i) {
            const auto p = NDArray<const double, 1>(&points(i, 0), {2});

            // Translate point to rectangle center
            const auto translated = p - rectangle.center;

            // Rotate point by -angle
            DEBUG_ONLY const double xRotated = ND::dot(translated, u);
            DEBUG_ONLY const double yRotated = ND::dot(translated, v);

            // Check if point lies within rectangle bounds
            DEBUG_ONLY const double halfWidth  = rectangle.size[0] * 0.5;
            DEBUG_ONLY const double halfHeight = rectangle.size[1] * 0.5;

            DEBUG_ONLY constexpr double eps = 1e-6;
            assert((std::abs(xRotated) <= halfWidth + eps)
                   && (std::abs(yRotated) <= halfHeight + eps)
                   && "Point lies outside the minimum area rectangle");
        }
    }

    void testConvexHull() {
        std::cout << "Running tests for computeConvexHull..." << std::endl;

        // Generate random points and test computeConvexHull invariants
        std::mt19937 rng(42); // Fixed seed for reproducibility
        std::uniform_real_distribution<double> dist(-1000.0, 1000.0);

        for (int iter = 0; iter < 1000; ++iter) {
            const size_type numPoints = rng() % 1000 + 1;
            auto            points = NDArray<double, 2>::Empty({numPoints, 2});

            for (size_type i = 0; i < numPoints; ++i) {
                points(i, 0) = dist(rng);
                points(i, 1) = dist(rng);
            }

            testConvexHullInvariants(points);
        }
    }

    void testMinAreaRectangle() {
        std::cout << "Running tests for minAreaRectangle..." << std::endl;

        // Generate random points and test minAreaRectangle invariants
        std::mt19937 rng(123); // Fixed seed for reproducibility
        std::uniform_real_distribution<double> dist(-1000.0, 1000.0);

        for (int iter = 0; iter < 1000; ++iter) {
            const size_type numPoints = rng() % 1000 + 1;
            auto            points = NDArray<double, 2>::Empty({numPoints, 2});

            for (size_type i = 0; i < numPoints; ++i) {
                points(i, 0) = dist(rng);
                points(i, 1) = dist(rng);
            }

            testMinAreaRectangleInvariants(points);
        }
    }

    static void printPattern(const NDArray<int, 2>& pattern) {
        for (size_type i = 0; i < pattern.shape()[0]; ++i) {
            for (size_type j = 0; j < pattern.shape()[1]; ++j) {
                std::cout << (pattern(i, j) ? '#' : '.');
            }
            std::cout << '\n';
        }
        std::cout << std::endl;
    }

    void testSkeletonize() {
        std::cout << "Running tests for skeletonize..." << std::endl;

        // R.C.
        auto pattern = NDArray<int, 2>::FromData({
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
             0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0,
             1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0,
             1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0,
             1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0,
             1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0,
             1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0},
            {0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0,
             1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0},
            {0, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 0,
             0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        });

        std::cout << "Input:\n";
        printPattern(pattern);
        skeletonize(pattern, pattern);
        std::cout << "Output:\n";
        printPattern(pattern);

        std::cout << std::endl;
    }
} // namespace Geometry
