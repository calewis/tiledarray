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
 *  expr.h
 *  Apr 1, 2014
 *
 */

#ifndef TILEDARRAY_EXPRESSIONS_EXPR_H__INCLUDED
#define TILEDARRAY_EXPRESSIONS_EXPR_H__INCLUDED

#include <TiledArray/expressions/expr_engine.h>
#include <TiledArray/reduce_task.h>
#include <TiledArray/tile_op/unary_reduction.h>
#include <TiledArray/tile_op/binary_reduction.h>
#include <TiledArray/tile_op/reduce_wrapper.h>

namespace TiledArray {

  // Forward declaration
  template <typename, unsigned int, typename, typename> class Array;

  namespace expressions {

    // Forward declaration
    template <typename> struct ExprTrait;


    /// Base class for expression evaluation

    /// \tparam Derived The derived class type
    template <typename Derived>
    class Expr {
    public:

      typedef Expr<Derived> Expr_; ///< This class type
      typedef Derived derived_type; ///< The derived object type
      typedef typename ExprTrait<Derived>::engine_type engine_type; ///< Expression engine type

    private:

      Expr<Derived>& operator=(const Expr<Derived>&);

      /// Task function used to evaluate lazy tiles

      /// \tparam T The lazy tile type
      /// \param tile The lazy tile
      /// \return The evaluated tile
      template <typename T>
      static typename TiledArray::eval_trait<T>::type eval_tile(const T& tile) {
        return tile;
      }

      /// Set an array tile with a lazy tile

      /// Spawn a task to evaluate a lazy tile and set the \a array tile at
      /// \c index with the result.
      /// \tparam A The array type
      /// \tparam I The index type
      /// \tparam T The lazy tile type
      /// \param array The result array
      /// \param index The tile index
      /// \param tile The lazy tile
      template <typename A, typename I, typename T>
      typename std::enable_if<TiledArray::detail::is_lazy_tile<T>::value>::type
      set_tile(A& array, const I index, const Future<T>& tile) const {
        array.set(index, array.get_world().taskq.add(
              & Expr_::template eval_tile<T>, tile));
      }

      /// Set the \c array tile at \c index with \c tile

      /// \tparam A The array type
      /// \tparam I The index type
      /// \tparam T The lazy tile type
      /// \param array The result array
      /// \param index The tile index
      /// \param tile The tile
      template <typename A, typename I, typename T>
      typename std::enable_if<! TiledArray::detail::is_lazy_tile<T>::value>::type
      set_tile(A& array, const I index, const Future<T>& tile) const {
        array.set(index, tile);
      }

      /// Array factor function

      /// Construct an array that will hold the result of this expression
      /// \tparam A The output array type
      /// \param world The world that will hold the result
      /// \param pmap The process map for the result
      /// \param target_vars The target variable list
      template <typename A>
      A make_array(World& world, const std::shared_ptr<typename A::pmap_interface>& pmap,
          const VariableList& target_vars) const
      {
        // Construct the expression engine
        engine_type engine(derived());
        engine.init(world, pmap, target_vars);

        // Create the distributed evaluator from this expression
        typename engine_type::dist_eval_type dist_eval = engine.make_dist_eval();
        dist_eval.eval();

        // Create the result array
        A result(dist_eval.get_world(), dist_eval.trange(),
            dist_eval.shape(), dist_eval.pmap());

        // Move the data from dist_eval into the result array
        auto it = dist_eval.pmap()->begin();
        const auto end = dist_eval.pmap()->end();
        for(; it != end; ++it) {
          const auto index = *it;
          if(! dist_eval.is_zero(index))
            set_tile(result, index, dist_eval.get(index));
        }

        // Wait for child expressions of dist_eval
        dist_eval.wait();

        return result;
      }

    public:

      /// Cast this object to it's derived type
      derived_type& derived() { return *static_cast<derived_type*>(this); }

      /// Cast this object to it's derived type
      const derived_type& derived() const { return *static_cast<const derived_type*>(this); }

      /// Evaluate this object and assign it to \c tsr

