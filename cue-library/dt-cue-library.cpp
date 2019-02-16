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

#include <dt-cue-library.hpp>

#include <stdexcept>
#include <fstream>
#include <sstream>
#include <string>

#include <utility>

#include <sys/stat.h>

#ifndef NDEBUG
#include <stdio.h>
#endif /* NDEBUG */

#if USE_BOOST

#include <boost/regex.hpp>

using regex = boost::regex;
using smatch = boost::smatch;

static inline bool regex_match(const std::string &str, boost::smatch &match_results, const boost::regex &regex_string)
{
	return boost::regex_match(str, match_results, regex_string);
}

#else /* USE_BOOST */

#include <regex>

using regex = std::regex;
using smatch = std::smatch;

static inline bool regex_match(const std::string &str, std::smatch &match_results, const std::regex &regex_string)
{
	return std::regex_match(str, match_results, regex_string);
}

#endif /* USE_BOOST */

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

	regex regex_title("^[ ]*TITLE \"([^\"]*)\"[[:cntrl:]]*$");
	regex regex_performer("^[ ]*PERFORMER \"([^\"]*)\"[[:cntrl:]]*$");
	regex regex_file("^[ ]*FILE \"([^\"]*)\" [[:alnum:]]+[[:cntrl:]]*$");
	regex regex_track("^[ ]*TRACK ([[:digit:]]+) ([[:alpha:]]+)[[:cntrl:]]*$");
	regex regex_index("^[ ]*INDEX ([[:digit:]]+) ([[:digit:]]+)[:\\.,]([[:digit:]]+)[:\\.,]([[:digit:]]+)[[:cntrl:]]*$");
	regex regex_comment_quoted("^[ ]*REM ([[:alnum:]]+) \"([^\"]*)\"[[:cntrl:]]*$");
	regex regex_comment_plain("^[ ]*REM ([[:alnum:]]+) ([^[:cntrl:]]+)[[:cntrl:]]*$");
	regex regex_else_quoted("^[ ]*([[:alnum:]]+) \"([^\"]*)\"[[:cntrl:]]*$");
	regex regex_else_plain("^[ ]*([[:alnum:]]+) ([^[:cntrl:]]+)[[:cntrl:]]*$");

	smatch results;

	bool got_track = false;
	bool got_filename = false;
	file obtained_file;
	track obtained_track;
	unsigned int track_index = 0;
	std::map<std::string, std::string> tags;

	while (std::getline(input_file, file_line))
	{
#ifndef NDEBUG
		printf("Line: %s\n", file_line.c_str());
#endif /* NDEBUG */

		if (regex_match(file_line, results, regex_title))
		{
#ifndef NDEBUG
			printf("\tGot title: %s\n", results[1].str().c_str());
#endif /* NDEBUG */

			tags["TITLE"] = results[1].str();
		}
		else if (regex_match(file_line, results, regex_performer))
		{
#ifndef NDEBUG
			printf("\tGot performer: %s\n", results[1].str().c_str());
#endif /* NDEBUG */

			tags["PERFORMER"] = results[1].str();
		}
		else if (regex_match(file_line, results, regex_file))
		{
#ifndef NDEBUG
			printf("\tGot file: %s\n", results[1].str().c_str());
#endif /* NDEBUG */

			if (got_filename)
			{
				if (got_track)
				{
					if (obtained_track.indices.find(1) == obtained_track.indices.end())
					{
						std::stringstream err;
						err << "Track with index " << track_index << " doesn't have index 01";
						throw std::runtime_error(err.str());
					}

					obtained_track.tags = std::move(tags);
					obtained_file.tracks[track_index] = obtained_track;
				}

				result.files.push_back(obtained_file);
				tags.clear();
				obtained_track = track();
				track_index = 0;
				got_track = false;
			}

			got_filename = true;
			obtained_file = file();
			obtained_file.filename = results[1].str();
		}
		else if (regex_match(file_line, results, regex_track))
		{
#ifndef NDEBUG
			printf("\tGot track: %s, type %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */

			if (!got_filename)
			{
				throw std::runtime_error("Got tag TRACK before any tag FILE");
			}

			if (got_track)
			{
				if (obtained_track.indices.find(1) == obtained_track.indices.end())
				{
					std::stringstream err;
					err << "Track with index " << track_index << " doesn't have index 01";
					throw std::runtime_error(err.str());
				}

				obtained_track.tags = std::move(tags);
				obtained_file.tracks[track_index] = obtained_track;
			}
			else
			{
				result.tags.insert(tags.begin(), tags.end());
			}

			tags.clear();

			got_track = true;
			obtained_track = track();
			track_index = std::stoul(results[1].str());

			// NOTE: check type being AUDIO, everything else is ignored
			if (results[2].str() == std::string("AUDIO"))
			{
				obtained_track.type = dtcue::track_type::audio;
			}
			else
			{
				obtained_track.type = dtcue::track_type::unknown;
			}
		}
		else if (regex_match(file_line, results, regex_index))
		{
#ifndef NDEBUG
			printf("\tGot index: %s, value %s:%s:%s\n", results[1].str().c_str(), results[2].str().c_str(), results[3].str().c_str(), results[4].str().c_str());
#endif /* NDEBUG */

			if (!got_track)
			{
				throw std::runtime_error("Got tag INDEX before any tag TRACK");
			}

			time_point index;
			index.minutes = results[2].str();
			index.seconds = results[3].str();
			index.frames = results[4].str();

			// NOTE: check index being 00 or 01, everything else is ignored
			obtained_track.indices[std::stoul(results[1].str())] = index;
		}
		else if (regex_match(file_line, results, regex_comment_quoted))
		{
#ifndef NDEBUG
			printf("\tGot comment quoted:\n\tName: %s\n\tValue: %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */

			tags[results[1].str()] = results[2].str();
		}
		else if (regex_match(file_line, results, regex_comment_plain))
		{
#ifndef NDEBUG
			printf("\tGot comment:\n\tName: %s\n\tValue: %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */

			tags[results[1].str()] = results[2].str();
		}
		else if (regex_match(file_line, results, regex_else_quoted))
		{
#ifndef NDEBUG
			printf("\tGot something else with quotes:\n\tName: %s\n\tValue: %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */

			tags[results[1].str()] = results[2].str();
		}
		else if (regex_match(file_line, results, regex_else_plain))
		{
#ifndef NDEBUG
			printf("\tGot something else:\n\tName: %s\n\tValue: %s\n", results[1].str().c_str(), results[2].str().c_str());
#endif /* NDEBUG */

			tags[results[1].str()] = results[2].str();
		}
		else
		{
			throw std::runtime_error("Unrecognized line: " + file_line);
		}
	}

	if (got_filename)
	{
		if (got_track)
		{
			if (obtained_track.indices.find(1) == obtained_track.indices.end())
			{
				std::stringstream err;
				err << "Track with index " << track_index << " doesn't have index 01";
				throw std::runtime_error(err.str());
			}

			obtained_track.tags = std::move(tags);
			obtained_file.tracks[track_index] = obtained_track;
		}

		result.files.push_back(obtained_file);
	}

	return result;
}

} // namespace dtcue
