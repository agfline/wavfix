/*
 *   Wavfix - repair damaged wav files and preserve metadata.
 *
 *   Copyright (c) 2016, 2017 Adrien Gesta-Fline
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Affero General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Affero General Public License for more details.
 *
 *   You should have received a copy of the GNU Affero General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 *
 *	TODO : Add support for RF64
 */


#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <stdarg.h>		// va_start();
#include <linux/limits.h>       // NAME_MAX

#include "libriff.h"
#include "libwav.h"
#include "libbext.h"
#include "file.h"


struct user_options {
//	uint8_t  frmt;
	uint16_t freq;
	uint16_t bitd;
	uint16_t chan;

	uint8_t  no_repair;
//	char	*out_dir;
	char    *suffix;

	uint8_t  verb;

	uint8_t  force_user_values;
};

struct user_options user_opt;



int rebuild_fmt_chunk ( struct chunk **ckls );
int rebuild_data_chunk( struct chunk **ckls );
int check_data_chunk( struct chunk **ckls, size_t file_sz );
void show_usage( void );
void show_help ( void );
int main( int argc, char *argv[] );

//extern inline uint8_t bext_codh_mode_to_chan( uint8_t mode );


/*
	verbose stuff
*/
static inline int _print_verb( int verb, const char *msg, ... )
	__attribute__((format (printf, 2, 3)));

#define verb( verb, fmt, ... ) _print_verb( verb, fmt, ##__VA_ARGS__ )

static inline int _print_verb( int verb, const char *fmt, ... ) {

	int r = 0;
	va_list arg;

	va_start(arg, fmt);

	if ( user_opt.verb > verb )
		r = vfprintf(stdout, fmt, arg);

	va_end(arg);

	return r;
}



