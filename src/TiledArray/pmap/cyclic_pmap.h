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
 *  cyclic_pmap.h
 *  May 1, 2012
 *
 */

#ifndef TILEDARRAY_PMAP_CYCLIC_PMAP_H__INCLUDED
#define TILEDARRAY_PMAP_CYCLIC_PMAP_H__INCLUDED

#include <TiledArray/pmap/pmap.h>

namespace TiledArray {
  namespace detail {

    /// Map processes using a 2D cyclic decomposition

    /// This map cyclicly distributes a two-dimensional grid of tiles among a
    /// two-dimensional grid of processes.
    class CyclicPmap : public Pmap {
    protected:

      // Import Pmap protected variables
      using Pmap::rank_; ///< The rank of this process
      using Pmap::procs_; ///< The number of processes
      using Pmap::size_; ///< The number of tiles mapped among all processes
      using Pmap::local_; ///< A list of local tiles

    private:

      const size_type rows_; ///< Number of tile rows to be mapped
      const size_type cols_; ///< Number of tile columns to be mapped
      const size_type proc_cols_; ///< Number of process columns
      const size_type proc_rows_; ///< Number of process rows

    public:
      typedef Pmap::size_type size_type; ///< Size type

      /// Construct process map

      /// \param world The world where the tiles will be mapped
      /// \param rows The number of tile rows to be mapped
      /// \param cols The number of tile columns to be mapped
      /// \param proc_rows The number of process rows in the map
      /// \param proc_cols The number of process columns in the map
      /// \throw TiledArray::Exception When <tt>proc_rows > rows</tt>
      /// \throw TiledArray::Exception When <tt>proc_cols > cols</tt>
      /// \throw TiledArray::Exception When <tt>proc_rows * proc_cols > world.size()</tt>
      CyclicPmap(World& world, size_type rows, size_type cols,
          size_type proc_rows, size_type proc_cols) :
        Pmap(world, rows * cols), rows_(rows), cols_(cols),
        proc_cols_(proc_cols), proc_rows_(proc_rows)
      {
        // Check that the size is non-zero
        TA_ASSERT(rows_ >= 1ul);
        TA_ASSERT(cols_ >= 1ul);

        // Check limits of process rows and columns
        TA_ASSERT(proc_rows_ >= 1ul);
        TA_ASSERT(proc_cols_ >= 1ul);
        TA_ASSERT((proc_rows_ * proc_cols_) <= procs_);

        // Initialize local tile list
        if(rank_ < (proc_rows_ * proc_cols_)) {
          // Compute rank coordinates
          const size_type rank_row = rank_ / proc_cols_;
          const size_type rank_col = rank_ % proc_cols_;

          const size_type local_rows =
              (rows_ / proc_rows_) + ((rows_ % proc_rows_) < rank_row ? 1ul : 0ul);
          const size_type local_cols =
              (cols_ / proc_cols_) + ((cols_ % proc_cols_) < rank_col ? 1ul : 0ul);

          // Allocate memory for the local tile list
          local_.reserve(local_rows * local_cols);

          // Iterate over local tiles
          for(size_type i = rank_row; i < rows_; i += proc_rows_) {
            const size_type row_end = (i + 1) * cols_;
            for(size_type tile = i * cols_ + rank_col; tile < row_end; tile += proc_cols_) {
              TA_ASSERT(CyclicPmap::owner(tile) == rank_);
              local_.push_back(tile);
            }
          }
        }
      }

      virtual ~CyclicPmap() { }

      /// Maps \c tile to the processor that owns it

      /// \param tile The tile to be queried
      /// \return Processor that logically owns \c tile
      virtual size_type owner(const size_type tile) const {
        TA_ASSERT(tile < size_);
        // Compute tile coordinate in tile grid
        const size_type tile_row = tile / cols_;
        const size_type tile_col = tile % cols_;
        // Compute process coordinate of tile in the process grid
        const size_type proc_row = tile_row % proc_rows_;
        const size_type proc_col = tile_col % proc_cols_;
        // Compute the process that owns tile
        const size_type proc = proc_row * proc_cols_ + proc_col;

        TA_ASSERT(proc < procs_);

        return proc;
      }


      /// Check that the tile is owned by this process

      /// \param tile The tile to be checked
      /// \return \c true if \c tile is owned by this process, otherwise \c false .
      virtual bool is_local(const size_type tile) const {
        return (CyclicPmap::owner(tile) == rank_);
      }

    }; // class CyclicPmap

  }  // namespace detail
}  // namespace TiledArray


#endif // TILEDARRAY_PMAP_CYCLIC_PMAP_H__INCLUDED
