/*
 * Copyright (C) 2016-2019 i.Dark_Templar <darktemplar@dark-templar-archives.net>
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

#if USE_BOOST

#include <boost/optional.hpp>

template <typename T>
using optional = boost::optional<T>;

#else /* USE_BOOST */

#include <experimental/optional>

template <typename T>
using optional = std::experimental::optional<T>;

#endif /* USE_BOOST */

namespace dtcue {

enum class track_type
{
	unknown = 0,
	audio
};

enum class track_flags
{
	flag_none = 0x00,
	flag_dcp  = 0x01,
	flag_4ch  = 0x02,
	flag_pre  = 0x04,
	flag_scms = 0x08
};

track_flags operator|(track_flags lhs, track_flags rhs);
track_flags& operator|=(track_flags &lhs, track_flags rhs);
track_flags operator&(track_flags lhs, track_flags rhs);
track_flags& operator&=(track_flags &lhs, track_flags rhs);

struct time_point
{
	std::string minutes;
	std::string seconds;
	std::string frames;
};

struct file_time_point
{
	size_t file_index;
	time_point time;

	file_time_point()
		: file_index(0)
	{
	}
};

struct track
{
	std::map<std::string, std::string> tags;

	unsigned int track_index;
	track_type type;

	track_flags flags;
	optional<time_point> pregap;
	optional<time_point> postgap;

	std::vector<std::string> files;
	std::map<unsigned int, file_time_point> indices;

	track()
		: track_index(0),
		type(track_type::unknown),
		flags(track_flags::flag_none)
	{
	}
};

struct cue
{
	std::string cdtextfile;

	std::map<std::string, std::string> tags;

	std::vector<track> tracks;
};

cue parse_cue_file(const std::string &filename);

} // namespace dtcue

#endif /* DT_CUE_LIBRARY_HPP */
