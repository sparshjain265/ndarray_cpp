/**
 * MIT License
 *
 * Copyright (c) 2025 Sparsh Jain
 *
 */

#ifndef INCLUDE_NDARRAY_CPP_SHARED_GEOMETRY_HPP
#define INCLUDE_NDARRAY_CPP_SHARED_GEOMETRY_HPP

#include <cmath>
#include <ndarray_cpp/shared/debug.hpp>
#include <ndarray_cpp/shared/ndarray.hpp>
#include <numbers>
#include <type_traits>
#include <vector>

#ifdef DEBUG
    #include <iostream>
#endif // DEBUG

namespace Geometry {
    using namespace ND;

    template <typename T>
    concept Arithmetic = std::is_arithmetic_v<T>;

    template <typename T>
    concept Integral = std::is_integral_v<T>;

    template <typename T>
    concept FloatingPoint = std::is_floating_point_v<T>;

    static inline constexpr double pi = std::numbers::pi;

    enum Order { Ascending, Descending };

    template <Arithmetic T, Arithmetic U = double>
    inline constexpr U cross(const NDArray<T, 1>& a, const NDArray<T, 1>& b) {

        assert(a.size() == static_cast<size_type>(2)
               && "cross product defined for 2D vectors only");
        assert(b.size() == static_cast<size_type>(2)
               && "cross product defined for 2D vectors only");

        DEBUG_ONLY static bool nonFloatWarningShown = false;
        if constexpr (!std::is_floating_point_v<T>) {
#ifdef DEBUG
            if (!nonFloatWarningShown) {

                std::cerr
                    << "Warning: cross product called with non floating-point "
                       "types. "
                    << "Results may be inaccurate due to ."
                    << std::endl;
                nonFloatWarningShown = true;
            }
#endif // DEBUG
        }

        auto ax = static_cast<U>(a[0]);
        auto ay = static_cast<U>(a[1]);
        auto bx = static_cast<U>(b[0]);
        auto by = static_cast<U>(b[1]);
        return ax * by - ay * bx;
    }

    // Argsort the first count points, all if count < 0
    template <Arithmetic T>
    std::vector<size_type> argSortPoints(const NDArray<T, 2>& points,
                                         const Order          order = Ascending,
                                         const int            count = -1) {
        const int N = (count < 0) ? static_cast<int>(points.shape()[0]) : count;
        assert(N <= static_cast<int>(points.shape()[0]));

        auto indices = std::vector<size_type>(static_cast<std::size_t>(N));
        std::iota(indices.begin(), indices.end(), 0);

        // Define comparison function based on order and sort indices
        // Sort Indices based on the order
        switch (order) {
        case Ascending:
            std::sort(indices.begin(), indices.end(),
                      [&points](size_type i, size_type j) {
                          return points(i, 0) < points(j, 0)
                              || (points(i, 0) <= points(j, 0)
                                  && points(i, 1) < points(j, 1));
                      });
            break;
        case Descending:
            std::sort(indices.begin(), indices.end(),
                      [&points](size_type i, size_type j) {
                          return points(i, 0) > points(j, 0)
                              || (points(i, 0) >= points(j, 0)
                                  && points(i, 1) > points(j, 1));
                      });
            break;

        default: break;
        }

        return indices;
    }

    // Function to compute convex hull of a set of 2D points
    // Returns the set of 2D points that form the convex hull
    template <Arithmetic T>
    NDArray<T, 2> computeConvexHull(const NDArray<T, 2>& points,
                                    const int            count = -1) {
        const int N = (count < 0) ? static_cast<int>(points.shape()[0]) : count;
        assert(N <= static_cast<int>(points.shape()[0]));

        if (N < 3) {
            auto trivialHull
                = NDArray<T, 2>::Empty({static_cast<size_type>(N), 2});
            for (int i = 0; i < N; ++i) {
                trivialHull(i, 0) = points(i, 0);
                trivialHull(i, 1) = points(i, 1);
            }

            return trivialHull;
        }

        const auto sortedIdx = argSortPoints(points, Ascending, count);

        // Store the hull points in a vector
        std::vector<NDArray<const T, 1>> hull;
        for (const auto& idx : sortedIdx) {
            // Extract the point as a 1D NDArray of size 2
            const auto p = NDArray<const T, 1>(&points(idx, 0), {2});

            while ((hull.size() >= 2)
                   && (cross(hull[hull.size() - 1] - hull[hull.size() - 2],
                             p - hull[hull.size() - 2])
                       <= 0)) {
                hull.pop_back();
            }
            hull.push_back(p);
        }

        const auto lowerSize = hull.size();
        for (int i = N - 2; i >= 0; --i) {
            const auto idx = sortedIdx[static_cast<std::size_t>(i)];
            const auto p   = NDArray<const T, 1>(&points(idx, 0), {2});
            while ((hull.size() > lowerSize)
                   && (cross(hull[hull.size() - 1] - hull[hull.size() - 2],
                             p - hull[hull.size() - 2])
                       <= 0)) {
                hull.pop_back();
            }
            hull.push_back(p);
        }

        // Remove repeated point
        hull.pop_back();

        // Convert hull points to NDArray
        auto hullPoints = NDArray<T, 2>::Empty({hull.size(), 2});
        for (size_type i = 0; i < hull.size(); ++i) {
            hullPoints(i, 0) = hull[i][0];
            hullPoints(i, 1) = hull[i][1];
        }

        return hullPoints;
    }

