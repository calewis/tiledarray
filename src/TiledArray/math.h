#ifndef TILEDARRAY_MATH_H__INCLUDED
#define TILEDARRAY_MATH_H__INCLUDED

#include <TiledArray/annotated_array.h>
#include <world/typestuff.h>
#include <Eigen/Core>
#include <functional>

namespace TiledArray {
  namespace math {


    template <typename Res, typename LeftArg, typename RightArg, template <typename> class Op>
    struct BinaryOp { };

    template <typename Res, typename Arg, template <typename> class Op>
    struct UnaryOp {

      template <typename ResArray, typename ArgArray>
      void operator()(Res& res, const Arg& arg) {

      }


    };

    template<typename I>
    class ContractedArray {
    public:
      typedef std::vector<I> size_array;
      typedef std::array<I, 3> packed_size_array;

      template<typename LeftSize, typename RightSize>
      ContractedArray(const LeftSize& lsize, const expressions::VariableList& lvars,
          const RightSize& rsize, const expressions::VariableList& rvars,
          detail::DimensionOrderType o) :
          vars_(lvars * rvars),
          order_(o)
      {
        typedef std::pair<expressions::VariableList::const_iterator,
            expressions::VariableList::const_iterator> vars_iter_pair;
        typedef std::pair<typename LeftSize::const_iterator, typename LeftSize::const_iterator> lsize_iter_pair;
        typedef std::pair<typename RightSize::const_iterator, typename RightSize::const_iterator> rsize_iter_pair;

        // find common variable lists
        vars_iter_pair lvars_common;
        vars_iter_pair rvars_common;
        expressions::detail::find_common(lvars.begin(), lvars.end(), rvars.begin(),
            rvars.end(), lvars_common, rvars_common);

        lsize_iter_pair lsize_common = get_common_range_iterators(lsize.begin(),
            std::distance(lvars.begin(), lvars_common.first),
            std::distance(lvars.begin(), lvars_common.second));
        rsize_iter_pair rsize_common = get_common_range_iterators(rsize.begin(),
            std::distance(rvars.begin(), rvars_common.first),
            std::distance(rvars.begin(), rvars_common.second));

        TA_ASSERT(std::lexicographical_compare(lsize_common.first,
            lsize_common.second, rsize_common.first, rsize_common.second),
            std::runtime_error, "The common start dimensions do not match.");

        // find dimensions of the result tile
        size_.resize(vars_.dim(), 0);
        start_.resize(vars_.dim(), 0);
        finish_.resize(vars_.dim(), 0);
        typename size_array::iterator size_it = size_.begin();
        typename size_array::iterator size_end = size_.end();
        typename size_array::iterator finish_it = finish_it.begin();
        expressions::VariableList::const_iterator v_it = vars_.begin();
        expressions::VariableList::const_iterator lvar_begin = lvars.begin();
        expressions::VariableList::const_iterator lvar_end = lvars.end();
        expressions::VariableList::const_iterator rvar_begin = rvars.begin();
        expressions::VariableList::const_iterator rvar_end = rvars.end();
        expressions::VariableList::const_iterator find_it;
        std::iterator_traits<expressions::VariableList::const_iterator>::difference_type n = 0;
        for(; size_it != size_end; ++size_it, ++finish_it, ++v_it) {
          if((find_it = std::find(lvar_begin, lvar_end, *v_it)) != lvars.end()) {
            n  = std::distance(lvar_begin, find_it);
            *size_it = *finish_it = lsize[n];
          } else {
            find_it = std::find(rvar_begin, rvar_end, *v_it);
            n  = std::distance(rvar_begin, find_it);
            *size_it = *finish_it = lsize[n];
          }
        }

        // calculate packed tile dimensions
        const I init = 1;
        packed_left_size_[0] = std::accumulate(lsize.begin(), lsize_common.first,
            init, std::multiplies<I>());
        packed_left_size_[2] = std::accumulate(lsize_common.second, lsize.end(),
            init, std::multiplies<I>());
        packed_right_size_[0] = std::accumulate(rsize.begin(), rsize_common.fist,
            init, std::multiplies<I>());
        packed_right_size_[2] = std::accumulate(rsize_common.second, rsize.end(),
            init, std::multiplies<I>());
        packed_left_size_[1] = std::accumulate(lsize_common.first, lsize_common.second,
            init, std::multiplies<std::size_t>());
        packed_right_size_[1] = packed_left_size_[1];
      }

