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

#include <list>
#include <vector>
#include <set>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <memory>
#include <string>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "cue-types.hpp"
#include "cue-action.hpp"

#if USE_BOOST

static inline bool regex_match(const std::string &str, boost::smatch &match_results, const boost::regex &regex_string)
{
	return boost::regex_match(str, match_results, regex_string);
}

#else /* USE_BOOST */

inline bool regex_match(const std::string &str, std::smatch &match_results, const std::regex &regex_string)
{
	return std::regex_match(str, match_results, regex_string);
}

#endif /* USE_BOOST */

struct track_part
{
	std::string filename;

	optional<dtcue::time_point> start_time;
	optional<dtcue::time_point> end_time;
};

struct track_data
{
	std::map<std::string, std::string> tags;

	unsigned int index;

	std::vector<track_part> parts;
};

enum class gap_action_type
{
	discard,
	prepend,
	append,
	prepend_first_then_append
};

static std::set<std::string> skipped_tags = {
	"CDTEXTFILE",
	"FLAGS",
	"PREGAP",
	"POSTGAP"
};

std::string escape_single_quote(const std::string &input)
{
	std::string result = input;

	size_t idx = result.rfind('\'');

	while (idx != std::string::npos)
	{
		result.replace(idx, 1, "\'\\\'\'");

		if (idx == 0)
		{
			break;
		}

		idx = result.rfind('\'', idx - 1);
	}

	return result;
}

void rename_tag(std::map<std::string, std::string> &tags, const std::string &oldname, const std::string &newname)
{
	auto tag_old = tags.find(oldname);
	auto tag_new = tags.find(newname);

	if ((tag_old != tags.end()) && (tag_new == tags.end()))
	{
		tags[newname] = tag_old->second;
		tags.erase(tag_old);
	}
}

bool is_timepoint_zero(const dtcue::time_point &timepoint)
{
	return ((std::stoul(timepoint.minutes) == 0) && (std::stoul(timepoint.seconds) == 0) && (std::stoul(timepoint.frames) == 0));
}

std::list<track_data> convert_cue_to_tracks(const dtcue::cue &cue, gap_action_type gap_action)
{
	std::list<track_data> result;
	std::map<std::string, std::string> global_tags;

	global_tags = cue.tags;

	// NOTE: initial TITLE is transformed into ALBUM, TITLE for track is left as it is
	rename_tag(global_tags, "TITLE", "ALBUM");

	auto track = cue.tracks.begin();
	auto next_track = track;

	while (track != cue.tracks.end())
	{
		++next_track;

		auto index0 = track->indices.find(0);
		auto index1 = track->indices.find(1);

		optional<std::map<unsigned int, dtcue::file_time_point>::const_iterator> index0_next;
		optional<std::map<unsigned int, dtcue::file_time_point>::const_iterator> index1_next;

		if (index1 == track->indices.end())
		{
			std::stringstream err;
			err << "Track with index " << track->track_index << " doesn't have index 01";
			throw std::runtime_error(err.str());
		}

		if (next_track != cue.tracks.end())
		{
			index0_next = next_track->indices.find(0);
			index1_next = next_track->indices.find(1);

			if (index1_next == next_track->indices.end())
			{
				std::stringstream err;
				err << "Track with index " << next_track->track_index << " doesn't have index 01";
				throw std::runtime_error(err.str());
			}
		}

		track_data data;
		data.index = track->track_index;

		// NOTE: track-specific tags are more important compared to generic tags
		data.tags = track->tags;
		data.tags.insert(global_tags.begin(), global_tags.end());

		if (data.tags.find("TRACKNUMBER") == data.tags.end())
		{
			std::stringstream tracknumber;
			tracknumber << data.index;
			data.tags["TRACKNUMBER"] = tracknumber.str();
		}

		// NOTE: rename PERFORMER tag into ARTIST
		rename_tag(data.tags, "PERFORMER", "ARTIST");

		for (const auto &skipped_tag: skipped_tags)
		{
			data.tags.erase(skipped_tag);
		}

		size_t index = index1->second.file_index;
		dtcue::time_point initial_timepoint = index1->second.time;

		switch (gap_action)
		{
		case gap_action_type::discard:
		case gap_action_type::append:
			break;

		case gap_action_type::prepend:
			if (index0 != track->indices.end())
			{
				index = index0->second.file_index;
				initial_timepoint = index0->second.time;
			}
			break;

		case gap_action_type::prepend_first_then_append:
			if ((index0 != track->indices.end()) && (track == cue.tracks.begin()))
			{
				index = index0->second.file_index;
				initial_timepoint = index0->second.time;
			}
			break;
		}

		{
			track_part part;

			part.filename = track->files[index];
			part.start_time = initial_timepoint;

			data.parts.push_back(part);
		}

		for (++index ; index < track->files.size(); ++index)
		{
			track_part part;

			part.filename = track->files[index];

			data.parts.push_back(part);
		}

		if (next_track != cue.tracks.end())
		{
			std::string filename;
			dtcue::time_point timepoint;

			switch (gap_action)
			{
			case gap_action_type::discard:
			case gap_action_type::prepend:
				if (index0_next && ((*index0_next) != next_track->indices.end()))
				{
					filename = next_track->files[(*index0_next)->second.file_index];
					timepoint = (*index0_next)->second.time;
				}
				else
				{
					filename = next_track->files[(*index1_next)->second.file_index];
					timepoint = (*index1_next)->second.time;
				}
				break;

			case gap_action_type::append:
			case gap_action_type::prepend_first_then_append:
				if ((*index1_next)->second.file_index != 0)
				{
					index = 0;
					size_t last_index = (*index1_next)->second.file_index;

					if (data.parts.back().filename == next_track->files[0])
					{
						index = 1;
					}

					for ( ; index <= last_index; ++index)
					{
						track_part part;

						part.filename = next_track->files[index];

						data.parts.push_back(part);
					}
				}

				filename = next_track->files[(*index1_next)->second.file_index];
				timepoint = (*index1_next)->second.time;
				break;
			}

			if (is_timepoint_zero(timepoint))
			{
				if (data.parts.back().filename != filename)
				{
					std::stringstream err;
					err << "Track with index " << next_track->track_index << " has invalid starting timestamp";
					throw std::runtime_error(err.str());
				}

				data.parts.pop_back();
			}
			else
			{
				if (data.parts.back().filename != filename)
				{
					std::stringstream err;
					err << "Track with index " << next_track->track_index << " doesn't continue at same file as previous track";
					throw std::runtime_error(err.str());
				}

				data.parts.back().end_time = timepoint;
			}
		}

		result.push_back(data);
		track = next_track;
	}

	return result;
}