    // Struct to store a rotated rectangle
    struct RotatedRectangle {
        NDArray<double, 1> center; // (x, y)
        NDArray<double, 1> size;   // (width, height)
        double             angle;  // radians, CCW from x-axis

        RotatedRectangle() noexcept
            : center(NDArray<double, 1>::Zeros({2}))
            , size(NDArray<double, 1>::Zeros({2}))
            , angle(0.0) {}

        inline constexpr double angleDegrees() const {
            return angle * (180.0 / pi);
        }
    };

    // Function to compute min area rectangle containing a set of points
    template <Arithmetic T>
    RotatedRectangle minAreaRectangle(const NDArray<T, 2>& points,
                                      const int            count = -1) {
        const auto N
            = (count < 0) ? static_cast<int>(points.shape()[0]) : count;
        assert(N <= static_cast<int>(points.shape()[0]));

        const auto hull = computeConvexHull(points, N);
        const auto n    = hull.shape()[0];
        if (n == 0) { return RotatedRectangle{}; }

        if (n == 1) {
            RotatedRectangle res{};
            res.center[0] = hull(0, 0);
            res.center[1] = hull(0, 1);
            return res;
        }

        auto minArea = std::numeric_limits<double>::infinity();

        RotatedRectangle bestRectangle{};

        for (size_type i = 0; i < n; ++i) {
            const auto p0   = NDArray<const T, 1>(&hull(i, 0), {2});
            const auto p1   = NDArray<const T, 1>(&hull((i + 1) % n, 0), {2});
            const auto edge = p1 - p0;

            const double edgeLength = ND::norm(edge);
            if (edgeLength <= 0.0) continue;

            const auto ux = edge / edgeLength;
            const auto uy = NDArray<double, 1>({-ux[1], ux[0]});

            double minX = std::numeric_limits<double>::infinity();
            double maxX = -minX;
            double minY = minX;
            double maxY = maxX;

            for (size_type j = 0; j < n; ++j) {
                const auto p     = NDArray<const T, 1>(&hull(j, 0), {2});
                double     projX = static_cast<double>(ND::dot(p, ux));
                double     projY = static_cast<double>(ND::dot(p, uy));
                minX             = std::min(minX, projX);
                maxX             = std::max(maxX, projX);
                minY             = std::min(minY, projY);
                maxY             = std::max(maxY, projY);
            }

            const double width  = maxX - minX;
            const double height = maxY - minY;
            const double area   = width * height;

            if (area < minArea) {
                minArea                 = area;
                const auto centerLocalX = (minX + maxX) * 0.5;
                const auto centerLocalY = (minY + maxY) * 0.5;
                bestRectangle.center    = ux * centerLocalX + uy * centerLocalY;
                bestRectangle.size      = NDArray<double, 1>({width, height});
                bestRectangle.angle     = std::atan2(ux[1], ux[0]);
            }
        }

        return bestRectangle;
    }

