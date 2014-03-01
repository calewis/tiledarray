/*
 *  This file is a part of TiledArray.
 *  Copyright (C) 2014  Virginia Tech
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *  Justus Calvin
 *  Department of Chemistry, Virginia Tech
 *
 *  outer.h
 *  Feb 16, 2014
 *
 */

#ifndef TILEDARRAY_MATH_OUTER_H__INCLUDED
#define TILEDARRAY_MATH_OUTER_H__INCLUDED

#include <TiledArray/madness.h>
#include <TiledArray/math/vector_op.h>

namespace TiledArray {
  namespace math {

    template <typename T, typename Op, typename Result>
    class BinderFirstHelper {
      const T& restrict t_;
      const Op& op_;

    public:
      typedef Result result_type;

      BinderFirstHelper(const T& restrict t, const Op& op) : t_(t), op_(op) { }

      template <typename Arg>
      TILEDARRAY_FORCE_INLINE result_type operator()(const Arg& restrict arg) const {
        return op_(t_, arg);
      }
    };


    template <typename T, typename Op>
    class BinderFirstHelper<T, Op, void> {
      const T& restrict t_;
      const Op& op_;

    public:
      typedef void result_type;

      BinderFirstHelper(const T& restrict t, const Op& op) : t_(t), op_(op) { }

      template <typename Result, typename Arg>
      TILEDARRAY_FORCE_INLINE void
      operator()(Result& restrict result, const Arg& restrict arg) const {
        return op_(result, t_, arg);
      }
    };

    template <typename T, typename Op>
    class BinderFirst : public BinderFirstHelper<T, Op, typename madness::detail::result_of<Op>::type> {
      typedef BinderFirstHelper<T, Op, typename madness::detail::result_of<Op>::type> BinderFirstHelper_;
    public:
      typedef typename BinderFirstHelper_::result_type result_type;

      BinderFirst(const T& restrict t, const Op& op) :
        BinderFirstHelper_(t, op)
      { }

    };

    template <typename T, typename Op>
    inline BinderFirst<T, Op> bind_first(const T& restrict t, const Op& op) {
      return BinderFirst<T, Op>(t, op);
    }

    template <typename T, typename Op, typename Result>
    class BinderSecondHelper {
      const T& restrict t_;
      const Op& op_;

    public:
      typedef Result result_type;

      BinderSecondHelper(const T& restrict t, const Op& op) : t_(t), op_(op) { }

      template <typename Arg>
      TILEDARRAY_FORCE_INLINE result_type operator()(const Arg& restrict arg) const {
        return op_(t_, arg);
      }
    };


    template <typename T, typename Op>
    class BinderSecondHelper<T, Op, void> {
      const T& restrict t_;
      const Op& op_;

    public:
      typedef void result_type;

      BinderSecondHelper(const T& restrict t, const Op& op) : t_(t), op_(op) { }

      template <typename Result, typename Arg>
      TILEDARRAY_FORCE_INLINE void operator()(Result& restrict result, const Arg& restrict arg) const {
        return op_(result, arg, t_);
      }
    };

    template <typename T, typename Op>
    class BinderSecond : public BinderSecondHelper<T, Op, typename madness::detail::result_of<Op>::type> {
      typedef BinderSecondHelper<T, Op, typename madness::detail::result_of<Op>::type> BinderSecondHelper_;
    public:
      typedef typename BinderSecondHelper_::result_type result_type;

      BinderSecond(const T& restrict t, const Op& op) :
        BinderSecondHelper_(t, op)
      { }

    };

    template <typename T, typename Op>
    inline BinderSecond<T, Op> bind_second(const T& restrict t, const Op& op) {
      return BinderSecond<T, Op>(t, op);
    }

    /// Outer algorithm automatic loop unwinding

    /// \tparam N The number of steps to unwind
    template <std::size_t N>
    class OuterVectorOpUnwind;

    template <>
    class OuterVectorOpUnwind<0> {
    public:

      static const std::size_t offset = TILEDARRAY_LOOP_UNWIND - 1;

