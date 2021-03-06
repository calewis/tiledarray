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
 *  binary_interface.h
 *  Oct 6, 2013
 *
 */

#ifndef TILEDARRAY_TILE_OP_BINARY_INTERFACE_H__INCLUDED
#define TILEDARRAY_TILE_OP_BINARY_INTERFACE_H__INCLUDED

#include <TiledArray/tile_op/tile_interface.h>
#include <TiledArray/tile_op/type_traits.h>
#include <TiledArray/permutation.h>
#include <TiledArray/zero_tensor.h>

namespace TiledArray {
  namespace math {

    template <typename Op>
    struct BinaryTileOpPolicy;

    /// Policy class for binary tile operations

    /// \tparam Result The result type
    /// \tparam Left The left-hand argument type
    /// \tparam Right The right-hand argument type
    /// \tparam LeftConsumable A flag that is \c true when the left-hand
    /// argument is consumable.
    /// \tparam RightConsumable A flag that is \c true when the right-hand
    /// argument is consumable.
    /// \tparam Op The binary tile operation template
    template <typename Result, typename Left, typename Right, bool LeftConsumable,
        bool RightConsumable, template <typename, typename, typename, bool, bool> class Op>
    struct BinaryTileOpPolicy<Op<Result, Left, Right, LeftConsumable, RightConsumable> > {
      typedef typename std::conditional<LeftConsumable, Left&, const Left&>::type
          first_argument_type; ///< The left-hand argument type
      typedef typename std::conditional<RightConsumable, Right&,
          const Right&>::type second_argument_type; ///< The right-hand argument type
      typedef Result result_type; ///< The result tile type

      typedef std::integral_constant<bool, LeftConsumable && std::is_same<Result,
          Left>::value> left_is_consumable; ///< Left is consumable type trait
      typedef std::integral_constant<bool, RightConsumable && std::is_same<Result,
          Right>::value> right_is_consumable; ///< Right is consumable type trait

    }; // struct BinaryTileOpPolicy


    /// Binary tile operation interface base

    /// This base class defines binary operations with zero and non-zero tiles,
    /// and maps arguments given to the appropriate evaluation kernel.
    /// \tparam Derived The derived operation class type
    template <typename Derived>
    class BinaryInterfaceBase {
    public:
      typedef typename BinaryTileOpPolicy<Derived>::first_argument_type
          first_argument_type; ///< The left-hand argument type
      typedef typename BinaryTileOpPolicy<Derived>::second_argument_type
          second_argument_type; ///< The right-hand argument type
      typedef typename BinaryTileOpPolicy<Derived>::result_type
          result_type; ///< The result tile type

      typedef typename BinaryTileOpPolicy<Derived>::left_is_consumable
          left_is_consumable; ///< Left is consumable type trait
      typedef typename BinaryTileOpPolicy<Derived>::right_is_consumable
          right_is_consumable; ///< Right is consumable type trait

    protected:

      /// Derived type accessor

      /// \return A const reference to the derived object
      const Derived& derived() const { return static_cast<const Derived&>(*this); }

    public:

      /// Evaluate two non-zero tiles and possibly permute

      /// Evaluate the result tile using the appropriate \c Derived class
      /// evaluation kernel.
      /// \param first The left-hand argument
      /// \param second The right-hand argument
      /// \return The result tile from the binary operation applied to the
      /// \c first and \c second .
      result_type operator()(first_argument_type first, second_argument_type second) const {
        if(derived().permutation())
          return derived().permute_op(first, second);

        return derived().template no_permute_op<left_is_consumable::value,
            right_is_consumable::value>(first, second);
      }

      /// Evaluate a zero tile to a non-zero tiles and possibly permute

      /// Evaluate the result tile using the appropriate \c Derived class
      /// evaluation kernel.
      /// \param first The left-hand argument
      /// \param second The right-hand argument
      /// \return The result tile from the binary operation applied to the
      /// \c first and \c second .
      result_type operator()(ZeroTensor first, second_argument_type second) const {
        if(derived().permutation())
          return derived().permute_op(first, second);

        return derived().template no_permute_op<left_is_consumable::value,
            right_is_consumable::value>(first, second);
      }

      /// Evaluate a non-zero tiles to a zero tile and possibly permute

      /// Evaluate the result tile using the appropriate \c Derived class
      /// evaluation kernel.
      /// \param first The left-hand argument
      /// \param second The right-hand argument
      /// \return The result tile from the binary operation applied to the
      /// \c first and \c second .
      result_type operator()(first_argument_type first, ZeroTensor second) const {
        if(derived().permutation())
          return derived().permute_op(first, second);

        return derived().template no_permute_op<left_is_consumable::value,
            right_is_consumable::value>(first, second);
      }

    }; // class BinaryInterfaceBase


  } // namespace math