    // We output is a reference parameter, so this operation can be done in
    // place if the caller provides the same argument for both parameters
    template <Integral T>
    void skeletonize(const NDArray<T, 2>& input, NDArray<T, 2>& output) {
        assert(input.shape() == output.shape() && "Shape Mismatch");

        output.fill(input);

        bool changed = true;
        auto keep    = NDArray<T, 2>::Empty(input.shape());

        // helpful constants
        constexpr T         zero{0};
        constexpr T         one{1};
        constexpr size_type sizeZero{0};
        constexpr size_type sizeOne{1};

        const size_type height = input.shape()[0];
        const size_type width  = input.shape()[1];

        /// The 8-neighborhood around pixel P1 at (i,j) is labeled:
        ///
        ///       P9  P2  P3
        ///       P8  P1  P4
        ///       P7  P6  P5
        ///
        /// Which maps to:
        ///       (i-1,j-1)  (i-1,j)  (i-1,j+1)
        ///       (i,  j-1)  (i,  j)  (i,  j+1)
        ///       (i+1,j-1)  (i+1,j)  (i+1,j+1)
        ///

        while (changed) {
            changed = false;
            keep.fill(one);

            // --- Sub-Iteration 1 ---
            for (size_type i = 0; i < height; ++i) {
                for (size_type j = 0; j < width; ++j) {
                    // Ignore background pixel
                    if (!output(i, j)) { continue; }

                    // get the neighbors in clockwise order
                    // P2, P3, P4, P5, P6, P7, P8, P9
                    const T p2 = (i > sizeZero) ? output(i - sizeOne, j) : zero;
                    const T p3 = (i > sizeZero && j < width - sizeOne)
                                   ? output(i - sizeOne, j + sizeOne)
                                   : zero;
                    const T p4
                        = (j < width - sizeOne) ? output(i, j + sizeOne) : zero;
                    const T p5 = (i < height - sizeOne && j < width - sizeOne)
                                   ? output(i + sizeOne, j + sizeOne)
                                   : zero;
                    const T p6 = (i < height - sizeOne) ? output(i + sizeOne, j)
                                                        : zero;
                    const T p7 = (i < height - sizeOne && j > sizeZero)
                                   ? output(i + sizeOne, j - sizeOne)
                                   : zero;
                    const T p8 = (j > sizeZero) ? output(i, j - sizeOne) : zero;
                    const T p9 = (i > sizeZero && j > sizeZero)
                                   ? output(i - sizeOne, j - sizeOne)
                                   : zero;

                    // Count Non-Zero neighbors
                    const int nonZeroNeighbors = static_cast<int>(
                        p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9);

                    // Condition 1: 2 <= nonZeroNeighbors <= 6
                    if (nonZeroNeighbors < 2 || nonZeroNeighbors > 6) {
                        continue;
                    }

                    // Count 0-1 transitions in clockwise p2, p3, ..., p9, p2
                    const std::array<T, 9> p{
                        {p2, p3, p4, p5, p6, p7, p8, p9, p2}
                    };
                    int zeroOneTransitionCount{0};
                    for (size_type index = 0; index < 8; ++index) {
                        if (!p[index] && p[index + 1]) {
                            ++zeroOneTransitionCount;
                        }
                    }

                    // Condition 2: 0-1 transition count == 1
                    if (zeroOneTransitionCount != 1) { continue; }

                    // Condition 3a: p2 * p4 * p6 == 0
                    if (p2 && p4 && p6) { continue; }

                    // Condition 3b: p4 * p6 * p8 == 0
                    if (p4 && p6 && p8) { continue; }

                    // If no condition satisfied, we should remove i, j
                    keep(i, j) = zero;
                    changed    = true;
                }
            }

            output = output * keep;

            keep.fill(one);

            // --- Sub-Iteration 2 ---
            for (size_type i = 0; i < height; ++i) {
                for (size_type j = 0; j < width; ++j) {
                    // Ignore background pixel
                    if (!output(i, j)) { continue; }

                    // get the neighbors in clockwise order
                    // P2, P3, P4, P5, P6, P7, P8, P9
                    const T p2 = (i > sizeZero) ? output(i - sizeOne, j) : zero;
                    const T p3 = (i > sizeZero && j < width - sizeOne)
                                   ? output(i - sizeOne, j + sizeOne)
                                   : zero;
                    const T p4
                        = (j < width - sizeOne) ? output(i, j + sizeOne) : zero;
                    const T p5 = (i < height - sizeOne && j < width - sizeOne)
                                   ? output(i + sizeOne, j + sizeOne)
                                   : zero;
                    const T p6 = (i < height - sizeOne) ? output(i + sizeOne, j)
                                                        : zero;
                    const T p7 = (i < height - sizeOne && j > sizeZero)
                                   ? output(i + sizeOne, j - sizeOne)
                                   : zero;
                    const T p8 = (j > sizeZero) ? output(i, j - sizeOne) : zero;
                    const T p9 = (i > sizeZero && j > sizeZero)
                                   ? output(i - sizeOne, j - sizeOne)
                                   : zero;

                    // Count Non-Zero neighbors
                    const int nonZeroNeighbors = static_cast<int>(
                        p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9);

                    // Condition 1: 2 <= nonZeroNeighbors <= 6
                    if (nonZeroNeighbors < 2 || nonZeroNeighbors > 6) {
                        continue;
                    }

                    // Count 0-1 transitions in clockwise p2, p3, ..., p9, p2
                    const std::array<T, 9> p{
                        {p2, p3, p4, p5, p6, p7, p8, p9, p2}
                    };
                    int zeroOneTransitionCount{0};
                    for (size_type index = 0; index < 8; ++index) {
                        if (!p[index] && p[index + 1]) {
                            ++zeroOneTransitionCount;
                        }
                    }

                    // Condition 2: 0-1 transition count == 1
                    if (zeroOneTransitionCount != 1) { continue; }

                    // Condition 3a: p2 * p4 * p8 == 0
                    if (p2 && p4 && p8) { continue; }

                    // Condition 3b: p2 * p6 * p8 == 0
                    if (p2 && p6 && p8) { continue; }

                    // If no condition satisfied, we should remove i, j
                    keep(i, j) = zero;
                    changed    = true;
                }
            }

            output = output * keep;
        }
    }

    /**************************************************************************/

    void testConvexHullInvariants(const NDArray<double, 2>& points);
    void testMinAreaRectangleInvariants(const NDArray<double, 2>& points);

    void testConvexHull();
    void testMinAreaRectangle();

    void testSkeletonize();
} // namespace Geometry

#endif /* INCLUDE_CPP_EIGEN_OPENCV_SHARED_GEOMETRY_HPP */