      template <typename X, typename Y, typename A, typename Op>
      static TILEDARRAY_FORCE_INLINE void
      outer(const X* restrict const left, const Y* restrict const right,
          A* restrict const result, const std::size_t stride, const Op& op)
      {
        A result_block[TILEDARRAY_LOOP_UNWIND];
        VecOpUnwindN::copy(result, result_block);

        VecOpUnwindN::binary(right, result_block, bind_first(left[offset], op));

        VecOpUnwindN::copy(result_block, result);
      }

      template <typename X, typename Y, typename A, typename B, typename Op>
      static TILEDARRAY_FORCE_INLINE void
      fill(const X* restrict const x_block, const Y* restrict const y_block,
          const A* restrict const a, B* restrict const b,
          const std::size_t stride, const Op& op)
      {
        A a_block[TILEDARRAY_LOOP_UNWIND];
        VecOpUnwindN::copy(a, a_block);

        VecOpUnwindN::binary(y_block, a_block, bind_first(x_block[offset], op));

        VecOpUnwindN::copy(a_block, b);
      }

      template <typename X, typename Y, typename A, typename Op>
      static TILEDARRAY_FORCE_INLINE void
      fill(const X* restrict const left, const Y* restrict const right,
          A* restrict const result, const std::size_t stride, const Op& op)
      {
        A result_block[TILEDARRAY_LOOP_UNWIND];
        VecOpUnwindN::unary(right, result_block, bind_first(left[offset], op));

        VecOpUnwindN::copy(result_block, result);
      }
    }; // class OuterVectorOpUnwind<0>


    template <std::size_t N>
    class OuterVectorOpUnwind : public OuterVectorOpUnwind<N - 1> {
    public:

      typedef OuterVectorOpUnwind<N - 1> OuterVectorOpUnwindN1;

      static const std::size_t offset = TILEDARRAY_LOOP_UNWIND - N - 1;

      template <typename X, typename Y, typename A, typename Op>
      static TILEDARRAY_FORCE_INLINE void
      outer(const X* restrict const x_block, const Y* restrict const y_block,
          A* restrict const a, const std::size_t stride, const Op& op)
      {
        {
          A a_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::copy(a, a_block);

          VecOpUnwindN::binary(y_block, a_block, bind_first(x_block[offset], op));

          VecOpUnwindN::copy(a_block, a);
        }

        OuterVectorOpUnwindN1::outer(x_block, y_block, a + stride, stride, op);
      }


      template <typename X, typename Y, typename A, typename B, typename Op>
      static TILEDARRAY_FORCE_INLINE void
      fill(const X* restrict const x, const Y* restrict const y,
          const A* restrict const a, B* restrict const b,
          const std::size_t stride, const Op& op)
      {
        {
          A a_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::copy(a, a_block);

          VecOpUnwindN::binary(y, a_block, bind_first(x[offset], op));

          VecOpUnwindN::copy(a_block, b);
        }

        OuterVectorOpUnwindN1::fill(x, y, a + stride, b + stride, stride, op);
      }

      template <typename X, typename Y, typename A, typename Op>
      static TILEDARRAY_FORCE_INLINE void
      fill(const X* restrict const x_block, const Y* restrict const y_block,
          A* restrict const a, const std::size_t stride, const Op& op)
      {
        {
          A a_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::unary(y_block, a_block, bind_first(x_block[offset], op));

          VecOpUnwindN::copy(a_block, a);
        }

        OuterVectorOpUnwindN1::fill(x_block, y_block, a + stride, stride, op);
      }
    }; // class OuterVectorOpUnwind

    // Convenience typedef
    typedef OuterVectorOpUnwind<TILEDARRAY_LOOP_UNWIND - 1> OuterVectorOpUnwindN;


    /// Compute and store outer of \c x and \c y in \c a