      template<typename LeftIndex, typename RightIndex>
      ContractedArray(const LeftIndex& lstart, const LeftIndex& lfinish,
          const expressions::VariableList& lvars,
          const RightIndex& rstart, const RightIndex& rfinish,
          const expressions::VariableList& rvars,
          detail::DimensionOrderType o) :
          vars_(lvars * rvars),
          order_(o)
      {
        typedef std::pair<expressions::VariableList::const_iterator,
            expressions::VariableList::const_iterator> vars_iter_pair;
        typedef std::pair<typename LeftIndex::const_iterator,
            typename LeftIndex::const_iterator> lindex_iter_pair;
        typedef std::pair<typename RightIndex::const_iterator,
            typename RightIndex::const_iterator> rindex_iter_pair;

        // find common variable lists
        vars_iter_pair lvars_common;
        vars_iter_pair rvars_common;
        expressions::detail::find_common(lvars.begin(), lvars.end(), rvars.begin(),
            rvars.end(), lvars_common, rvars_common);

        lindex_iter_pair lstart_common = get_common_range_iterators(lstart.begin(),
            std::distance(lvars.begin(), lvars_common.first),
            std::distance(lvars.begin(), lvars_common.second));
        rindex_iter_pair rstart_common = get_common_range_iterators(rstart.begin(),
            std::distance(rvars.begin(), rvars_common.first),
            std::distance(rvars.begin(), rvars_common.second));

        lindex_iter_pair lfinish_common = get_common_range_iterators(lfinish.begin(),
            std::distance(lvars.begin(), lvars_common.first),
            std::distance(lvars.begin(), lvars_common.second));
        rindex_iter_pair rfinish_common = get_common_range_iterators(rfinish.begin(),
            std::distance(rvars.begin(), rvars_common.first),
            std::distance(rvars.begin(), rvars_common.second));


        TA_ASSERT(std::lexicographical_compare(lstart_common.first,
            lstart_common.second, rstart_common.first, rstart_common.second),
            std::runtime_error, "The common start dimensions do not match.");
        TA_ASSERT(std::lexicographical_compare(lfinish_common.first,
            lfinish_common.second, rfinish_common.first, rfinish_common.second),
            std::runtime_error, "The common finish dimensions do not match.");

        // find dimensions of the result tile
        size_.resize(vars_.dim(), 0);
        start_.resize(vars_.dim(), 0);
        finish_.resize(vars_.dim(), 0);
        typename size_array::iterator size_it = size_.begin();
        typename size_array::iterator start_it = start_.begin();
        typename size_array::iterator finish_it = finish_it.begin();
        typename size_array::iterator size_end = size_.end();
        expressions::VariableList::const_iterator v_it = vars_.begin();
        expressions::VariableList::const_iterator lvar_begin = lvars.begin();
        expressions::VariableList::const_iterator lvar_end = lvars.end();
        expressions::VariableList::const_iterator rvar_begin = rvars.begin();
        expressions::VariableList::const_iterator rvar_end = rvars.end();
        expressions::VariableList::const_iterator find_it;
        std::iterator_traits<expressions::VariableList::const_iterator>::difference_type n = 0;
        for(; size_it != size_end; ++size_it, ++start_it, ++finish_it, ++v_it) {
          if((find_it = std::find(lvar_begin, lvar_end, *v_it)) != lvars.end()) {
            n  = std::distance(lvar_begin, find_it);
            *start_it = lstart[n];
            *finish_it = lfinish[n];
            *size_it = *finish_it - *start_it;
          } else {
            find_it = std::find(rvar_begin, rvar_end, *v_it);
            n  = std::distance(rvar_begin, find_it);
            *start_it = rstart[n];
            *finish_it = rfinish[n];
            *size_it = *finish_it - *start_it;
          }
        }

        // calculate packed tile dimensions
        packed_left_size_[0] = accumulate<I>(lfinish.begin(), lfinish_common.first, lstart.begin());
        packed_left_size_[2] = accumulate<I>(lfinish_common.second, lfinish.end(), lstart_common.second);
        packed_right_size_[0] = accumulate<I>(rfinish.begin(), rfinish_common.first, rstart.begin());
        packed_right_size_[2] = accumulate<I>(rfinish_common.second, rfinish.end(), rstart_common.second);
        packed_left_size_[1] = accumulate<I>(lfinish_common.first, lfinish_common.second, lstart_common.first);
        packed_right_size_[1] = packed_left_size_[1];
      }