      /// This expression is evaluated in parallel in distributed environments,
      /// where the content of \c tsr will be replace by the results of the
      /// evaluated tensor expression.
      /// \tparam A The array type
      /// \param tsr The tensor to be assigned
      template <typename A>
      void eval_to(TsrExpr<A>& tsr) const {

        // Get the target world.
        World& world = (tsr.array().is_initialized() ?
            tsr.array().get_world() :
            World::get_default());

        // Get the output process map.
        std::shared_ptr<typename TsrExpr<A>::array_type::pmap_interface> pmap;
        if(tsr.array().is_initialized())
          pmap = tsr.array().get_pmap();

        // Get result variable list.
        VariableList target_vars(tsr.vars());

        // Swap the new array with the result array object.
        make_array<A>(world, pmap, target_vars).swap(tsr.array());
      }

      /// Array conversion operator

      /// \tparam T The array element type
      /// \tparam DIM The array dimension
      /// \tparam Tile The array tile type
      /// \tparam Policy The array policy type
      /// \return A array object that holds the result of this expression
//      template <typename T, unsigned int DIM, typename Tile, typename Policy>
//      explicit operator Array<T, DIM, Tile, Policy>() {
//        return make_array<Array<T, DIM, Tile, Policy> >(World::get_default(),
//            std::shared_ptr<typename Array<T, DIM, Tile, Policy>::pmap_interface>(),
//            VariableList());
//      }

      /// Expression print

      /// \param os The output stream
      /// \param target_vars The target variable list for this expression
      void print(ExprOStream& os, const VariableList& target_vars) const {
        // Construct the expression engine
        engine_type engine(derived());
        engine.init_vars(target_vars);
        engine.init_struct(target_vars);
        engine.print(os, target_vars);
      }

    private:

      struct ExpressionReduceTag { };

    public:

      template <typename Op>
      Future<typename Op::result_type>
      reduce(const Op& op, World& world = World::get_default()) const {
        // Typedefs
        typedef madness::TaggedKey<madness::uniqueidT, ExpressionReduceTag> key_type;
        typedef TiledArray::math::UnaryReduceWrapper<typename engine_type::value_type,
            Op> reduction_op_type;

        // Construct the expression engine
        engine_type engine(derived());
        engine.init(world, std::shared_ptr<typename engine_type::pmap_interface>(),
            VariableList());

        // Create the distributed evaluator from this expression
        typename engine_type::dist_eval_type dist_eval = engine.make_dist_eval();
        dist_eval.eval();

        // Create a local reduction task
        reduction_op_type wrapped_op(op);
        TiledArray::detail::ReduceTask<reduction_op_type> reduce_task(world, wrapped_op);

        // Move the data from dist_eval into the local reduction task
        typename engine_type::dist_eval_type::pmap_interface::const_iterator it =
            dist_eval.pmap()->begin();
        const typename engine_type::dist_eval_type::pmap_interface::const_iterator end =
            dist_eval.pmap()->end();
        for(; it != end; ++it)
          if(! dist_eval.is_zero(*it))
            reduce_task.add(dist_eval.get(*it));

        // All reduce the result of the expression
        return world.gop.all_reduce(key_type(dist_eval.id()), reduce_task.submit(), op);
      }