    /// <tt>a[i][j] = op(x[i], y[j])</tt>.
    /// \tparam X The left-hand vector element type
    /// \tparam Y The right-hand vector element type
    /// \tparam A The a matrix element type
    /// \param[in] m The size of the left-hand vector
    /// \param[in] n The size of the right-hand vector
    /// \param[in] x The left-hand vector
    /// \param[in] y The right-hand vector
    /// \param[out] a The result matrix of size \c m*n
    /// \param[in] op The operation that will compute the outer product elements
    template <typename X, typename Y, typename A, typename Op>
    void outer_fill(const std::size_t m, const std::size_t n,
        const X* restrict const x, const Y* restrict const y,
        A* restrict const a, const Op& op)
    {
      std::size_t i = 0ul;

#if TILEDARRAY_LOOP_UNWIND > 1

      // Compute block iteration limit
      const std::size_t mx = m & index_mask::value; // = m - m % TILEDARRAY_LOOP_UNWIND
      const std::size_t nx = n & index_mask::value; // = n - n % TILEDARRAY_LOOP_UNWIND

      for(; i < mx; i += TILEDARRAY_LOOP_UNWIND) {

        // Load x block
        X x_block[TILEDARRAY_LOOP_UNWIND];
        VecOpUnwindN::copy(x + i, x_block);
        A* restrict const a_i = a + (i * n);

        std::size_t j = 0ul;
        for(; j < nx; j += TILEDARRAY_LOOP_UNWIND) {

          // Load y block
          Y y_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::copy(y + j, y_block);

          // Compute and store a block
          OuterVectorOpUnwindN::fill(x_block, y_block, a_i + j, n, op);

        }

        for(; j < n; ++j) {

          // Load y block
          const Y y_j = y[j];

          // Compute a block
          A a_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::unary(x_block, a_block, bind_second(y_j, op));

          // Store a block
          VecOpUnwindN::scatter(a_block, a_i + j, n);
        }
      }

#endif // TILEDARRAY_LOOP_UNWIND > 1

      for(; i < m; ++i) {

        const X x_i = x[i];
        unary_vector_op(n, y, a, bind_second(x_i, op));

      }
    }

    /// Compute the outer of \c x and \c y to modify \c a

    /// Compute <tt>op(a[i][j], x[i], y[j])</tt> for each \c i and \c j pair,
    /// where \c a[i][j] is modified by \c op.
    /// \tparam X The left hand vector element type
    /// \tparam Y The right-hand vector element type
    /// \tparam A The a matrix element type
    /// \tparam Op The operation that will compute outer product elements
    /// \param[in] m The size of the left-hand vector
    /// \param[in] n The size of the right-hand vector
    /// \param[in] alpha The scaling factor
    /// \param[in] x The left-hand vector
    /// \param[in] y The right-hand vector
    /// \param[in,out] a The result matrix of size \c m*n
    /// \param[in]
    template <typename X, typename Y, typename A, typename Op>
    void outer(const std::size_t m, const std::size_t n,
        const X* restrict const x, const Y* restrict const y,
        A* restrict const a, const Op& op)
    {
      std::size_t i = 0ul;

#if TILEDARRAY_LOOP_UNWIND > 1

      // Compute block iteration limit
      const std::size_t mx = m & index_mask::value; // = m - m % TILEDARRAY_LOOP_UNWIND
      const std::size_t nx = n & index_mask::value; // = n - n % TILEDARRAY_LOOP_UNWIND

      for(; i < mx; i += TILEDARRAY_LOOP_UNWIND) {

        // Load x block
        X x_block[TILEDARRAY_LOOP_UNWIND];
        VecOpUnwindN::copy(x + i, x_block);

        // Compute pointer offset
        A* restrict const a_i = a + (i * n);

        std::size_t j = 0ul;
        for(; j < nx; j += TILEDARRAY_LOOP_UNWIND) {

          // Load y block
          Y y_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::copy(y + j, y_block);

          // Load, compute, and store a block
          OuterVectorOpUnwindN::outer(x_block, y_block, a_i + j, n, op);

        }

        for(; j < n; ++j) {

          // Load a block
          A* restrict const a_ij = a_i + j;
          A a_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::gather(a_ij, a_block, n);

          // Load y block
          const Y y_j = y[j];

          // Compute a block
          VecOpUnwindN::binary(x_block, a_block, bind_second(y_j, op));

          // Store a block
          VecOpUnwindN::scatter(a_block, a_ij, n);

        }
      }

#endif // TILEDARRAY_LOOP_UNWIND > 1

      for(; i < m; ++i) {
        const X x_i = x[i];
        binary_vector_op(n, y, a, bind_second(x_i, op));
      }
    }


