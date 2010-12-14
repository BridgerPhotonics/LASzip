/******************************************************************************
 *
 * Project:  integrating laszip into liblas - http://liblas.org -
 * Purpose:
 * Author:   Martin Isenburg
 *           isenburg at cs.unc.edu
 *
 ******************************************************************************
 * Copyright (c) 2010, Martin Isenburg
 *
 * This is free software; you can redistribute and/or modify it under
 * the terms of the GNU Lesser General Licence as published
 * by the Free Software Foundation.
 *
 * See the COPYING file for more information.
 *
 ****************************************************************************/

/*
===============================================================================

  FILE:  laswriteitemcompressed_v1.hpp
  
  CONTENTS:
  
    Implementation of LASitemReadCompressed for *all* items (version 1).

  PROGRAMMERS:
  
    martin isenburg@cs.unc.edu
  
  COPYRIGHT:
  
    copyright (C) 2007-2010  martin isenburg@cs.unc.edu
    
    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    12 December 2010 -- refactored after watching two movies with silke
  
===============================================================================
*/
#ifndef LAS_WRITE_ITEM_COMPRESSED_V1_H
#define LAS_WRITE_ITEM_COMPRESSED_V1_H

#include "laswriteitem.hpp"
#include "entropyencoder.hpp"
#include "integercompressor.hpp"

class LASwriteItemCompressed_POINT10_v1 : public LASwriteItemCompressed
{
public:

  LASwriteItemCompressed_POINT10_v1(EntropyEncoder* enc);

  BOOL init(U8* item);
  BOOL write(U8* item);

  ~LASwriteItemCompressed_POINT10_v1();

private:
  EntropyEncoder* enc;
  U8* last_item;

  I32 last_dir;
  I32 last_x_diff[2][3];
  I32 last_y_diff[2][3];
  I32 last_incr[2];
  IntegerCompressor* ic_dx;
  IntegerCompressor* ic_dy;
  IntegerCompressor* ic_z;
  EntropyModel* m_changed_values;
  IntegerCompressor* ic_intensity;
  EntropyModel* m_bit_byte;
  EntropyModel* m_classification;
  IntegerCompressor* ic_scan_angle_rank;
  EntropyModel* m_user_data;
  IntegerCompressor* ic_point_source_ID;
};

class LASwriteItemCompressed_GPSTIME_v1 : public LASwriteItemCompressed
{
public:

  LASwriteItemCompressed_GPSTIME_v1(EntropyEncoder* enc);

  BOOL init(U8* item);
  BOOL write(U8* item);

  ~LASwriteItemCompressed_GPSTIME_v1();

private:
  EntropyEncoder* enc;
  I64F64 last_gpstime;

  EntropyModel* m_gpstime_multi;
  EntropyModel* m_gpstime_0diff;
  IntegerCompressor* ic_gpstime;
  I32 multi_extreme_counter;
  I32 last_gpstime_diff;
};

class LASwriteItemCompressed_RGB_v1 : public LASwriteItemCompressed
{
public:

  LASwriteItemCompressed_RGB_v1(EntropyEncoder* enc);

  BOOL init(U8* item);
  BOOL write(U8* item);

  ~LASwriteItemCompressed_RGB_v1();

private:
  EntropyEncoder* enc;
  U8* last_item;

  IntegerCompressor* ic_rgb;
};

class LASwriteItemCompressed_BYTE_v1 : public LASwriteItemCompressed
{
public:

  LASwriteItemCompressed_BYTE_v1(EntropyEncoder* enc, U32 number);

  BOOL init(U8* item);
  BOOL write(U8* item);

  ~LASwriteItemCompressed_BYTE_v1();

private:
  EntropyEncoder* enc;
  U32 number;
  U8* last_item;

  IntegerCompressor* ic_byte;
};

#endif