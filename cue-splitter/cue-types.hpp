/*
 * Copyright (C) 2018-2019 i.Dark_Templar <darktemplar@dark-templar-archives.net>
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

#ifndef DT_CUE_TYPES_HPP
#define DT_CUE_TYPES_HPP

#if USE_BOOST

#include <boost/regex.hpp>
#include <boost/optional.hpp>

template <typename T>
using optional = boost::optional<T>;

using regex = boost::regex;
using smatch = boost::smatch;

#else /* USE_BOOST */

#include <regex>
#include <experimental/optional>

template <typename T>
using optional = std::experimental::optional<T>;

using regex = std::regex;
using smatch = std::smatch;

#endif /* USE_BOOST */

#endif /* DT_CUE_TYPES_HPP */