std::string convert_frames_to_time(const std::string &frames)
{
	if ((frames.length() == 0) || (frames.length() > 2))
	{
		return std::string();
	}

	unsigned long value = std::stoul(frames);
	if (value >= 75)
	{
		return std::string();
	}

	value = (value * 1000000) / 75;

	std::stringstream stringvalue;
	stringvalue << std::setfill('0') << std::setw(6) << value;
	return stringvalue.str();
}

bool convert_and_save_frames(std::map<std::string, std::string> &map, const std::string &frames)
{
	if (map.find(frames) == map.end())
	{
		std::string value = convert_frames_to_time(frames);
		if (value.empty())
		{
			return false;
		}

		map[frames] = value;
	}

	return true;
}

void print_usage(const char *name)
{
	fprintf(stderr, "USAGE: %s [-v|--verbose] [-n|--dry-run] [--gap-discard|--gap-prepend|--gap-append|--gap-prepend-first-then-append] cuesheet\n", name);
}

int main(int argc, char **argv)
{
	bool verbose = false;
	bool dry_run = false;
	gap_action_type gap_action = gap_action_type::discard;
	char *filename = NULL;

	std::map<std::string, std::string> frames_to_seconds_map;

	try
	{
		for (int i = 1; i < argc; ++i)
		{
			if ((strcmp(argv[i], "-v") == 0)
				|| (strcmp(argv[i], "--verbose") == 0))
			{
				verbose = true;
			}
			else if ((strcmp(argv[i], "-n") == 0)
				|| (strcmp(argv[i], "--dry-run") == 0))
			{
				dry_run = true;
			}
			else if (strcmp(argv[i], "--gap-discard") == 0)
			{
				gap_action = gap_action_type::discard;
			}
			else if (strcmp(argv[i], "--gap-prepend") == 0)
			{
				gap_action = gap_action_type::prepend;
			}
			else if (strcmp(argv[i], "--gap-append") == 0)
			{
				gap_action = gap_action_type::append;
			}
			else if (strcmp(argv[i], "--gap-prepend-first-then-append") == 0)
			{
				gap_action = gap_action_type::prepend_first_then_append;
			}
			else if (filename == NULL)
			{
				filename = argv[i];
			}
			else
			{
				print_usage(argv[0]);
				return -1;
			}
		}

		if (filename == NULL)
		{
			print_usage(argv[0]);
			return -1;
		}

		dtcue::cue cuesheet = dtcue::parse_cue_file(filename);

		// convert frames to time, from 1/75 to 1/1000000, and save these values to map
		{
			for (auto track = cuesheet.tracks.begin(); track != cuesheet.tracks.end(); ++track)
			{
				for (auto index = track->indices.begin(); index != track->indices.end(); ++index)
				{
					if (!convert_and_save_frames(frames_to_seconds_map, index->second.time.frames))
					{
						fprintf(stderr, "Failed to convert frames for track %u, index %u: value is %s\n", track->track_index, index->first, index->second.time.frames.c_str());
						return -1;
					}
				}
			}
		}

		if (verbose)
		{
			printf("\nGlobal tags:\n");

			for (auto tag = cuesheet.tags.begin(); tag != cuesheet.tags.end(); ++tag)
			{
				printf("\t%s=%s\n", tag->first.c_str(), tag->second.c_str());
			}

			printf("Tracks:\n");

			for (auto track = cuesheet.tracks.begin(); track != cuesheet.tracks.end(); ++track)
			{
				printf("\tTrack %u\n", track->track_index);

				for (size_t file_idx = 0; file_idx < track->files.size(); ++file_idx)
				{
					printf("\t\tFile %zu: %s\n", file_idx, track->files[file_idx].c_str());
				}

				for (auto index = track->indices.begin(); index != track->indices.end(); ++index)
				{
					printf("\t\tINDEX %02d: file %zu, %s:%s.%s\n", index->first, index->second.file_index, index->second.time.minutes.c_str(), index->second.time.seconds.c_str(), frames_to_seconds_map[index->second.time.frames].c_str());
				}

				for (auto tag = track->tags.begin(); tag != track->tags.end(); ++tag)
				{
					printf("\t\t%s=%s\n", tag->first.c_str(), tag->second.c_str());
				}
			}

			printf("\n");
		}

		std::list<track_data> tracks = convert_cue_to_tracks(cuesheet, gap_action);

		// convert frames to time, from 1/75 to 1/1000000, and save these values to map
		{
			for (auto track = tracks.begin(); track != tracks.end(); ++track)
			{
				for (auto part = track->parts.begin(); part != track->parts.end(); ++part)
				{
					if (part->start_time)
					{
						if (!convert_and_save_frames(frames_to_seconds_map, part->start_time->frames))
						{
							fprintf(stderr, "Failed to convert frames for track %u, file %s, start time: value is %s\n", track->index, part->filename.c_str(), part->start_time->frames.c_str());
							return -1;
						}
					}

					if (part->end_time)
					{
						if (!convert_and_save_frames(frames_to_seconds_map, part->end_time->frames))
						{
							fprintf(stderr, "Failed to convert frames for track %u, file %s, end time: value is %s\n", track->index, part->filename.c_str(), part->end_time->frames.c_str());
							return -1;
						}
					}
				}
			}
		}

		if (verbose)
		{
			for (auto track = tracks.begin(); track != tracks.end(); ++track)
			{
				printf("Track %d\n", track->index);

				for (auto part = track->parts.begin(); part != track->parts.end(); ++part)
				{
					printf("Filename: %s\n", part->filename.c_str());

					if (part->start_time)
					{
						printf("Start: %s:%s.%s\n", part->start_time->minutes.c_str(), part->start_time->seconds.c_str(), frames_to_seconds_map[part->start_time->frames].c_str());
					}
					else
					{
						printf("Start: NONE\n");
					}

					if (part->end_time)
					{
						printf("End:   %s:%s.%s\n", part->end_time->minutes.c_str(), part->end_time->seconds.c_str(), frames_to_seconds_map[part->end_time->frames].c_str());
					}
					else
					{
						printf("End:   NONE\n");
					}
				}

				for (auto tag = track->tags.begin(); tag != track->tags.end(); ++tag)
				{
					printf("%s=%s\n", tag->first.c_str(), tag->second.c_str());
				}

				printf("\n");
			}
		}

		std::list<std::list<std::shared_ptr<dtcue::command> > > commands_list;
		std::set<std::shared_ptr<dtcue::command>, dtcue::command_comparator> init_commands, deinit_commands;

		for (auto track = tracks.begin(); track != tracks.end(); ++track)
		{
			std::list<std::shared_ptr<dtcue::command> > commands;

			{
				std::stringstream cmdstream;

				// TODO: support concatenating tracks from parts of multiple files
				if (track->parts.size() != 1)
				{
					std::stringstream err;
					err << "Track with index " << track->index << " consists of more than 1 file. This is currently not supported.";
					throw std::runtime_error(err.str());
				}

				if (track->parts.front().filename.rfind(".flac") == track->parts.front().filename.length() - strlen(".flac"))
				{
					// use "C" locale in order to always use '.' as separator
					cmdstream << "LC_ALL=C ";
					cmdstream << "flac -d -F";

					if (track->parts.front().start_time)
					{
						cmdstream << " --skip=" << track->parts.front().start_time->minutes << ':' << track->parts.front().start_time->seconds << '.' << frames_to_seconds_map[track->parts.front().start_time->frames];
					}

					if (track->parts.front().end_time)
					{
						cmdstream << " --until=" << track->parts.front().end_time->minutes << ':' << track->parts.front().end_time->seconds << '.' << frames_to_seconds_map[track->parts.front().end_time->frames];
					}

					cmdstream << " -o \'_track_" << track->index << ".wav\' \'" << escape_single_quote(track->parts.front().filename) << "\'";
				}
				else if (track->parts.front().filename.rfind(".wv") == track->parts.front().filename.length() - strlen(".wv"))
				{
					cmdstream << "wvunpack";

					if (track->parts.front().start_time)
					{
						unsigned int minutes = std::stoul(track->parts.front().start_time->minutes);
						cmdstream << " --skip=" << minutes / 60 << ":" << minutes % 60 << ':' << track->parts.front().start_time->seconds << "." << frames_to_seconds_map[track->parts.front().start_time->frames];
					}

					if (track->parts.front().end_time)
					{
						unsigned int minutes = std::stoul(track->parts.front().end_time->minutes);
						cmdstream << " --until=" << minutes / 60 << ":" << minutes % 60 << ':' << track->parts.front().end_time->seconds << "." << frames_to_seconds_map[track->parts.front().end_time->frames];
					}

					cmdstream << " -o \'_track_" << track->index << ".wav\' \'" << escape_single_quote(track->parts.front().filename) << "\'";
				}
				else if ((track->parts.front().filename.rfind(".ape") == track->parts.front().filename.length() - strlen(".ape"))
					|| (track->parts.front().filename.rfind(".m4a") == track->parts.front().filename.length() - strlen(".m4a"))
					|| (track->parts.front().filename.rfind(".wav") == track->parts.front().filename.length() - strlen(".wav")))
				{
					std::string track_filename;

					if (track->parts.front().filename.rfind(".ape") == track->parts.front().filename.length() - strlen(".ape"))
					{
						track_filename = track->parts.front().filename;
						track_filename.replace(track_filename.rfind(".ape"), std::string::npos, ".wav");

						cmdstream << "mac \'" << escape_single_quote(track->parts.front().filename) << "\' \'" << escape_single_quote(track_filename) << "\' -d";

						init_commands.insert(std::make_shared<dtcue::external_command>(cmdstream.str()));

						cmdstream.str(std::string());

						cmdstream << "rm \'" << escape_single_quote(track_filename) << "\'";

						deinit_commands.insert(std::make_shared<dtcue::external_command>(cmdstream.str()));

						cmdstream.str(std::string());
					}
					else if (track->parts.front().filename.rfind(".m4a") == track->parts.front().filename.length() - strlen(".m4a"))
					{
						track_filename = track->parts.front().filename;
						track_filename.replace(track_filename.rfind(".m4a"), std::string::npos, ".wav");

						cmdstream << "alac -f \'" << escape_single_quote(track_filename) << "\' \'" << escape_single_quote(track->parts.front().filename) << "\'";

						init_commands.insert(std::make_shared<dtcue::external_command>(cmdstream.str()));

						cmdstream.str(std::string());

						cmdstream << "rm \'" << escape_single_quote(track_filename) << "\'";

						deinit_commands.insert(std::make_shared<dtcue::external_command>(cmdstream.str()));

						cmdstream.str(std::string());
					}
					else
					{
						track_filename = track->parts.front().filename;
					}

					cmdstream << "ffmpeg -i \'" << escape_single_quote(track_filename) << "\'";

					if (track->parts.front().start_time)
					{
						unsigned int minutes = std::stoul(track->parts.front().start_time->minutes);
						cmdstream << " -ss " << minutes / 60 << ":" << minutes % 60 << ':' << track->parts.front().start_time->seconds << "." << frames_to_seconds_map[track->parts.front().start_time->frames];
					}

					if (track->parts.front().end_time)
					{
						unsigned int minutes = std::stoul(track->parts.front().end_time->minutes);
						cmdstream << " -to " << minutes / 60 << ":" << minutes % 60 << ':' << track->parts.front().end_time->seconds << "." << frames_to_seconds_map[track->parts.front().end_time->frames];
					}

					cmdstream << " -acodec copy \'_track_" << track->index << ".wav\'";
				}
				else
				{
					fprintf(stderr, "Unsupported file type found, filename: %s\n", track->parts.front().filename.c_str());
					continue;
				}

				commands.push_back(std::make_shared<dtcue::external_command>(cmdstream.str()));

				cmdstream.str(std::string());

				cmdstream << "flac -8 -F --no-lax \'_track_" << track->index << ".wav\'";

				commands.push_back(std::make_shared<dtcue::external_command>(cmdstream.str()));

				cmdstream.str(std::string());

				cmdstream << "rm \'_track_" << track->index << ".wav\'";

				commands.push_back(std::make_shared<dtcue::external_command>(cmdstream.str()));

				cmdstream.str(std::string());

				cmdstream << "metaflac";

				// first set ALBUM, TITLE, ARTIST and TRACKNUMBER, after that set everything else
				std::list<std::string> preferred_tags;
				preferred_tags.push_back("ALBUM");
				preferred_tags.push_back("TITLE");
				preferred_tags.push_back("ARTIST");
				preferred_tags.push_back("TRACKNUMBER");

				for (auto searched = preferred_tags.begin(); searched != preferred_tags.end(); ++searched)
				{
					auto tag = track->tags.find(*searched);
					if (tag != track->tags.end())
					{
						cmdstream << " --set-tag=\'" << escape_single_quote(tag->first) << "=" << escape_single_quote(tag->second) << "\'";
					}
				}

				for (auto tag = track->tags.begin(); tag != track->tags.end(); ++tag)
				{
					if (std::find(preferred_tags.begin(), preferred_tags.end(), tag->first) == preferred_tags.end())
					{
						cmdstream << " --set-tag=\'" << escape_single_quote(tag->first) << "=" << escape_single_quote(tag->second) << "\'";
					}
				}

				cmdstream << " \'_track_" << track->index << ".flac\'";

				commands.push_back(std::make_shared<dtcue::external_command>(cmdstream.str()));

				auto tag = track->tags.find("TITLE");
				if (tag != track->tags.end())
				{
					cmdstream.str(std::string());
					cmdstream << "mv \'_track_" << track->index << ".flac\' \'" << track->index << " - " << escape_single_quote(tag->second) << ".flac\'";
					commands.push_back(std::make_shared<dtcue::external_command>(cmdstream.str()));
				}
			}

			commands_list.push_back(commands);
		}

		for (auto command = init_commands.begin(); command != init_commands.end(); ++command)
		{
			if (verbose)
			{
				printf("%s\n", (*command)->print().c_str());
			}

			if (!dry_run)
			{
				if (!(*command)->run())
				{
					fprintf(stderr, "Action failed: %s\n", (*command)->print().c_str());
				}
			}
		}

		for (auto command_list_item = commands_list.begin(); command_list_item != commands_list.end(); ++command_list_item)
		{
			for (auto command = command_list_item->begin(); command != command_list_item->end(); ++command)
			{
				if (verbose)
				{
					printf("%s\n", (*command)->print().c_str());
				}

				if (!dry_run)
				{
					if (!(*command)->run())
					{
						fprintf(stderr, "Action failed: %s\n", (*command)->print().c_str());
					}
				}
			}
		}

		for (auto command = deinit_commands.begin(); command != deinit_commands.end(); ++command)
		{
			if (verbose)
			{
				printf("%s\n", (*command)->print().c_str());
			}

			if (!dry_run)
			{
				if (!(*command)->run())
				{
					fprintf(stderr, "Action failed: %s\n", (*command)->print().c_str());
				}
			}
		}
	}
	catch (const std::exception &exc)
	{
		fprintf(stderr, "Caught std::exception: %s\n", exc.what());
		return -1;
	}
	catch (...)
	{
		fprintf(stderr, "Caught unknown exception\n");
		return -1;
	}

	return 0;
}