int rebuild_fmt_chunk( struct chunk **ckls ) {

	int pr = 0; // keep track if function prints

	struct fmt *fmt = wav_new_fmt( WAVE_FORMAT_PCM );

	struct chunk *bext_ck = NULL;
	struct bext  *bext    = NULL;
	struct coding_history *codh = NULL;

	bext_ck = get_riff_chunk_by_id( *ckls, "bext" );

	if ( bext_ck != NULL ) {
		bext = (struct bext *)(bext_ck->bytes - RIFF_CK_HEADER_SZ);
		codh = get_bext_coding_history_list( bext );
	}

	if ( bext == NULL && 
	     ! ( user_opt.freq > 0  &&  user_opt.bitd > 0  &&  user_opt.chan > 0 ) ) {

		pr = verb( 2, "\n|  |  File contain no <bext> chunk." );
		pr = verb( 1, "\n|  |  You'll have to provide audio parameters using" );
		pr = verb( 1, "\n|  |  -f, -b and -c options." );

		goto failed;

	} else if ( bext != NULL && 
		    codh == NULL &&
		    ! ( user_opt.freq    > 0 && user_opt.bitd    > 0 && user_opt.chan > 0 ) ) {

		pr = verb( 2, "\n|  |  Found <bext> chunk with no coding_history data." );
		pr = verb( 1, "\n|  |  You'll have to provide audio parameters using" );
		pr = verb( 1, "\n|  |  -f, -b and -c options." );

		goto failed;

	} else if ( bext != NULL && 
		    codh != NULL && 
		    ! ( codh->frequency > 0  &&  codh->wordlength > 0  &&     codh->mode > 0 ) && 
		    ! ( user_opt.freq   > 0  &&  user_opt.bitd    > 0  &&  user_opt.chan > 0 ) ) {

		pr = verb( 2, "\n|  |  Found <bext> chunk with coding_history data," );
		pr = verb( 2, "\n|  |  but lacking of suitable data to rebuild <fmt >." );
		pr = verb( 1, "\n|  |  You'll have to provide audio parameters using" );
		pr = verb( 1, "\n|  |  -f, -b and -c options." );

		goto failed;

	} else if ( bext != NULL && 
		    codh != NULL && 
		    ( codh->frequency > 0   &&   codh->wordlength > 0   &&   codh->mode > 0 ) ) {

		pr = verb( 2, "\n|  |  Found <bext> chunk with coding_history data." );
		pr = verb( 2, "\n|  |  Using it to retrieve audio parameters. Rebuilding <fmt >.." );


		/*
			set <fmt > frequency
		*/
		if ( user_opt.freq != 0 && user_opt.freq != codh->frequency ) {

			pr = verb( 1, "\n|  |  -f option does not match <bext> (%d Hz).", 
				codh->frequency );

			if ( user_opt.force_user_values ) {
				pr = verb( 1, " Using -f." );
				fmt->samples_per_sec = user_opt.freq;
			} else {
				pr = verb( 1, " Using <bext>." );
				fmt->samples_per_sec = codh->frequency;
			}

		} else {
			fmt->samples_per_sec = codh->frequency;
		}


		/*
			set <fmt > word_length
		*/
		if ( user_opt.bitd != 0 && user_opt.bitd != codh->wordlength ) {
			pr = verb( 1, "\n|  |  -b option does not match <bext> (%d Bits). ", 
				codh->wordlength );

			if ( user_opt.force_user_values ) {
				pr = verb( 1, "Using -w." );
				fmt->bits_per_sample = user_opt.bitd;
			} else {
				pr = verb( 1, "Using <bext>." );
				fmt->bits_per_sample = codh->wordlength;
			}

		} else {
			fmt->bits_per_sample = codh->wordlength;
		}


		/*
			set <fmt > channels
		*/
		if ( user_opt.chan != 0 && user_opt.chan != bext_codh_mode_to_chan(codh->mode) ) {
			pr = verb( 1, "\n|  |  -c option does not match <bext> (%d Ch). ",
				bext_codh_mode_to_chan(codh->mode) );

			if ( user_opt.force_user_values ) {
				pr = verb( 1, "Using -c." );
				fmt->channels = user_opt.chan;
			} else {
				pr = verb( 1, "Using <bext>." );
				fmt->channels = bext_codh_mode_to_chan(codh->mode);
			}

		} else {
			fmt->channels = bext_codh_mode_to_chan(codh->mode);
		}


		/*
			set <fmt > avg_bytes_per_sec
		*/
		fmt->avg_bytes_per_sec = (fmt->channels * fmt->samples_per_sec * fmt->bits_per_sample) / 8;


		/*
			set <fmt > block_align
		*/
		fmt->block_align = (fmt->channels * fmt->bits_per_sample) / 8;

	} else if ( ( bext == NULL || 
		      codh == NULL || 
		      ! ( codh->frequency > 0  &&  codh->wordlength > 0  &&     codh->mode > 0 ) ) && 
		    ( ! ( user_opt.freq   > 0  &&  user_opt.bitd    > 0  &&  user_opt.chan > 0 ) ) ) {

		pr = verb( 2, "\n|  |  Using user audio parameters. Rebuilding <fmt >.." );

		fmt->samples_per_sec = user_opt.freq;
		fmt->bits_per_sample = user_opt.bitd;
		fmt->channels = user_opt.chan;

		fmt->avg_bytes_per_sec = (fmt->channels * fmt->samples_per_sec * fmt->bits_per_sample) / 8;
		fmt->block_align = (fmt->channels * fmt->bits_per_sample) / 8;

	}


	/*
		insert new <fmt > chunk in chunk_list
	*/
	struct chunk *fmt_ck = new_riff_chunk();

	fmt_ck->id[0] = 'f';
	fmt_ck->id[1] = 'm';
	fmt_ck->id[2] = 't';
	fmt_ck->id[3] = ' ';

	fmt_ck->sz = fmt->cksz;
	fmt_ck->bytes = (unsigned char *)(fmt) + RIFF_CK_HEADER_SZ;

	fmt_ck->free_bytes = (void *)&wav_free_fmt;

	insert_riff_chunk_before_id( ckls, fmt_ck, "data" );

	if ( codh != NULL )
		free_bext_coding_history( &codh );

	if ( pr )
		verb( 1, "\n|  |  done.\n" );
	else
		verb( 1, " done.\n" );

	return 1;	// success

failed:

	if ( fmt != NULL )
		free( fmt );

	if ( codh != NULL )
		free_bext_coding_history( &codh );

	if ( pr )
		verb( 1, "\n|  |  failed.\n" );
	else
		verb( 1, " failed.\n" );

	return 0;	// failed
}