      const size_array& size() const { return size_; }
      const size_array& start() const { return start_; }
      const size_array& finish() const { return finish_; }
      const packed_size_array& packed_left_size() const { return packed_left_size_; }
      const packed_size_array& packed_right_size() const { return packed_right_size_; }
      const expressions::VariableList& vars() const { return vars_; }
      detail::DimensionOrderType& order() const { return order_; }

      I m() const { return packed_left_size_[0]; }
      I n() const { return packed_left_size_[2]; }
      I o() const { return packed_right_size_[0]; }
      I p() const { return packed_right_size_[2]; }
      I i() const { return packed_left_size_[1]; }

    private:

      template<typename T, typename InIter1, typename InIter2, typename AccOp, typename BinOp>
      static T accumulate(InIter1 first1, InIter1 last1, InIter2 first2) {
        T initial = 1;
        while(first1 != last1)
          initial *= *first1++ - *first2++;

        return initial;
      }

      template<typename InIter, typename Diff>
      static std::pair<InIter, InIter> get_common_range_iterators(InIter first, Diff d1, Diff d2) {
        std::pair<InIter, InIter> result(first, first);
        std::advance(result.first, d1);
        std::advance(result.second, d2);

        return result;
      }

      expressions::VariableList vars_;
      size_array size_;
      size_array start_;
      size_array finish_;
      packed_size_array packed_left_size_;
      packed_size_array packed_right_size_;
      detail::DimensionOrderType order_;
    }; // class ContractedData

    /// Contract a and b, and place the results into c.
    /// c[m,o,n,p] = a[m,i,n] * b[o,i,p]
    template<detail::DimensionOrderType D, typename I, typename T>
    void contract(const I& m, const I& n, const I& o,
        const I& p, const I& i, const T* a, const T* b, T* c)
    {
      typedef Eigen::Matrix< T , Eigen::Dynamic , Eigen::Dynamic,
          (D == detail::decreasing_dimension_order ? Eigen::RowMajor : Eigen::ColMajor) | Eigen::AutoAlign > matrix_type;

      // determine the lower order dimension size
      const std::size_t ma1 = ( D == detail::increasing_dimension_order ? m : n );
      const std::size_t mb1 = ( D == detail::increasing_dimension_order ? o : p );

      // calculate iterator step sizes.
      const std::size_t a_step = i * ma1;
      const std::size_t b_step = i * mb1;
      const std::size_t c_step = ma1 * mb1;

      // calculate iterator boundaries
      const T* a_begin = NULL;
      const T* b_begin = NULL;
      T* c_begin = c;
      const T* const a_end = a + (m * i * n);
      const T* const b_end = b + (o * i * p);
//      const T* const c_end = c + (m * n * o * p);

      // iterate over the highest order dimensions of a and b, and store the
      // results of the matrix-matrix multiplication.
      for(a_begin = a; a_begin != a_end; a_begin += a_step) {
        Eigen::Map<matrix_type> ma(a_begin, i, ma1);
        for(b_begin = b; b_begin != b_end; b_begin += b_step, c_begin += c_step) {
          Eigen::Map<matrix_type> mb(b_begin, i, mb1);
          Eigen::Map<matrix_type> mc(c_begin, ma1, mb1);

          mc = ma.transpose() * mb;
        }
      }
    }

  } // namespace math
} // namespace TiledArray

#endif // TILEDARRAY_MATH_H__INCLUDED
