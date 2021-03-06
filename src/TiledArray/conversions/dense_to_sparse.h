#pragma once
#ifndef TILEDARRAY_DENSETOSPARSE_H__INCLUDED
#define TILEDARRAY_DENSETOSPARSE_H__INCLUDED

#include <TiledArray/array.h>

namespace TiledArray {

  /// Function to convert a dense array into a block sparse array

  /// If the input array is dense then create a copy by checking the norms of the
  /// tiles in the dense array and then cloning the significant tiles into the
  /// sparse array.
  template <typename T, unsigned int DIM, typename Tile>
  Array<T, DIM, Tile, SparsePolicy>
  to_sparse(Array<T, DIM, Tile, DensePolicy> const &dense_array) {
      typedef Array<T, DIM, Tile, SparsePolicy> ArrayType;  // return type

      // Constructing a tensor to hold the norm of each tile in the Dense Array
      TiledArray::Tensor<float> tile_norms(dense_array.trange().tiles(), 0.0);

      const auto end = dense_array.end();
      const auto begin = dense_array.begin();
      for (auto it = begin; it != end; ++it) {
          // write the norm of each local tile to the tensor
          tile_norms[it.ordinal()] = it->get().norm();
      }

      // Construct a sparse shape the constructor will handle communicating the
      // norms of the local tiles to the other nodes
      TiledArray::SparseShape<float> shape(dense_array.get_world(), tile_norms,
                                           dense_array.trange());

      ArrayType sparse_array(dense_array.get_world(), dense_array.trange(),
                             shape);

      // Loop over the local dense tiles and if that tile is in the
      // sparse_array set the sparse array tile with a clone so as not to hold
      // a pointer to the original tile.
      for (auto it = begin; it != end; ++it) {
          const auto ord = it.ordinal();
          if (!sparse_array.is_zero(ord)) {
              sparse_array.set(ord, it->get().clone());
          }
      }

      return sparse_array;
  }

  /// If the array is already sparse return a copy of the array.
  template <typename T, unsigned int DIM, typename Tile>
  Array<T, DIM, Tile, SparsePolicy>
  to_sparse(Array<T, DIM, Tile, SparsePolicy> const &sparse_array) {
      return sparse_array;
  }

}  // namespace TiledArray

#endif /* end of include guard: TILEDARRAY_DENSETOSPARSE_H__INCLUDED */