int rebuild_data_chunk( struct chunk **ckls ) {

	struct chunk *tmp_ckls = *ckls;
	struct chunk *null_ck  = NULL;

	int pr = 0;

	pr = verb( 2, "\n|  |  Trying to locate the biggest unknown bytes block.." );

	/* 
		get the biggest NULL chunk (assume it
		is our data chunk missing ID/SIZE)
	*/

	size_t null_sz = 0;

	while ( tmp_ckls != NULL ) {

		if ( is_riff_chunk_id( tmp_ckls, "NULL" ) && tmp_ckls->sz > null_sz ) {
			null_sz = tmp_ckls->sz;
			null_ck = tmp_ckls;
		}

		tmp_ckls = tmp_ckls->next;
	}

	if ( null_ck == NULL ) {
		pr = verb( 1, "\n|  |  Missing unknown bytes suitable for <data> chunk." );
		goto error;
	}

	if ( null_ck->sz < RIFF_CK_HEADER_SZ ) {
		pr = verb( 1, "\n|  |  Biggest block is too small to contain <data> chunk." );
		goto error;
	}

	pr = verb( 2, "\n|  |  Got %u bytes begining at offset %zu.", 
		null_ck->sz, 
		get_riff_chunk_offset(*ckls, null_ck, OFFSET_FROM_BOF) );

	pr = verb( 2, "\n|  |  Assume these are audio data. Rebuilding <data>.." );

	/* 
		clean some null bytes if any.

		seen on Sound Device (SD552): 
		presume it is some reserved 
		space for <fmt > chunk..
	*/

	size_t offset = 0;

	while ( null_ck->bytes[offset] == 0x00 )
		offset++;


	/*
		seen on Sound Device (SD552):
		Missing only <data> chunk id, 
		but size is here..
	*/
	if ( null_ck->sz - offset == *(uint32_t *)(null_ck->bytes + offset) - 4 )
		offset += 4;

	null_ck->id[0] = 'd';
	null_ck->id[1] = 'a';
	null_ck->id[2] = 't';
	null_ck->id[3] = 'a';

	null_ck->sz -= offset;

	null_ck->bytes += offset;

	if ( pr )
		verb( 1, "\n|  |  done.\n" );
	else
		verb( 1, " done.\n" );

	return 1;	// success

error:

	if ( pr )
		verb( 1, "\n|  |  failed.\n" );
	else
		verb( 1, " failed.\n" );

	return 0;	// failed
}