  /// Binary tile operation interface

  /// In addition to the interface defined by \c BinaryInterfaceBase, this
  /// class defines binary operations with lazy tiles. It will evaluate
  /// arguments as necessary and pass them to the \c BinaryInterfaceBase
  /// interface functions.
  ///
  /// To use this interface class, a derived class needs to have the following
  /// form:
  /// \code
  /// template <typename Result, typename Left, typename Right, bool LeftConsumable,
  ///     bool RightConsumable>
  /// class Operation : public BinaryInterface<Operation<Result, Left, Right,
  ///     LeftConsumable, RightConsumable>, LeftConsumable, RightConsumable>
  /// {
  /// public:
  ///   typedef Operation<Result, Left, Right, LeftConsumable, RightConsumable> Operation_;
  ///   typedef BinaryInterface<Operation_, LeftConsumable, RightConsumable> BinaryInterface_;
  ///   typedef typename BinaryInterface_::first_argument_type first_argument_type;
  ///   typedef typename BinaryInterface_::second_argument_type second_argument_type;
  ///   typedef typename BinaryInterface_::zero_left_type zero_left_type;
  ///   typedef typename BinaryInterface_::zero_right_type zero_right_type;
  ///   typedef typename BinaryInterface_::result_type result_type;
  ///
  /// public:
  ///
  ///   // Permuting tile evaluation function
  ///
  ///   result_type permute_op(const Left& first, const Right& second) const {
  ///     // ...
  ///   }
  ///
  ///   result_type permute_op(zero_left_type, const Right& second) const {
  ///     // ...
  ///   }
  ///
  ///   result_type permute_op(const Left& first, zero_right_type) const {
  ///     // ...
  ///   }
  ///
  ///   // Non-permuting tile evaluation functions
  ///   // The compiler will select the correct functions based on the
  ///   // type ane consumability of the arguments.
  ///
  ///   template <bool LC, bool RC>
  ///   typename std::enable_if<! ((LC && std::is_same<Result, Left>::value) ||
  ///       (RC && std::is_same<Result, Right>::value)), result_type>::type
  ///   no_permute_op(const Left& first, const Right& second) {
  ///     // ...
  ///   }
  ///
  ///   template <bool LC, bool RC>
  ///   typename std::enable_if<LC && std::is_same<Result, Left>::value, result_type>::type
  ///   no_permute_op(Left& first, const Right& second) {
  ///      // ...
  ///   }
  ///
  ///   template <bool LC, bool RC>
  ///   typename std::enable_if<(RC && std::is_same<Result, Right>::value) &&
  ///       (!(LC && std::is_same<Result, Left>::value)), result_type>::type
  ///   no_permute_op(const Left& first, Right& second) {
  ///     // ...
  ///   }
  ///
  ///   template <bool LC, bool RC>
  ///   typename std::enable_if<! RC, result_type>::type
  ///   no_permute_op(zero_left_type, const Right& second) {
  ///     // ...
  ///   }
  ///
  ///   template <bool LC, bool RC>
  ///   typename std::enable_if<RC, result_type>::type
  ///   no_permute_op(zero_left_type, Right& second) {
  ///     // ...
  ///   }
  ///
  ///   template <bool LC, bool RC>
  ///   typename std::enable_if<! LC, result_type>::type
  ///   no_permute_op(const Left& first, zero_right_type) {
  ///     // ...
  ///   }
  ///
  ///   template <bool LC, bool RC>
  ///   typename std::enable_if<LC, result_type>::type
  ///   no_permute_op(Left& first, zero_right_type) {
  ///     // ...
  ///   }
  ///
  ///   // Implement default constructor, copy constructor and assignment operator
  ///
  ///   // Import interface from base class
  ///   using BinaryInterface_::operator();
  ///
  /// }; // class Operation
  /// \endcode
  /// \tparam Derived The derived operation class type
  template <typename Derived>
  class BinaryInterface : public math::BinaryInterfaceBase<Derived> {
  public:
    typedef math::BinaryInterfaceBase<Derived> BinaryInterfaceBase_; ///< This class type
    typedef typename BinaryInterfaceBase_::first_argument_type first_argument_type; ///< The left-hand argument type
    typedef typename BinaryInterfaceBase_::second_argument_type second_argument_type; ///< The right-hand argument type
    typedef typename BinaryInterfaceBase_::result_type result_type; ///< The result tile type

  private:

    Permutation perm_; ///< The result permutation

  protected:

    // Import the derived accessor function
    using BinaryInterfaceBase_::derived;

  public:

    /// Default constructor
    BinaryInterface() : perm_() { }

    /// Permution constructor

    /// \param perm The permutation that will be applied in this operation
    explicit BinaryInterface(const Permutation& perm) : perm_(perm) { }

    /// Copy constructor

