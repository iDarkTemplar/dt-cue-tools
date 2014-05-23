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

#include <list>
#include <stdexcept>
#include <sstream>

#include <stdio.h>
#include <string.h>

struct track_data
{
	std::string filename;
	std::map<std::string, std::string> tags;

	unsigned int index;

	boost::optional<dtcue::time_point> start_time;
	boost::optional<dtcue::time_point> end_time;
};

enum class gap_action_type
{
	discard,
	prepend,
	append,
	append_prepend_first
};

std::list<track_data> convert_cue_to_tracks(const dtcue::cue &cue, gap_action_type gap_action)
{
	std::list<track_data> result;
	std::map<std::string, std::string> global_tags;

	global_tags = cue.tags;

	// NOTE: initial TITLE is transformed into ALBUM, TITLE for track is left as it is
	{
		auto tag_title = global_tags.find("TITLE");
		auto tag_album = global_tags.find("ALBUM");

		if ((tag_title != global_tags.end()) && (tag_album == global_tags.end()))
		{
			global_tags["ALBUM"] = tag_title->second;
			global_tags.erase(tag_title);
		}
	}

	for (auto file = cue.files.begin(); file != cue.files.end(); ++file)
	{
		auto track = file->tracks.begin();
		auto next_track = track;

		while (track != file->tracks.end())
		{
			++next_track;
			auto index0 = track->second.indices.find(0);
			auto index1 = track->second.indices.find(1);

			if (index1 == track->second.indices.end())
			{
				std::stringstream err;
				err << "Track with index " << track->first << " doesn't have index 01";
				throw std::runtime_error(err.str());
			}

			track_data data;
			data.filename = file->filename;
			data.index = track->first;

			// NOTE: track-specific tags are more important compared to generic tags
			data.tags = track->second.tags;
			data.tags.insert(global_tags.begin(), global_tags.end());

			if (data.tags.find("TRACKNUMBER") == data.tags.end())
			{
				std::stringstream tracknumber;
				tracknumber << data.index;
				data.tags["TRACKNUMBER"] = tracknumber.str();
			}

			switch (gap_action)
			{
			case gap_action_type::prepend:
				if (index0 != track->second.indices.end())
				{
					data.start_time = index0->second;
				}
				else
				{
					data.start_time = index1->second;
				}
				break;

			case gap_action_type::append_prepend_first:
				if ((data.index == 1) && (index0 != track->second.indices.end()))
				{
					data.start_time = index0->second;
				}
				else
				{
					data.start_time = index1->second;
				}
				break;

			default:
				data.start_time = index1->second;
				break;
			}

			if (next_track != file->tracks.end())
			{
				auto index0_next = next_track->second.indices.find(0);
				auto index1_next = next_track->second.indices.find(1);

				if (index1_next == next_track->second.indices.end())
				{
					std::stringstream err;
					err << "Track with index " << next_track->first << " doesn't have index 01";
					throw std::runtime_error(err.str());
				}

				switch (gap_action)
				{
				case gap_action_type::append_prepend_first:
				case gap_action_type::append:
					data.end_time = index1_next->second;
					break;

				default:
					if (index0_next != next_track->second.indices.end())
					{
						data.end_time = index0_next->second;
					}
					else
					{
						data.end_time = index1_next->second;
					}
					break;
				}
			}

			result.push_back(data);
			track = next_track;
		}
	}

	return result;
}

void print_usage(const char *name)
{
	fprintf(stderr, "USAGE: %s [-v|--verbose] [-n|--dry-run] [--gap-discard|--gap-prepend|--gap-append|--gap-append2] cuesheet\n", name);
}

int main(int argc, char **argv)
{
	bool verbose = false;
	bool dry_run = false;
	gap_action_type gap_action = gap_action_type::discard;
	char *filename = NULL;

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
		else if (strcmp(argv[i], "--gap-append2") == 0)
		{
			gap_action = gap_action_type::append_prepend_first;
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

	if (verbose)
	{
		printf("\nGlobal tags:\n");

		for (auto tag = cuesheet.tags.begin(); tag != cuesheet.tags.end(); ++tag)
		{
			printf("\t%s=%s\n", tag->first.c_str(), tag->second.c_str());
		}

		printf("Files:\n");

		for (auto file = cuesheet.files.begin(); file != cuesheet.files.end(); ++file)
		{
			printf("\tFile: %s\n", file->filename.c_str());

			for (auto track = file->tracks.begin(); track != file->tracks.end(); ++track)
			{
				printf("\t\tTrack %d\n", track->first);

				for (auto index = track->second.indices.begin(); index != track->second.indices.end(); ++index)
				{
					printf("\t\t\tINDEX %02d: %s:%s:%s\n", index->first, index->second.minutes.c_str(), index->second.seconds.c_str(), index->second.chunks_of_seconds.c_str());
				}

				for (auto tag = track->second.tags.begin(); tag != track->second.tags.end(); ++tag)
				{
					printf("\t\t\t%s=%s\n", tag->first.c_str(), tag->second.c_str());
				}
			}
		}

		printf("\n");
	}

	std::list<track_data> tracks = convert_cue_to_tracks(cuesheet, gap_action);

	if (verbose)
	{
		for (auto track = tracks.begin(); track != tracks.end(); ++track)
		{
			printf("Track %d\n", track->index);
			printf("Filename: %s\n", track->filename.c_str());

			if (track->start_time)
			{
				printf("Start: %s:%s:%s\n", track->start_time->minutes.c_str(), track->start_time->seconds.c_str(), track->start_time->chunks_of_seconds.c_str());
			}
			else
			{
				printf("Start: NONE\n");
			}

			if (track->end_time)
			{
				printf("End:   %s:%s:%s\n", track->end_time->minutes.c_str(), track->end_time->seconds.c_str(), track->end_time->chunks_of_seconds.c_str());
			}
			else
			{
				printf("End:   NONE\n");
			}

			for (auto tag = track->tags.begin(); tag != track->tags.end(); ++tag)
			{
				printf("%s=%s\n", tag->first.c_str(), tag->second.c_str());
			}
		}
	}

	return 0;
}