int check_data_chunk( struct chunk **ckls, size_t file_sz ) {

	int pr = 0;

	struct chunk *data_ck = get_riff_chunk_by_id( *ckls, "data" );

	size_t offset_top = get_riff_chunk_offset( *ckls, data_ck, OFFSET_FROM_BOF );
	size_t offset_bot = get_riff_chunk_offset( *ckls, data_ck, OFFSET_FROM_EOF );

	/* 
		if there's nothing following <data> 
		and (<data> chunk size + ( all chunks sizes ) == file size), we're good.
	*/

	if ( get_riff_chunk_by_id( *ckls, "NULL") == NULL && 
	     (offset_top + offset_bot + data_ck->sz + RIFF_CK_HEADER_SZ) == file_sz ) {

		if ( pr )
			verb( 2, "\n|  |  ok.\n" );
		else
			verb( 1, " ok.\n" );

		return 0;	// ok for sure

	/*
		if (<data> chunk size is more than 90% of file size),
		assume we're good. This is for special case when some
		unknown bytes block remains.

		Note : At this point, <data> chunk should not contain
		garbage bytes.   It  should  be  valid size, or zero.
	*/

	} else if ( (data_ck->sz * 100 / file_sz) >= 90 ) {

		if ( pr )
			verb( 2, "\n|  |  ok.\n" );
		else
			verb( 1, " ok.\n" );

		return 0;	// assume ok
	}


	if ( data_ck->sz == 0 )
		pr = verb( 2, "\n|  |  Chunk size is 0 byte. That is very unlikely.." );
	

	if ( is_riff_chunk_id( data_ck->next, "NULL" ) ) {

		pr = verb( 2, "\n|  |  A block of %u unknown bytes comes after <data> chunk.", 
			data_ck->next->sz );
		pr = verb( 2, "\n|  |  Assume those are audio data. Merging them with <data> chunk.." );

		struct chunk *null_ck = data_ck->next;

		data_ck->sz += null_ck->sz;
		data_ck->next = null_ck->next;

		null_ck->next = NULL;
		free_riff_chunk_list( &null_ck );

		if ( pr )
			verb( 2, "\n|  |  done.\n" );
		else
			verb( 1, " done.\n" );

		return 1;	// successfully recovered
	}


	if ( pr )
		verb( 1, "\n|  |  failed.\n" );
	else
		verb( 1, " failed.\n");

	return 2;	// nothing to do
}

void show_usage( void ) {
	fprintf(stderr, "\n\
usage : wavfix <options> broken_file.wav\n\
        wavfix <options> /path/to/broken_files/* \n\
-h or --help for more details.\n\
");
	exit(1);
}

void show_help( void ) {

	printf("\n\
wavfix, version 0.1 by Adrien Gesta-Fline\n\
\n\
wavfix can repair corrupted wave files while keeping all meta-chunks intact.\n\
Its  ability  to  preserve  metadata  ensures  software  compatibility  and \n\
workflow integrity.\n\
\n\
usage : wavfix <options> broken_file.wav\n\
\n\
    audio options :\n\
        -f, --frequency   <n>     set frequency to n Hz (44100, 48000, etc.)\n\
        -b, --bit-depth   <n>     set bit-depth to n bits (16, 24, 32, etc.)\n\
        -c, --channels    <n>     set channels number to  n  channels (1 for\n\
                                  mono, 2 for stereo, etc.)\n\
\n\
        -F                        force using previous values instead of the\n\
                                  ones contained in <bext> chunk if present.\n\
\n\
    output options:\n\
        -s, --suffix              repaired file names will  be  the same  as\n\
                                  the broken ones plus this suffix.  Default\n\
                                  is '_REPAIRED'.\n\
\n\
    misc options :\n\
        -N, --no-repair           use this option to only investigate files.\n\
        -h, --help                display this help\n\
\n\
");

	exit(0);
}

