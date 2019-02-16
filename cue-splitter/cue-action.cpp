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

#include "cue-action.hpp"

#include <typeinfo>

#include <stdlib.h>

namespace dtcue {

bool command_comparator::operator() (const std::shared_ptr<command> &x, const std::shared_ptr<command> &y) const
{
	if ((!x) || (!y))
	{
		// x may be less than y only if y is present
		return static_cast<bool>(y);
	}

	const std::type_info &x_type = typeid(*x);
	const std::type_info &y_type = typeid(*y);

	if (x_type != y_type)
	{
		return (x_type.before(y_type));
	}

	return x->compare(*y);
}

external_command::external_command(const std::string &command_string)
	: command(),
	m_command_string(command_string)
{
}

bool external_command::run() const
{
	system(m_command_string.c_str());

	return true;
}

std::string external_command::print() const
{
	return m_command_string;
}

bool external_command::compare(const command &other) const
{
	const external_command &other_cmd = dynamic_cast<const external_command&>(other);

	return (m_command_string < other_cmd.m_command_string);
}

} // namespace dtcue
