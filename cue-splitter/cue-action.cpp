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

#include "cue-action.hpp"

#include <stdlib.h>

namespace dtcue {

external_command::external_command(const std::string &command_string)
	: command(),
	m_command_string(command_string)
{
}

void external_command::run() const
{
	system(m_command_string.c_str());
}

std::string external_command::print() const
{
	return m_command_string;
}

} // namespace dtcue
