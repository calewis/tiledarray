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
 *  noop.h
 *  June 27, 2013
 *
 */

#ifndef TILEDARRAY_TILE_OP_NOOP_H__INCLUDED
#define TILEDARRAY_TILE_OP_NOOP_H__INCLUDED

#include <TiledArray/tile_op/unary_interface.h>

namespace TiledArray {
  namespace math {

    /// Tile no operation (noop)

    /// This no operation will return the original or apply a permutation to the
    /// result tensor. If no permutation is given or the permutation is null,
    /// then the result is not permuted.
    /// \tparam Result The result type
    /// \tparam Arg The argument type
    /// \tparam Consumable Flag that is \c true when Arg is consumable
    template <typename Result, typename Arg, bool Consumable>
    class Noop : public UnaryInterface<Noop<Result, Arg, Consumable> >  {
    public:
      typedef Noop<Result, Arg, Consumable> Noop_; ///< This object type
      typedef UnaryInterface<Noop_> UnaryInterface_;
      typedef typename UnaryInterface_::argument_type argument_type; ///< The argument type
      typedef typename UnaryInterface_::result_type result_type; ///< The result tile type

      /// Default constructor

      /// Construct a no operation that does not permute the result tile
      Noop() : UnaryInterface_() { }

      /// Permute constructor

      /// Construct a no operation that permutes the result tensor
      /// \param perm The permutation to apply to the result tile
      Noop(const Permutation& perm) : UnaryInterface_(perm) { }

      /// Copy constructor

      /// \param other The no operation object to be copied
      Noop(const Noop_& other) : UnaryInterface_(other) { }

      /// Copy assignment

      /// \param other The no operation object to be copied
      /// \return A reference to this object
      Noop_& operator=(const Noop_& other) {
        UnaryInterface_::operator =(other);
        return *this;
      }

      // Import interface from base class
      using UnaryInterface_::operator();

      // Permuting tile evaluation function
      // These operations cannot consume the argument tile since this operation
      // requires temporary storage space.

      result_type permute_op(const Arg& arg) const {
        using TiledArray::permute;
        return permute(arg, UnaryInterface_::permutation());
      }

      // Non-permuting tile evaluation functions
      // The compiler will select the correct functions based on the consumability
      // of the arguments.

      template <bool C>
      static typename std::enable_if<!C, result_type>::type
      no_permute_op(const Arg& arg) {
        using TiledArray::clone;
        return clone(arg);
      }

      template <bool C>
      static typename std::enable_if<C, result_type>::type
      no_permute_op(Arg& arg) { return arg; }

    }; // class Noop

  } // namespace math
} // namespace TiledArray

#endif // TILEDARRAY_TILE_OP_NOOP_H__INCLUDED
