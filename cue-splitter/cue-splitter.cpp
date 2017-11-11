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
#include <set>
#include <stdexcept>
#include <sstream>
#include <memory>
#include <string>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if USE_BOOST

#include <boost/regex.hpp>
#include <boost/optional.hpp>

template <typename T>
using optional = boost::optional<T>;

using regex = boost::regex;
using smatch = boost::smatch;

inline bool regex_match(const std::string &str, boost::smatch &match_results, const boost::regex &regex_string)
{
	return boost::regex_match(str, match_results, regex_string);
}

#else /* USE_BOOST */

#include <regex>
#include <experimental/optional>

template <typename T>
using optional = std::experimental::optional<T>;

using regex = std::regex;
using smatch = std::smatch;

inline bool regex_match(const std::string &str, std::smatch &match_results, const std::regex &regex_string)
{
	return std::regex_match(str, match_results, regex_string);
}

#endif /* USE_BOOST */

struct track_data
{
	std::string filename;
	std::map<std::string, std::string> tags;

	unsigned int index;

	optional<dtcue::time_point> start_time;
	optional<dtcue::time_point> end_time;
};

enum class gap_action_type
{
	discard,
	prepend,
	append,
	append_prepend_first
};

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

std::list<track_data> convert_cue_to_tracks(const dtcue::cue &cue, gap_action_type gap_action)
{
	std::list<track_data> result;
	std::map<std::string, std::string> global_tags;

	global_tags = cue.tags;

	// NOTE: initial TITLE is transformed into ALBUM, TITLE for track is left as it is
	rename_tag(global_tags, "TITLE", "ALBUM");

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

			// NOTE: rename PERFORMER tag into ARTIST
			rename_tag(data.tags, "PERFORMER", "ARTIST");

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
	fprintf(stderr, "USAGE: %s [-v|--verbose] [-n|--dry-run] [--gap-discard|--gap-prepend|--gap-append|--gap-append2] [-s<c>] cuesheet\n", name);
}

