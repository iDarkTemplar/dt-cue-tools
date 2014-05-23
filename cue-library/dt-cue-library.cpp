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

#include <dt-cue-library.hpp>

#include <stdexcept>
#include <fstream>

#include <sys/stat.h>

#include <boost/regex.hpp>

#ifndef NDEBUG
#include <stdio.h>
#endif /* NDEBUG */

namespace dtcue {

cue parse_cue_file(const std::string &filename)
{
	struct stat statbuf;

	if ((stat(filename.c_str(), &statbuf) == -1)
		|| (!S_ISREG(statbuf.st_mode)))
	{
		throw std::invalid_argument("File '" + filename + "' is not a valid regular file");
	}

	std::ifstream input_file(filename.c_str());
	std::string file_line;

	{ // Make sure BOM mark is ignored
		char a, b, c;
		a = input_file.get();
		b = input_file.get();
		c = input_file.get();

		if ((a != (char)0xEF) || (b != (char)0xBB) || (c != (char)0xBF))
		{
			input_file.seekg(0);
		}
	}

	cue result;

	boost::regex regex_title("^[ ]*TITLE \"([^\"]*)\"[[:cntrl:]]?$");
	boost::regex regex_performer("^[ ]*PERFORMER \"([^\"]*)\"[[:cntrl:]]?$");
	boost::regex regex_file("^[ ]*FILE \"([^\"]*)\" [[:alnum:]]+[[:cntrl:]]?$");
	boost::regex regex_track("^[ ]*TRACK ([[:digit:]]+) ([[:alpha:]]+)[[:cntrl:]]?$");
	boost::regex regex_index("^[ ]*INDEX ([[:digit:]]+) ([[:digit:]]+[:\\.,][[:digit:]]+[:\\.,][[:digit:]]+)[[:cntrl:]]?$");
	boost::regex regex_comment_quoted("^[ ]*REM ([[:alnum:]]+) \"([^\"]*)\"[[:cntrl:]]?$");
	boost::regex regex_comment_plain("^[ ]*REM ([[:alnum:]]+) ([^[:cntrl:]]+)[[:cntrl:]]?$");
	boost::regex regex_else_quoted("^[ ]*([[:alnum:]]+) \"([^\"]*)\"[[:cntrl:]]?$");
	boost::regex regex_else_plain("^[ ]*([[:alnum:]]+) ([^[:cntrl:]]+)[[:cntrl:]]?$");

	boost::smatch results;

	while (std::getline(input_file, file_line))
	{
#ifndef NDEBUG
		printf("Line: %s\n",file_line.c_str());
#endif /* NDEBUG */

		// TODO: process line;
		if (boost::regex_match(file_line, results, regex_title))
		{
#ifndef NDEBUG
			printf("\tGot title: %s\n", results[1].str().c_str());
#endif /* NDEBUG */

			// NOTE: initial TITLE is transformed into ALBUM, TITLE for track is left as it is
		}
		else if (boost::regex_match(file_line, results, regex_performer))
		{
#ifndef NDEBUG
			printf("\tGot performer: %s\n", results[1].str().c_str());
#endif /* NDEBUG */
		}
		else if (boost::regex_match(file_line, results, regex_file))
		{
#ifndef NDEBUG
			printf("\tGot file: %s\n", results[1].str().c_str());
#endif /* NDEBUG */
		}
		else if (boost::regex_match(file_line, results, regex_track))
		{
#ifndef NDEBUG
			printf("\tGot track: %s, type %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */

			// TODO: check type being AUDIO, everything else is ignored
		}
		else if (boost::regex_match(file_line, results, regex_index))
		{
#ifndef NDEBUG
			printf("\tGot index: %s, value %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */

			// TODO: check index being 00 or 01, everything else is ignored
		}
		else if (boost::regex_match(file_line, results, regex_comment_quoted))
		{
#ifndef NDEBUG
			printf("\tGot comment quoted:\n\tName: %s\n\tValue: %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */
		}
		else if (boost::regex_match(file_line, results, regex_comment_plain))
		{
#ifndef NDEBUG
			printf("\tGot comment:\n\tName: %s\n\tValue: %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */
		}
		else if (boost::regex_match(file_line, results, regex_else_quoted))
		{
#ifndef NDEBUG
			printf("\tGot something else with quotes:\n\tName: %s\n\tValue: %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */
		}
		else if (boost::regex_match(file_line, results, regex_else_plain))
		{
#ifndef NDEBUG
			printf("\tGot something else:\n\tName: %s\n\tValue: %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */
		}
	}

	return result;
}

} // namespace dtcue