    /// Compute the outer of \c x, \c y, and \c a, and store the result in \c b

    /// Store a modified copy of \c a in \c b, where modified elements are
    /// generated using the following algorithm:
    /// \code
    /// A temp = a[i][j];
    /// op(temp, x[i], y[j]);
    /// b[i][j] = temp;
    /// \endcode
    /// for each unique pair of \c i and \c j.
    /// \tparam X The left hand vector element type
    /// \tparam Y The right-hand vector element type
    /// \tparam A The a matrix element type
    /// \tparam B The b matrix element type
    /// \tparam Op The operation that will compute outer product elements
    /// \param[in] m The size of the left-hand vector
    /// \param[in] n The size of the right-hand vector
    /// \param[in] alpha The scaling factor
    /// \param[in] x The left-hand vector
    /// \param[in] y The right-hand vector
    /// \param[in] a The input matrix of size \c m*n
    /// \param[out] b The output matrix of size \c m*n
    template <typename X, typename Y, typename A, typename B, typename Op>
    void outer_fill(const std::size_t m, const std::size_t n,
        const X* restrict const x, const Y* restrict const y,
        const A* restrict const a, B* restrict const b, const Op& op)
    {
      std::size_t i = 0ul;

#if TILEDARRAY_LOOP_UNWIND > 1

      // Compute block iteration limit
      const std::size_t mx = m & index_mask::value; // = m - m % TILEDARRAY_LOOP_UNWIND
      const std::size_t nx = n & index_mask::value; // = n - n % TILEDARRAY_LOOP_UNWIND

      for(; i < mx; i += TILEDARRAY_LOOP_UNWIND) {

        // Load x block
        X x_block[TILEDARRAY_LOOP_UNWIND];
        VecOpUnwindN::copy(x + i, x_block);

        // Compute a & b block pointers
        const std::size_t in = i * n;
        const A* restrict const a_i = a + (in);
        B* restrict const b_i = b + (in);

        std::size_t j = 0ul;
        for(; j < nx; j += TILEDARRAY_LOOP_UNWIND) {

          // Load y block
          Y y_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::copy(y + j, y_block);

          // Load, compute, and store a block
          OuterVectorOpUnwindN::fill(x_block, y_block, a_i + j, b_i + j, n, op);

        }

        for(; j < n; ++j) {

          // Load a block
          A a_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::gather(a_i + j, a_block, n);

          // Load y block
          const Y y_j = y[j];

          // Compute a block
          VecOpUnwindN::binary(x_block, a_block, bind_second(y_j, op));

          // Store a block
          VecOpUnwindN::scatter(a_block, b_i + j, n);
        }
      }

#endif // TILEDARRAY_LOOP_UNWIND > 1

      for(; i < m; ++i) {

        // Load x block
        const X x_i = x[i];

        std::size_t j = 0ul;

#if TILEDARRAY_LOOP_UNWIND > 1

        // Compute a & b block pointers
        const std::size_t in = i * n;
        const A* restrict const a_i = a + in;
        B* restrict const b_i = b + in;

        for(; j < nx; j += TILEDARRAY_LOOP_UNWIND) {

          // Load a block
          A a_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::copy(a_i + j, a_block);

          // Load y block
          Y y_block[TILEDARRAY_LOOP_UNWIND];
          VecOpUnwindN::copy(y + j, y_block);

          // Compute outer block
          VecOpUnwindN::binary(y_block, a_block, bind_second(x_i, op));

          // Store a block
          VecOpUnwindN::copy(a_block, b_i + j);

        }

#endif // TILEDARRAY_LOOP_UNWIND > 1

        for(; j < n; ++j) {

          A a_ij = a_i[j];
          const Y y_j = y[j];
          op(a_ij, x_i, y_j);
          b_i[j] = a_ij;
        }
      }
    }

  } // namespace math
} // namespace TiledArray

#endif // TILEDARRAY_MATH_OUTER_H__INCLUDED