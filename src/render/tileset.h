/*
 * Copyright 2012, 2013 Moritz Hilscher
 *
 * This file is part of mapcrafter.
 *
 * mapcrafter is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * mapcrafter is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with mapcrafter.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TILE_H_
#define TILE_H_

#include "../mc/world.h"

#include <set>
#include <vector>
#include <boost/filesystem.hpp>

namespace fs = boost::filesystem;

/**
 * The renderer renders the world to tiles, which are arranged in a quadtree. Every node
 * is a tile with 2x2 (maximal 4) children. These children have numbers, depending on
 * their position:
 *   1: children is on the top left,
 *   2: top right,
 *   3: bottom left,
 *   4: bottom right
 *
 * The leaf nodes in the tree don't have children. These tiles are rendered directly
 * from the world data and are called render tiles or top level tiles. The other tiles
 * are composite tiles and are composed from their children tiles.
 *
 * The render tiles have a position. All tiles have a path from the root node to the tile.
 * The length of this path is the zoom level. The root node has the zoom level 0 and the
 * render tiles the maximum zoom level.
 *
 * The TileSet class manages the tiles for the rendering process. The render tiles are
 * stored as tile positions, all other composite tiles as tile paths. The tile set scans
 * the world and all chunks, calculates the maximum needed zoom level and finds out, which
 * tiles exist and which tiles need to get rendered (useful for incremental rendering,
 * when only a few chunks where changed).
 *
 * The tile renderer can then render the render tiles and compose the composite tiles
 * recursively from the render tiles.
 *
 * All tile images are stored on disk like the tree structure. The tile with the zoom
 * level 0 is "base.png". All children tiles are stored in directories 1/ 2/ 3/ 4/ and
 * images 1.png 2.png 3.png 4.png.
 */

namespace mapcrafter {
namespace render {

const int TILE_WIDTH = 1;

/**
 * This class represents the position of a render tile.
 */
class TilePos {
private:
	int x, y;
public:
	TilePos();
	TilePos(int x, int y);

	int getX() const;
	int getY() const;

	TilePos& operator+=(const TilePos& p);
	TilePos& operator-=(const TilePos& p);
	TilePos operator+(const TilePos& p2) const;
	TilePos operator-(const TilePos& p2) const;

	bool operator==(const TilePos& other) const;
	bool operator<(const TilePos& other) const;
};

/**
 * This class represents the path to a tile in the quadtree. Every part in the path
 * is a 1, 2, 3 or 4.
 * The length of the path is the zoom level of the tile.
 */
class TilePath {
private:
	std::vector<int> path;
public:
	TilePath();
	TilePath(const std::vector<int>& path);
	~TilePath();

	const std::vector<int>& getPath() const;
	int getDepth() const;
	TilePos getTilePos() const;

	TilePath& operator+=(int node);
	TilePath operator+(int node) const;

	TilePath parent() const;

	bool operator==(const TilePath& other) const;
	bool operator<(const TilePath& other) const;

	std::string toString() const;

	static TilePath byTilePos(const TilePos& tile, int depth);
};

std::ostream& operator<<(std::ostream& stream, const TilePath& path);
std::ostream& operator<<(std::ostream& stream, const TilePos& tile);

/**
 * This class manages all tiles required to render a world.
 */
class TileSet {
private:
	// the depth needed to render all tiles
	int min_depth;
	// depth of the tile set = maximum zoom level
	int depth;

	// all available top level tiles
	std::set<TilePos> render_tiles;
	// the top level tiles which actually need to get rendered
	std::set<TilePos> required_render_tiles;
	// timestamps of render tiles required to rerender a tile
	// (= highest timestamp of all chunks in a tile)
	std::map<TilePos, int> tile_timestamps;

	// same here for composite tiles
	std::set<TilePath> composite_tiles;
	std::set<TilePath> required_composite_tiles;

	// count of required render tiles (= tree leaves) in a composite tile
	std::map<TilePath, int> containing_render_tiles;

	void findRenderTiles(const mc::World& world);
	void findRequiredCompositeTiles(const std::set<TilePos>& render_tiles,
			std::set<TilePath>& tiles);

	void updateContainingRenderTiles();
public:
	TileSet();
	TileSet(const mc::World& World);
	virtual ~TileSet();

	void scan(const mc::World& world);
	void scanRequiredByTimestamp(int last_change);
	void scanRequiredByFiletimes(const fs::path& output_dir);

	int getMinDepth() const;
	int getDepth() const;
	void setDepth(int depth);

	bool hasTile(const TilePath& path) const;
	bool isTileRequired(const TilePath& path) const;

	const std::set<TilePos>& getAvailableRenderTiles() const;
	const std::set<TilePath>& getAvailableCompositeTiles() const;
	const std::set<TilePos>& getRequiredRenderTiles() const;
	const std::set<TilePath>& getRequiredCompositeTiles() const;

	int getRequiredRenderTilesCount() const;
	int getRequiredCompositeTilesCount() const;

	int getContainingRenderTiles(const TilePath& tile) const;

	int findRenderTasks(int worker_count, std::vector<std::map<TilePath, int> >& workers) const;
};

}
}

#endif /* TILE_H_ */
