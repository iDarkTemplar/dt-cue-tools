/*
 * Copyright (C) 2018 i.Dark_Templar <darktemplar@dark-templar-archives.net>
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

#ifndef DT_CUE_ACTION_HPP
#define DT_CUE_ACTION_HPP

#include <string>

#include <dt-cue-library.hpp>

#include "cue-types.hpp"

namespace dtcue {

class command
{
public:
	virtual ~command() = default;

	virtual void run() const = 0;
	virtual std::string print() const = 0;
};

class external_command: public command
{
public:
	external_command(const std::string &command_string);

	virtual void run() const;
	virtual std::string print() const;

private:
	std::string m_command_string;
};

} // namespace dtcue

#endif /* DT_CUE_ACTION_HPP */
