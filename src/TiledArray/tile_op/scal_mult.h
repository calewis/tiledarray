/*
 *  This file is a part of TiledArray.
 *  Copyright (C) 2013  Virginia Tech
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
 *  scal_mult.h
 *  May 8, 2013
 *
 */

#ifndef TILEDARRAY_TILE_OP_SCAL_MULT_H__INCLUDED
#define TILEDARRAY_TILE_OP_SCAL_MULT_H__INCLUDED

#include <TiledArray/tile_op/binary_interface.h>

namespace TiledArray {
  namespace math {

    /// Tile multiplication and scale operation

    /// This multiplication operation will multiply the content two tiles, then
    /// scale and apply a permutation to the result tensor. If no permutation is
    /// given or the permutation is null, then the result is not permuted.
    /// \tparam Result The result type
    /// \tparam Left The left-hand argument type
    /// \tparam Right The right-hand argument type
    /// \tparam LeftConsumable A flag that is \c true when the left-hand
    /// argument is consumable.
    /// \tparam RightConsumable A flag that is \c true when the right-hand
    /// argument is consumable.
    template <typename Result, typename Left, typename Right, bool LeftConsumable,
        bool RightConsumable>
    class ScalMult : public BinaryInterface<ScalMult<Result, Left, Right,
        LeftConsumable, RightConsumable> >
    {
    public:
      typedef ScalMult<Result, Left, Right, LeftConsumable, RightConsumable> ScalMult_; ///< This object type
      typedef BinaryInterface<ScalMult_> BinaryInterface_; ///< Interface base class type
      typedef typename BinaryInterface_::first_argument_type first_argument_type; ///< The left-hand argument type
      typedef typename BinaryInterface_::second_argument_type second_argument_type; ///< The right-hand argument type
      typedef typename BinaryInterface_::result_type result_type; ///< The result tile type
      typedef typename TiledArray::detail::scalar_type<result_type>::type scalar_type; ///< Scalar type

    private:

      scalar_type factor_; ///< The scaling factor

    public:
      /// Default constructor

      /// Construct a multiplication operation that does not permute the result tile
      /// and has a scaling factor of 1.
      ScalMult() : BinaryInterface_(), factor_(1) { }

      /// Permute constructor

      /// Construct a multiplication operation that scales the result tensor
      /// \param factor The scaling factor for the operation [default = 1]
      explicit ScalMult(const scalar_type factor) :
        BinaryInterface_(), factor_(factor)
      { }

      /// Permute constructor

      /// Construct a multiplication operation that permutes and scales the result tensor
      /// \param perm The permutation to apply to the result tile
      /// \param factor The scaling factor for the operation [default = 1]
      explicit ScalMult(const Permutation& perm, const scalar_type factor = scalar_type(1)) :
        BinaryInterface_(perm), factor_(factor)
      { }

      /// Copy constructor

      /// \param other The multiplication operation object to be copied
      ScalMult(const ScalMult_& other) : BinaryInterface_(other), factor_(other.factor_) { }

      /// Copy assignment

      /// \param other The multiplication operation object to be copied
      /// \return A reference to this object
      ScalMult_& operator=(const ScalMult_& other) {
        BinaryInterface_::operator =(other);
        factor_ = other.factor_;
        return *this;
      }

      // Import interface from base class
      using BinaryInterface_::operator();

      // Permuting tile evaluation function
      // These operations cannot consume the argument tile since this operation
      // requires temporary storage space.

      result_type permute_op(first_argument_type first, second_argument_type second) const {
        using TiledArray::mult;
        return mult(first, second, factor_, BinaryInterface_::permutation());
      }

      result_type permute_op(ZeroTensor, const Right&) const {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      result_type permute_op(const Left&, ZeroTensor) const {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      // Non-permuting tile evaluation functions
      // The compiler will select the correct functions based on the consumability
      // of the arguments.

      template <bool LC, bool RC>
      typename std::enable_if<!(LC || RC), result_type>::type
      no_permute_op(first_argument_type first, second_argument_type second) const {
        using TiledArray::mult;
        return mult(first, second, factor_);
      }

      template <bool LC, bool RC>
      typename std::enable_if<LC, result_type>::type
      no_permute_op(Left& first, second_argument_type second) const {
        using TiledArray::mult_to;
        return mult_to(first, second, factor_);
      }

      template <bool LC, bool RC>
      typename std::enable_if<!LC && RC, result_type>::type
      no_permute_op(first_argument_type first, Right& second) const {
        using TiledArray::mult_to;
        return mult_to(second, first, factor_);
      }

      template <bool LC, bool RC>
      static typename std::enable_if<!RC, result_type>::type
      no_permute_op(ZeroTensor, const Right&) {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      template <bool LC, bool RC>
      static typename std::enable_if<RC, result_type>::type
      no_permute_op(ZeroTensor, Right&) {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      template <bool LC, bool RC>
      static typename std::enable_if<!LC, result_type>::type
      no_permute_op(const Left&, ZeroTensor) {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

      template <bool LC, bool RC>
      static typename std::enable_if<LC, result_type>::type
      no_permute_op(Left&, ZeroTensor) {
        TA_ASSERT(false); // Invalid arguments for this operation
        return result_type();
      }

    }; // class ScalMult

  }  // namespace math
} // namespace TiledArray

#endif // TILEDARRAY_TILE_OP_SCAL_MULT_H__INCLUDED
