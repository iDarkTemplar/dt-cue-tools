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

#ifndef DT_CUE_ACTION_HPP
#define DT_CUE_ACTION_HPP

#include <string>
#include <memory>

#include <dt-cue-library.hpp>

#include "cue-types.hpp"

namespace dtcue {

struct command_comparator;

class command
{
public:
	virtual ~command() = default;

	virtual bool run() const = 0;
	virtual std::string print() const = 0;

protected:
	// compare with instance of same class only
	virtual bool compare(const command &other) const = 0;

	friend class command_comparator;
};

struct command_comparator: public std::binary_function<std::shared_ptr<command>, std::shared_ptr<command>, bool>
{
	bool operator() (const std::shared_ptr<command> &x, const std::shared_ptr<command> &y) const;
};

class external_command: public command
{
public:
	explicit external_command(const std::string &command_string);

	virtual bool run() const;
	virtual std::string print() const;

protected:
	virtual bool compare(const command &other) const;

private:
	std::string m_command_string;
};

} // namespace dtcue

#endif /* DT_CUE_ACTION_HPP */
