/* 
 * Copyright (C) 2001 Rich Wareham <richwareham@users.sourceforge.net>
 * 
 * This file is part of libdvdnav, a DVD navigation library.
 * 
 * libdvdnav is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * libdvdnav is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA
 *
 * $Id$
 *
 */

#ifndef DVDNAV_INTERNAL_H_INCLUDED
#define DVDNAV_INTERNAL_H_INCLUDED

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "dvdnav.h"
#include "vm.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>

#include <dvdread/dvd_reader.h>
#include <dvdread/ifo_read.h>
#include <dvdread/ifo_types.h>


/* Maximum length of an error string */
#define MAX_ERR_LEN 255

/* Use the POSIX PATH_MAX if available */
#ifdef PATH_MAX
#define MAX_PATH_LEN PATH_MAX
#else
#define MAX_PATH_LEN 255 /* Arbitary */
#endif

#ifndef DVD_VIDEO_LB_LEN
#define DVD_VIDEO_LB_LEN 2048
#endif

/*
 * These are defined here because they are
 * not in ifo_types.h, they maybe one day 
 */

#ifndef audio_status_t
typedef struct {
#ifdef WORDS_BIGENDIAN
  unsigned int available     : 1;
  unsigned int zero1         : 4;
  unsigned int stream_number : 3;
  uint8_t zero2;
#else
  uint8_t zero2;
  unsigned int stream_number : 3;
  unsigned int zero1         : 4;  
  unsigned int available     : 1;
#endif
} ATTRIBUTE_PACKED audio_status_t;
#endif

#ifndef spu_status_t
typedef struct {
#ifdef WORDS_BIGENDIAN
  unsigned int available               : 1;
  unsigned int zero1                   : 2;
  unsigned int stream_number_4_3       : 5;
  unsigned int zero2                   : 3;
  unsigned int stream_number_wide      : 5;
  unsigned int zero3                   : 3;
  unsigned int stream_number_letterbox : 5;
  unsigned int zero4                   : 3;
  unsigned int stream_number_pan_scan  : 5;
#else
  unsigned int stream_number_pan_scan  : 5;
  unsigned int zero4                   : 3;
  unsigned int stream_number_letterbox : 5;
  unsigned int zero3                   : 3;
  unsigned int stream_number_wide      : 5;
  unsigned int zero2                   : 3;
  unsigned int stream_number_4_3       : 5;
  unsigned int zero1                   : 2;
  unsigned int available               : 1;
#endif
} ATTRIBUTE_PACKED spu_status_t;
#endif

typedef struct dvdnav_vobu_s {
  int32_t vobu_start; /* Logical Absolute. MAX needed is 0x300000 */
  int32_t vobu_length; /* Relative offset */
  int32_t blockN; /* Relative offset */
  int32_t vobu_next; /* Relative offset */
} dvdnav_vobu_t;  
   
/* The main DVDNAV type */

struct dvdnav_s {
  /* General data */
  char path[MAX_PATH_LEN];        /* Path to DVD device/dir */
  dvd_file_t *file;               /* Currently opened file */
  int open_vtsN;                  /* The domain and number of the... */
  int open_domain;                /* ..currently opened VOB */
 
  /* Position data */
  vm_position_t position_next;
  vm_position_t position_current;
  dvdnav_vobu_t vobu;  
  cell_playback_t *cell;
  uint32_t jmp_blockN;
  uint32_t jmp_vobu_start;
  uint32_t seekto_block;

  /* NAV data */
  pci_t pci;
  dsi_t dsi;
  
  /* Flags */
  int expecting_nav_packet;
  int at_soc;      /* Are we at the start of a cell? */
  int still_frame; /* >=0 send still frame event with len still_frame
                    * -1 don't send event. */
  int jumping;     /* non-zero if we are in the process of jumping */
  int seeking;     /* non-zero if we are in the process of seeking */
  int stop;        /* Are we stopped? (note not paused, actually stopped) */
  int highlight_changed; /* The highlight changed */
  int spu_clut_changed; /* The SPU CLUT changed */ 
  int spu_stream_changed; /* The SPU STREAM changed */ 
  int audio_stream_changed; /* The AUDIO STREAM changed */ 
  int started; /* vm_start has been called? */
  int use_read_ahead; /* 1 - use read-ahead cache, 0 - don't */
  /* VM */
  vm_t* vm;
  pthread_mutex_t vm_lock;

  /* Highlight */
  int hli_state;  /* State of highlight 0 - disabled, 1 - selected,
		 2 - activated */
  uint16_t hli_bbox[4]; /* Highlight bounding box */
  uint32_t hli_clut; /* Highlight palette */
  uint32_t hli_pts;  /* Highlight PTS for matching with SPU packet. */
  uint32_t hli_buttonN;  /* Button number for SPU decoder. */
  /* Read-ahead cache. */
  uint8_t      *cache_buffer;
  int32_t      cache_start_sector; /* -1 means cache invalid */
  size_t       cache_block_count;
  size_t       cache_malloc_size;
  int          cache_valid;
  
  /* Errors */
  char err_str[MAX_ERR_LEN];
};

/* Common things we do everytime we do a jump */
void dvdnav_do_post_jump(dvdnav_t *self);

/** USEFUL MACROS **/

#define printerrf(format, args...) snprintf(this->err_str, MAX_ERR_LEN, format, ## args);
#define printerr(str) strncpy(this->err_str, str, MAX_ERR_LEN);
/* Save my typing */

#define S_ERR DVDNAV_STATUS_ERR
#define S_OK  DVDNAV_STATUS_OK

#endif /* DVDNAV_INTERNAL_H_INCLUDED */