    /// \param other The object to be copied
    BinaryInterface(const BinaryInterface<Derived>& other) :
      perm_(other.perm_)
    { }

    /// Assignment operator that will be applied in this operation

    /// \param other The object to be copied
    /// \return A reference to this object
    BinaryInterface<Derived>& operator=(const BinaryInterface<Derived>& other) {
      perm_ = other.perm_;
      return *this;
    }

    /// Set the permutation that will be applied in this operation

    /// \param perm The permutation that will be applied in this operation
    void permutation(const Permutation& perm) { perm_ = perm; }

    /// Permutation accessor

    /// \return A const reference to this operation's permutation
    const Permutation& permutation() const { return perm_; }

    // Import interface of BinaryInterfaceBase
    using BinaryInterfaceBase_::operator();

    // The following operators will evaluate lazy tile and use the base class
    // interface functions to call the correct evaluation kernel.

    /// Evaluate two lazy tiles

    /// This function will evaluate the \c first and \c second , then pass the
    /// evaluated tiles to the appropriate \c BinaryInterfaceBase_::operator()
    /// function.
    /// \tparam L The left-hand, lazy tile type
    /// \tparam R The right-hand, lazy tile type
    /// \param first The left-hand, lazy tile argument
    /// \param second The right-hand, lazy tile argument
    /// \return The result tile from the binary operation applied to the
    /// evaluated \c first and \c second .
    template <typename L, typename R>
    typename std::enable_if<detail::is_lazy_tile<L>::value && detail::is_lazy_tile<R>::value,
        result_type>::type
    operator()(const L& first, const R& second) const {
      typename eval_trait<L>::type eval_first(first);
      typename eval_trait<R>::type eval_second(second);
      return operator()(eval_first, eval_second);
    }

    /// Evaluate lazy and non-lazy tiles

    /// This function will evaluate the \c first , then pass the
    /// evaluated tile and \c second to the appropriate
    /// \c BinaryInterfaceBase_::operator() function.
    /// \tparam L The left-hand, lazy tile type
    /// \tparam R The right-hand, non-lazy tile type
    /// \param first The left-hand, lazy tile argument
    /// \param second The right-hand, non-lazy tile argument
    /// \return The result tile from the binary operation applied to the
    /// evaluated \c first and \c second .
    template <typename L, typename R>
    typename std::enable_if<
        detail::is_lazy_tile<L>::value &&
        (! detail::is_lazy_tile<typename std::remove_const<R>::type >::value),
        result_type>::type
    operator()(const L& first, R& second) const {
      typename eval_trait<L>::type eval_first(first);
      return operator()(eval_first, second);
    }

    /// Evaluate non-lazy and lazy tiles

    /// This function will evaluate the \c second , then pass the
    /// evaluated tile and \c first to the appropriate
    /// \c BinaryInterfaceBase_::operator() function.
    /// \tparam L The left-hand, non-lazy tile type
    /// \tparam R The right-hand, lazy tile type
    /// \param first The left-hand, non-lazy tile argument
    /// \param second The right-hand, lazy tile argument
    /// \return The result tile from the binary operation applied to the
    /// evaluated \c first and \c second .
    template <typename L, typename R>
    typename std::enable_if<
        (! detail::is_lazy_tile<typename std::remove_const<L>::type>::value) &&
        detail::is_lazy_tile<R>::value,
        result_type>::type
    operator()(L& first, const R& second) const {
      typename eval_trait<R>::type eval_second(second);
      return operator()(first, eval_second);
    }

  }; // class BinaryInterface


  /// Binary tile operation interface

