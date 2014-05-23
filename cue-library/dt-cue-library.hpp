/*
 * Copyright (C) 2016 i.Dark_Templar <darktemplar@dark-templar-archives.net>
 *
 * This file is part of DT Cue Tools.
 *
 * DT Cue Tools is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * DT Cue Tools is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with DT Cue Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef DT_CUE_LIBRARY_HPP
#define DT_CUE_LIBRARY_HPP

#include <string>
#include <map>
#include <vector>

#include <boost/optional.hpp>

namespace dtcue {

enum class track_type
{
	audio,
	unknown
};

struct time_point
{
	std::string minutes;
	std::string seconds;
	std::string chunks_of_seconds;
};

struct file_chunk
{
	std::string filename;

	boost::optional<time_point> start_point;
	boost::optional<time_point> end_point;
};

struct track
{
	std::map<std::string, std::string> tags;

	std::vector<file_chunk> file_chunks;

	track_type type;
};

struct cue
{
	std::map<std::string, std::string> tags;

	std::vector<track> tracks;
};

cue parse_cue_file(const std::string &filename);

} // namespace dtcue

#endif /* DT_CUE_LIBRARY_HPP */
