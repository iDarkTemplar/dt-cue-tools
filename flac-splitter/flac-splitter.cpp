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
		printf("%s=%s\n", tag->first.c_str(), tag->second.c_str());
	}

	printf("Tracks:\n");

	for (auto track = cuesheet.tracks.begin(); track != cuesheet.tracks.end(); ++track)
	{
		printf("Next track\n");

		for (auto tag = track->tags.begin(); tag != track->tags.end(); ++tag)
		{
			printf("%s=%s\n", tag->first.c_str(), tag->second.c_str());
		}
	}

	return 0;
}