int main( int argc, char *argv[] ) {

	int c;

	/*
		set default user values
	*/
	user_opt.freq = 0;
	user_opt.bitd = 0;
	user_opt.chan = 0;
	user_opt.force_user_values = 0;
	user_opt.no_repair = 0;
	user_opt.suffix = "_REPAIRED";
//	user_opt.out_dir = NULL;
	user_opt.verb = 3;


	static struct option long_options[] = {

		{"frequency",	required_argument, 0, 'f'},
		{"bit-depth",	required_argument, 0, 'b'},
		{"channels",	required_argument, 0, 'c'},

		{"suffix",	required_argument, 0, 's'},
		{"output-dir",	required_argument, 0, 'd'},
		{"no-repair",	no_argument,       0, 'N'},

//		{"verbose",	no_argument,       0, 'v'},
		{"help",	no_argument,       0, 'h'},

		{0, 0, 0, 0}
	};


	while ( 1 ) {

		/* getopt_long stores the option index here. */
		int option_index = 0;

		c = getopt_long (argc, argv, "hNFvf:b:c:s:d:",
                       long_options, &option_index);

		/* Detect the end of the options. */
		if ( c == -1 )
			break;

		switch ( c ) {

			case 'f':	user_opt.freq = atoi(optarg);	break;
			case 'b':	user_opt.bitd = atoi(optarg);	break;
			case 'c':	user_opt.chan = atoi(optarg);	break;

			case 'F':	user_opt.force_user_values = 1; break;

			case 's':	user_opt.suffix  = optarg;	break;
//			case 'd':	user_opt.out_dir = optarg;	break;

//			case 'v':	user_opt.verb++;		break;
			case 'N':	user_opt.no_repair = 1;		break;
			case 'h':	show_help();			break;

			/* getopt_long already printed an error message. */
			case '?':	show_usage();			break;

			default:	/*abort();*/			break;
		}
	}


	if ( optind == argc ) {
		fprintf(stderr, "missing wave file.\n");
		show_usage();
	}


	unsigned char *buf  = NULL;

	size_t file_size  = 0;
	char  *file_path  = NULL;
	char   outfile[NAME_MAX];

	struct chunk *ckls = NULL;
	struct chunk *ck   = NULL;

	int needs_fix = 0;
	int r = 0;

	printf("\n");


	/*
		loop through file list
	*/
	while ( optind < argc ) {

		verb( 1, "\n");


		file_path = argv[optind];

		verb( 1, "\033[1m> Processing '%s' \033[0m\n", file_path );

		needs_fix = 0;
		file_size = load_file( &buf, file_path );

		/*
			check we've loaded file
		*/
		if ( file_size == 0 ) {

			verb( 1, "\n\n" );

			goto next_file;
		}

		/*
			check RIFF and WAVE headers
		*/
		if ( *(uint32_t *)buf       != RIFF_ID ||
		     *(uint32_t *)(buf + 8) != WAVE_ID ) {

			verb( 1, "| File does not appear to be a valid RIFF WAVE.\n" );
//			verb( 1, "| If you're sure it is, re run with --force-riff\n" );

			goto next_file;
		}

		/*
			compare file size from stat 
			with file size from RIFF header
		*/
		if ( *(uint32_t *)(buf + 4) + 8 != file_size ) {

			verb( 1, "| [w] Wrong RIFF size: %010u B + 8 [file size: %010lu B;]\n", 
				*(uint32_t *)(buf + 4),
				file_size );

			needs_fix = 1;
		}

		/*
			retrieve chunks
		*/
		ckls = get_riff_chunk_list( buf, file_size );

		if ( ckls == NULL ) {
			printf( "\n[e] Could not retrieve any chunk. is the file totally empty ?\n\n" );
			goto next_file;
		}

		ck = ckls;
		size_t offset = RIFF_HEADER_SZ;
		int ck_count = 0;
		int null_ck_count = 0;

		verb( 1, "| Current file structure :\n| ======================\n" );

		while ( ck != NULL ) {

			verb( 1, "|     %c%c%c%c%c%c chunk [offset: %010zu; size: %010u + %d + %d;] %s%s\n",
				(is_riff_chunk_id(ck, "NULL")? '[' : '<'),
				ck->id[0],
				ck->id[1],
				ck->id[2],
				ck->id[3],
				(is_riff_chunk_id(ck, "NULL")? ']' : '>'),
				offset,
				ck->sz,
				(is_riff_chunk_id(ck, "NULL")? 0 : 4),
				(is_riff_chunk_id(ck, "NULL")? 0 : 4),
				(ck->sz % 2)? "odd" : "",
				(ck->sz % 2)? (ck->bytes + ck->sz + 1 == 0x00)? " w pad" : " w/ pad" : "" );

			if ( is_riff_chunk_id( ck, "NULL" ) )
				null_ck_count++;

			ck_count++;

			offset += ck->sz + RIFF_CK_HEADER_SZ;

			ck = ck->next;
		}

		verb( 1, "|\n" );

		/*
			check <data> chunk
		*/
		if ( get_riff_chunk_by_id( ckls, "data" ) == NULL ) {

			verb( 1, "| [w] Missing <data> chunk." );

			needs_fix = 1;

			if ( user_opt.no_repair ) {
				verb( 1, "\n" );
			} else {
				verb( 1, " recovering.. " );

				if ( rebuild_data_chunk( &ckls ) ) {
//					verb( 1, "done.\n" );
				} else {
//					verb( 1, "failed.\n" );
					goto next_file;
				}
			}

		} else {

			verb( 1, "| [i] Checking <data> chunk.. " );

			r = check_data_chunk( &ckls, file_size );

//			if ( user_opt.verb > 1 )
//				switch ( r ) {
//					case 0:	printf("ok.\n");	break;
//					case 1:	printf("done.\n");	break;
//					case 2:	printf("failed.\n"); 	break;
//				}

			if ( r == 1 )
				needs_fix = 1;

			if ( r == 2 ) {
				goto next_file;
			}
		}

		/*
			check <fmt > chunk
		*/
		if ( get_riff_chunk_by_id( ckls, "fmt " ) == NULL ) {

			verb( 1, "| [w] Missing <fmt > chunk." );
			needs_fix = 1;

			if ( user_opt.no_repair ) {
				verb( 1, "\n" );
			} else {
				verb( 1, " recovering.. " );

				if ( rebuild_fmt_chunk( &ckls ) ) {
//					verb( 1, "done.\n" );
				} else {
//					verb( 1, "failed.\n" );
					goto next_file;
				}
			}
		}

		/*
			see if there is any unknown data block remaining
			print new file structure if needed
		*/
		if ( needs_fix && user_opt.no_repair == 0 ) {

			verb( 1, "|\n| Recovered file structure :\n| ========================\n" );

			offset = RIFF_HEADER_SZ;
			null_ck_count = 0;
			ck = ckls;
			while ( ck != NULL ) {

				verb( 1, "|     %c%c%c%c%c%c chunk [offset: %010zu; size: %010u + %d + %d;] %s%s\n",
					(is_riff_chunk_id(ck, "NULL")? '[' : '<'),
					ck->id[0],
					ck->id[1],
					ck->id[2],
					ck->id[3],
					(is_riff_chunk_id(ck, "NULL")? ']' : '>'),
					offset,
					ck->sz,
					(is_riff_chunk_id(ck, "NULL")? 0 : 4),
					(is_riff_chunk_id(ck, "NULL")? 0 : 4),
					(ck->sz % 2)? "odd" : "",
					(ck->sz % 2)? (ck->bytes + ck->sz + 1 == 0x00)? " w pad" : " w/ pad" : "" );

				if ( is_riff_chunk_id( ck, "NULL") )
					null_ck_count++;

				offset += ck->sz + RIFF_CK_HEADER_SZ;

				ck = ck->next;
			}
		}

		/* TODO: ansure that unknown bytes appear after <fmt > and <data> */

		if ( user_opt.no_repair == 0 && null_ck_count ) {

			verb( 1, "| [w] %d unknwon bytes block%s remains.\n", 
				null_ck_count, 
				(null_ck_count > 1)? "s" : "" );

			verb( 1, "|     Some programs like  Pro Tools use wrong formated files\n");
			verb( 1, "|     like this ( <DIGI> chunk size missing  1 byte in PT ),\n");
			verb( 1, "|     so we wont correct it. Yet it should play fine anyway.\n");
		}

		verb( 1, "|\n" );

		/*
			write to file if needed
		*/
		if ( needs_fix ) {
			if ( user_opt.no_repair ) {
				verb( 1, "| File needs to be fixed.\n" );
			} else {

				build_output_file_path( outfile, file_path, user_opt.suffix );

				verb( 1, "| [i] Saving repaired file to '%s'\n", outfile );

				if ( write_repaired_file( ckls, outfile ) ) {
					verb( 1, "| File successfully recovered.\n" );
				} else {
					verb( 1, "| Failed writing output file.\n" );
				}
			}
		} else {
			verb( 1, "| File ok.\n" );
		}


next_file:

		if ( ckls != NULL )
			free_riff_chunk_list( &ckls );

		if ( buf != NULL ) {
			free( buf );
			buf = NULL;
		}

		optind++;
	}



	return 0;
}