      template <typename D, typename Op>
      Future<typename Op::result_type>
      reduce(const Expr<D>& right_expr, const Op& op,
          World& world = World::get_default()) const
      {
        // Typedefs
        typedef madness::TaggedKey<madness::uniqueidT, ExpressionReduceTag> key_type;
        typedef TiledArray::math::BinaryReduceWrapper<typename engine_type::value_type,
            typename D::engine_type::value_type, Op> reduction_op_type;

        // Evaluate this expression
        engine_type left_engine(derived());
        left_engine.init(world, std::shared_ptr<typename engine_type::pmap_interface>(),
            VariableList());

        // Create the distributed evaluator for this expression
        typename engine_type::dist_eval_type left_dist_eval =
            left_engine.make_dist_eval();
        left_dist_eval.eval();

        // Evaluate the right-hand expression
        typename D::engine_type right_engine(right_expr.derived());
        right_engine.init(world, left_engine.pmap(), left_engine.vars());

        // Create the distributed evaluator for the right-hand expression
        typename D::engine_type::dist_eval_type right_dist_eval =
            right_engine.make_dist_eval();
        right_dist_eval.eval();

#ifndef NDEBUG
        if(left_dist_eval.trange() != right_dist_eval.trange()) {
          if(World::get_default().rank() == 0) {
            TA_USER_ERROR_MESSAGE( \
                "The TiledRanges of the left- and right-hand arguments the binary reduction are not equal:" \
                << "\n    left  = " << left_dist_eval.trange() \
                << "\n    right = " << right_dist_eval.trange() );
          }

          TA_EXCEPTION("The TiledRange objects of a binary expression are not equal.");
        }
#endif // NDEBUG

        // Create a local reduction task
        reduction_op_type wrapped_op(op);
        TiledArray::detail::ReducePairTask<reduction_op_type>
            local_reduce_task(world, wrapped_op);

        // Move the data from dist_eval into the local reduction task
        typename engine_type::dist_eval_type::pmap_interface::const_iterator it =
            left_dist_eval.pmap()->begin();
        const typename engine_type::dist_eval_type::pmap_interface::const_iterator end =
            left_dist_eval.pmap()->end();
        for(; it != end; ++it) {
          const typename engine_type::size_type index = *it;
          const bool left_not_zero = !left_dist_eval.is_zero(index);
          const bool right_not_zero = !right_dist_eval.is_zero(index);

          if(left_not_zero && right_not_zero) {
            local_reduce_task.add(left_dist_eval.get(index), right_dist_eval.get(index));
          } else {
            if(left_not_zero) left_dist_eval.get(index);
            if(right_not_zero) right_dist_eval.get(index);
          }
        }

        return world.gop.all_reduce(key_type(left_dist_eval.id()),
            local_reduce_task.submit(), op);
      }

      Future<typename ExprTrait<Derived>::scalar_type>
      trace(World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::eval_type value_type;
        return reduce(TiledArray::math::TraceReduction<value_type>(), world);
      }

      Future<typename ExprTrait<Derived>::scalar_type>
      sum(World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::eval_type value_type;
        return reduce(TiledArray::math::SumReduction<value_type>(), world);
      }

      Future<typename ExprTrait<Derived>::scalar_type>
      product(World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::eval_type value_type;
        return reduce(TiledArray::math::ProductReduction<value_type>(), world);
      }

      Future<typename ExprTrait<Derived>::scalar_type>
      squared_norm(World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::eval_type value_type;
        return reduce(TiledArray::math::SquaredNormReduction<value_type>(), world);
      }

    private:

      template <typename T>
      static T sqrt(const T t) { return std::sqrt(t); }

    public:

      Future<typename ExprTrait<Derived>::scalar_type>
      norm(World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::scalar_type scalar_type;
        return world.taskq.add(Expr_::template sqrt<scalar_type>, squared_norm(world));
      }

      Future<typename ExprTrait<Derived>::scalar_type>
      min(World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::eval_type value_type;
        return reduce(TiledArray::math::MinReduction<value_type>(), world);
      }

      Future<typename ExprTrait<Derived>::scalar_type>
      max(World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::eval_type value_type;
        return reduce(TiledArray::math::MaxReduction<value_type>(), world);
      }

      Future<typename ExprTrait<Derived>::scalar_type>
      abs_min(World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::eval_type value_type;
        return reduce(TiledArray::math::AbsMinReduction<value_type>(), world);
      }

      Future<typename ExprTrait<Derived>::scalar_type>
      abs_max(World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::eval_type value_type;
        return reduce(TiledArray::math::AbsMaxReduction<value_type>(), world);
      }

      template <typename D>
      Future<typename ExprTrait<Derived>::scalar_type>
      dot(const Expr<D>& right_expr, World& world = World::get_default()) const {
        typedef typename EngineTrait<engine_type>::eval_type left_value_type;
        typedef typename EngineTrait<typename D::engine_type>::eval_type right_value_type;
        return reduce(right_expr, TiledArray::math::DotReduction<left_value_type,
            right_value_type>(), world);
      }

    }; // class Expr

  } // namespace expressions
} // namespace TiledArray

#endif // TILEDARRAY_EXPRESSIONS_EXPR_H__INCLUDED