int main(int argc, char **argv)
{
	bool verbose = false;
	bool dry_run = false;
	gap_action_type gap_action = gap_action_type::discard;
	char *filename = NULL;
	optional<char> separator;

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
			else if (strcmp(argv[i], "--gap-append2") == 0)
			{
				gap_action = gap_action_type::append_prepend_first;
			}
			else if ((strncmp(argv[i], "-s", 2) == 0) && (strlen(argv[i]) == 3))
			{
				separator = argv[i][2];
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

		if (!separator)
		{
			std::unique_ptr<FILE, int (*)(FILE*)> version_file(popen("flac --version", "r"), &fclose);
			if (!version_file)
			{
				fprintf(stderr, "Can't open piped child process\n");
				return -1;
			}

			char *buffer = NULL;
			size_t bufsize = 0;
			ssize_t result;

			result = getline(&buffer, &bufsize, version_file.get());
			std::unique_ptr<char, void (*)(void*)> managed_buffer(buffer, &free);

			if ((result < 0) || (buffer == NULL))
			{
				fprintf(stderr, "Error in function getline\n");
				return -1;
			}

			std::string bufferstr = buffer;

			regex regex_flac_version("[.\\n]*flac ([[:digit:]]+)\\.([[:digit:]]+)\\.[[:digit:]]+[.\\n]*");
			smatch regex_results;

			if (!regex_match(bufferstr, regex_results, regex_flac_version))
			{
				fprintf(stderr, "Line is: %s\n", buffer);
				fprintf(stderr, "Couldn't query version of flac tool. Please manually specify separator with option -s<c>\n");
				return -1;
			}

			unsigned int a, b;
			a = std::stoul(regex_results[1].str());
			b = std::stoul(regex_results[2].str());

			// check if flac is 1.3.0 or greater
			if ((a > 1)
				|| ((a == 1)
					&& (b >= 3)))
			{
				separator = ',';
			}
			else
			{
				separator = '.';
			}
		}

		std::list<std::list<std::string> > commands_list;
		std::set<std::string> init_commands, deinit_commands;

		for (auto track = tracks.begin(); track != tracks.end(); ++track)
		{
			std::list<std::string> commands;

			{
				std::stringstream cmdstream;

				if (track->filename.rfind(".flac") == track->filename.length() - strlen(".flac"))
				{
					cmdstream << "flac -d -F";

					if (track->start_time)
					{
						cmdstream << " --skip=" << track->start_time->minutes << ':' << track->start_time->seconds << *separator << track->start_time->chunks_of_seconds;
					}

					if (track->end_time)
					{
						cmdstream << " --until=" << track->end_time->minutes << ':' << track->end_time->seconds << *separator << track->end_time->chunks_of_seconds;
					}

					cmdstream << " -o \"_track_" << track->index << ".wav\" \"" << track->filename << "\"";
				}
				else if (track->filename.rfind(".wv") == track->filename.length() - strlen(".wv"))
				{
					cmdstream << "wvunpack";

					if (track->start_time)
					{
						unsigned int minutes = std::stoul(track->start_time->minutes);
						cmdstream << " --skip=" << minutes / 60 << ":" << minutes % 60 << ':' << track->start_time->seconds << "." << track->start_time->chunks_of_seconds;
					}

					if (track->end_time)
					{
						unsigned int minutes = std::stoul(track->end_time->minutes);
						cmdstream << " --until=" << minutes / 60 << ":" << minutes % 60 << ':' << track->end_time->seconds << "." << track->end_time->chunks_of_seconds;
					}

					cmdstream << " -o \"_track_" << track->index << ".wav\" \"" << track->filename << "\"";
				}
				else if ((track->filename.rfind(".ape") == track->filename.length() - strlen(".ape"))
					|| (track->filename.rfind(".wav") == track->filename.length() - strlen(".wav")))
				{
					std::string track_filename;

					if (track->filename.rfind(".ape") == track->filename.length() - strlen(".ape"))
					{
						track_filename = track->filename;
						track_filename.replace(track_filename.rfind(".ape"), std::string::npos, ".wav");

						cmdstream << "mac \"" << track->filename << "\" \"" << track_filename << "\" -d";

						init_commands.insert(cmdstream.str());

						cmdstream.str(std::string());

						cmdstream << "rm \"" << track_filename << "\"";

						deinit_commands.insert(cmdstream.str());

						cmdstream.str(std::string());
					}
					else
					{
						track_filename = track->filename;
					}

					cmdstream << "ffmpeg -i \"" << track_filename << "\"";

					if (track->start_time)
					{
						unsigned int minutes = std::stoul(track->start_time->minutes);
						cmdstream << " -ss " << minutes / 60 << ":" << minutes % 60 << ':' << track->start_time->seconds << "." << track->start_time->chunks_of_seconds;
					}

					if (track->end_time)
					{
						unsigned int minutes = std::stoul(track->end_time->minutes);
						cmdstream << " -to " << minutes / 60 << ":" << minutes % 60 << ':' << track->end_time->seconds << "." << track->end_time->chunks_of_seconds;
					}

					cmdstream << " -acodec copy \"_track_" << track->index << ".wav\"";
				}
				else
				{
					continue;
				}

				commands.push_back(cmdstream.str());

				cmdstream.str(std::string());

				cmdstream << "flac -8 -F --no-lax \"_track_" << track->index << ".wav\"";

				commands.push_back(cmdstream.str());

				cmdstream.str(std::string());

				cmdstream << "rm \"_track_" << track->index << ".wav\"";

				commands.push_back(cmdstream.str());

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
						cmdstream << " --set-tag=\"" << tag->first << "=" << tag->second << "\"";
					}
				}

				for (auto tag = track->tags.begin(); tag != track->tags.end(); ++tag)
				{
					if (std::find(preferred_tags.begin(), preferred_tags.end(), tag->first) == preferred_tags.end())
					{
						cmdstream << " --set-tag=\"" << tag->first << "=" << tag->second << "\"";
					}
				}

				cmdstream << " \"_track_" << track->index << ".flac\"";

				commands.push_back(cmdstream.str());

				auto tag = track->tags.find("TITLE");
				if (tag != track->tags.end())
				{
					cmdstream.str(std::string());
					cmdstream << "mv \"_track_" << track->index << ".flac\" \"" << track->index << " - " << tag->second << ".flac\"";
					commands.push_back(cmdstream.str());
				}
			}

			commands_list.push_back(commands);
		}

		for (auto command = init_commands.begin(); command != init_commands.end(); ++command)
		{
			if (verbose)
			{
				printf("%s\n", command->c_str());
			}

			if (!dry_run)
			{
				system(command->c_str());
			}
		}

		for (auto command_list_item = commands_list.begin(); command_list_item != commands_list.end(); ++command_list_item)
		{
			for (auto command = command_list_item->begin(); command != command_list_item->end(); ++command)
			{
				if (verbose)
				{
					printf("%s\n", command->c_str());
				}

				if (!dry_run)
				{
					system(command->c_str());
				}
			}
		}

		for (auto command = deinit_commands.begin(); command != deinit_commands.end(); ++command)
		{
			if (verbose)
			{
				printf("%s\n", command->c_str());
			}

			if (!dry_run)
			{
				system(command->c_str());
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