  /// In addition to the interface defined by \c BinaryInterfaceBase, this
  /// class defines binary operations with lazy tiles. It will evaluate
  /// arguments as necessary and pass them to the \c BinaryInterfaceBase
  /// interface functions. This specialization is necessary to handle runtime
  /// consumable resources, when the tiles are not marked as consumable at
  /// compile time.
  /// \tparam Result The result tile type
  /// \tparam Left The left-hand tile type
  /// \tparam Right The right-hand tile type
  /// \tparam Op The derived class template
  template <typename Result, typename Left, typename Right,
      template <typename, typename, typename, bool, bool> class Op>
  class BinaryInterface<Op<Result, Left, Right, false, false> > :
      public math::BinaryInterfaceBase<Op<Result, Left, Right, false, false> >
  {
  public:
    typedef math::BinaryInterfaceBase<Op<Result, Left, Right, false, false> > BinaryInterfaceBase_; ///< This class type
    typedef typename BinaryInterfaceBase_::first_argument_type first_argument_type; ///< The left-hand argument type
    typedef typename BinaryInterfaceBase_::second_argument_type second_argument_type; ///< The right-hand argument type
    typedef typename BinaryInterfaceBase_::result_type result_type; ///< The result tile type

  private:

    Permutation perm_; ///< The result permutation

  protected:

    // Import the derived accessor function
    using BinaryInterfaceBase_::derived;

  public:

    /// Default constructor
    BinaryInterface() : perm_() { }

    /// Permution constructor

    /// \param perm The permutation that will be applied in this operation
    explicit BinaryInterface(const Permutation& perm) : perm_(perm) { }

    /// Copy constructor

    /// \param other The object to be copied
    BinaryInterface(const BinaryInterface<Op<Result, Left, Right, false, false> >& other) :
      perm_(other.perm_)
    { }

    /// Assignment operator that will be applied in this operation

    /// \param other The object to be copied
    /// \return A reference to this object
    BinaryInterface<Op<Result, Left, Right, false, false> >&
    operator=(const BinaryInterface<Op<Result, Left, Right, false, false> >& other) {
      perm_ = other.perm_;
      return *this;
    }

    /// Set the permutation that will be applied in this operation

    /// \param perm The permutation that will be applied in this operation
    void permutation(const Permutation& perm) { perm_ = perm; }

    /// Permutation accessor

    /// \return A const reference to this operation's permutation
    const Permutation& permutation() const { return perm_; }

    // Import interface of BinaryInterfaceBase
    using BinaryInterfaceBase_::operator();

    /// Evaluate two lazy-array tiles

    /// This function will evaluate the \c first and \c second , then pass the
    /// evaluated tiles to the appropriate \c Derived class evaluation kernel.
    /// \tparam L The left-hand, lazy-array tile type
    /// \tparam R The right-hand, lazy-array tile type
    /// \param first The left-hand, non-lazy tile argument
    /// \param second The right-hand, lazy tile argument
    /// \return The result tile from the binary operation applied to the
    /// evaluated \c first and \c second .
    template <typename L, typename R>
    typename std::enable_if<detail::is_array_tile<L>::value && detail::is_array_tile<R>::value,
        result_type>::type
    operator()(const L& first, const R& second) const {
      typename eval_trait<L>::type eval_first(first);
      typename eval_trait<R>::type eval_second(second);

      if(perm_)
        return derived().permute_op(eval_first, eval_second);

      if(first.is_consumable())
        return derived().template no_permute_op<true, false>(eval_first, eval_second);
      else if(second.is_consumable())
        return derived().template no_permute_op<false, true>(eval_first, eval_second);

      return derived().template no_permute_op<false, false>(eval_first, eval_second);
    }


    template <typename L, typename R>
    typename std::enable_if<
        detail::is_array_tile<L>::value &&
        (! detail::is_lazy_tile<typename std::remove_const<R>::type>::value),
        result_type>::type
    operator()(const L& first, R& second) const {
      typename eval_trait<L>::type eval_first(first);

      if(perm_)
        return derived().permute_op(eval_first, second);

      if(first.is_consumable())
        return derived().template no_permute_op<true, false>(eval_first, second);

      return derived().template no_permute_op<false, false>(eval_first, second);
    }


    template <typename L, typename R>
    typename std::enable_if<
        (! detail::is_lazy_tile<typename std::remove_const<L>::type>::value) &&
        detail::is_array_tile<R>::value,
        result_type>::type
    operator()(L& first, const R& second) const {
      typename eval_trait<R>::type eval_second(second);

      if(perm_)
        return derived().permute_op(first, eval_second);

      if(second.is_consumable())
        return derived().template no_permute_op<false, true>(first, eval_second);

      return derived().template no_permute_op<false, false>(first, eval_second);
    }

    template <typename L, typename R>
    typename std::enable_if<
        detail::is_non_array_lazy_tile<L>::value && detail::is_non_array_lazy_tile<R>::value,
        result_type>::type
    operator()(const L& first, const R& second) const {
      typename eval_trait<L>::type eval_first(first);
      typename eval_trait<R>::type eval_second(second);
      return operator()(eval_first, eval_second);
    }


    template <typename L, typename R>
    typename std::enable_if<
        detail::is_non_array_lazy_tile<L>::value &&
        (! detail::is_non_array_lazy_tile<typename std::remove_const<R>::type>::value),
        result_type>::type
    operator()(const L& first, R& second) const {
      typename eval_trait<L>::type eval_first(first);
      return operator()(eval_first, second);
    }


    template <typename L, typename R>
    typename std::enable_if<
        (! detail::is_non_array_lazy_tile<typename std::remove_const<L>::type>::value) &&
        detail::is_non_array_lazy_tile<R>::value,
        result_type>::type
    operator()(L& first, const R& second) const {
      typename eval_trait<R>::type eval_second(second);
      return operator()(first, eval_second);
    }

  }; // class BinaryInterface

} // namespace TiledArray

#endif // TILEDARRAY_TILE_OP_BINARY_INTERFACE_H__INCLUDED
