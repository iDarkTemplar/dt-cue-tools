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

#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		fprintf(stderr, "USAGE: %s cuesheet\n", argv[0]);
		return -1;
	}

	dtcue::cue cuesheet = dtcue::parse_cue_file(argv[1]);

	// NOTE: initial TITLE is transformed into ALBUM, TITLE for track is left as it is

	printf("Global tags:\n");

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

	return 0;
}
