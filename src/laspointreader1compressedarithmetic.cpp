/*
===============================================================================

  FILE:  laspointreader1compressedarithmetic.cpp
  
  CONTENTS:
  
    see corresponding header file
  
  PROGRAMMERS:
  
    martin isenburg@cs.unc.edu
  
  COPYRIGHT:
  
    copyright (C) 2007  martin isenburg@cs.unc.edu
    
    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    see corresponding header file
  
===============================================================================
*/

#include "laspointreader1compressedarithmetic.h"

#define MULTI_MAX 512

bool LASpointReader1compressedArithmetic::read_point(LASpoint* point, double* gps_time, unsigned short* rgb)
{
	if (dec)
	{
		*point = last_point;

    // find the median difference among the three preceding differences
    int median_x;
    if (last_x_diff[last_dir][0] < last_x_diff[last_dir][1])
    {
      if (last_x_diff[last_dir][1] < last_x_diff[last_dir][2])
        median_x = last_x_diff[last_dir][1];
      else if (last_x_diff[last_dir][0] < last_x_diff[last_dir][2])
        median_x = last_x_diff[last_dir][2];
      else
        median_x = last_x_diff[last_dir][0];
    }
    else
    {
      if (last_x_diff[last_dir][0] < last_x_diff[last_dir][2])
        median_x = last_x_diff[last_dir][0];
      else if (last_x_diff[last_dir][1] < last_x_diff[last_dir][2])
        median_x = last_x_diff[last_dir][2];
      else
        median_x = last_x_diff[last_dir][1];
    }

    int median_y;
    if (last_y_diff[last_dir][0] < last_y_diff[last_dir][1])
    {
      if (last_y_diff[last_dir][1] < last_y_diff[last_dir][2])
        median_y = last_y_diff[last_dir][1];
      else if (last_y_diff[last_dir][0] < last_y_diff[last_dir][2])
        median_y = last_y_diff[last_dir][2];
      else
        median_y = last_y_diff[last_dir][0];
    }
    else
    {
      if (last_y_diff[last_dir][0] < last_y_diff[last_dir][2])
        median_y = last_y_diff[last_dir][0];
      else if (last_y_diff[last_dir][1] < last_y_diff[last_dir][2])
        median_y = last_y_diff[last_dir][2];
      else
        median_y = last_y_diff[last_dir][1];
    }

    // decompress x y z coordinates
    int x_diff = ic_dx->Decompress(median_x, last_dir);
    point->x += x_diff;
    // we use the number k of bits corrector bits to switch contexts
    int k_bits = ic_dx->GetK();
    int y_diff = ic_dy->Decompress(median_y, k_bits);
    point->y += y_diff;
    k_bits = (ic_dx->GetK() + ic_dy->GetK())/2;
    point->z = ic_z->Decompress(last_point.z, k_bits);

    // decompress which other values have changed
		int changed_values = dec->decode(m_changed_values);

		// decompress the intensity if it has changed
		if (changed_values & 32)
		{
			point->intensity = ic_intensity->Decompress(last_point.intensity);
		}

		// decompress the edge_of_flight_line, scan_direction_flag, ... if it has changed
		if (changed_values & 16)
		{
			((unsigned char*)point)[14] = dec->decode(m_bit_byte);
		}

		// decompress the classification ... if it has changed
		if (changed_values & 8)
		{
			point->classification = dec->decode(m_classification);
		}
		
		// decompress the scan_angle_rank ... if it has changed
		if (changed_values & 4)
		{
			point->scan_angle_rank = ic_scan_angle_rank->Decompress(last_point.scan_angle_rank, k_bits < 3);
		}

		// decompress the user_data ... if it has changed
		if (changed_values & 2)
		{
			point->user_data = dec->decode(m_user_data);
		}

		// decompress the point_source_ID ... if it has changed
		if (changed_values & 1)
		{
			point->point_source_ID = ic_point_source_ID->Decompress(last_point.point_source_ID);
		}

		// decompress the gps_time ... if it has changed
		if (changed_values & 64)
		{
			int multi = dec->decode(m_gps_time_multi[k_bits < 3]);

      if (multi <  MULTI_MAX-1)
      {
        int gps_time_diff;
 
        if (multi == 1)
        {
          gps_time_diff = ic_gps_time->Decompress(last_gps_time_diff, 0);
          last_gps_time_diff = gps_time_diff;
          multi_extreme_counter = 0;
        }
	      else if (multi == 0)
	      {
          gps_time_diff = ic_gps_time->Decompress(last_gps_time_diff/2, 3);
          multi_extreme_counter++;
	        if (multi_extreme_counter > 3)
	        {
	          last_gps_time_diff = gps_time_diff;
	          multi_extreme_counter = 0;
	        }
	      }
	      else if (multi < 10)
	      {
	        gps_time_diff = ic_gps_time->Decompress(multi*last_gps_time_diff, 1);
	      }
	      else
	      {
	        gps_time_diff = ic_gps_time->Decompress(multi*last_gps_time_diff, 2);
	        if (multi == MULTI_MAX-2)
	        {
	          multi_extreme_counter++;
	          if (multi_extreme_counter > 3)
	          {
	            last_gps_time_diff = gps_time_diff;
	            multi_extreme_counter = 0;
	          }
	        }
	      }
	      last_gps_time.i64 += gps_time_diff;
      }
      else
      {
        last_gps_time.f64 = dec->readDouble();
      }
    }

    // only record the difference if the scan direction has not changed
    if (last_dir == point->scan_direction_flag)
    {
      last_x_diff[last_dir][last_incr[last_dir]] = x_diff;
      last_y_diff[last_dir][last_incr[last_dir]] = y_diff;

      last_incr[last_dir]++;
      if (last_incr[last_dir] > 2) last_incr[last_dir] = 0;
    }
	}
	else
	{
		fread(point, sizeof(LASpoint), 1, file);
		fread(&(last_gps_time.f64), sizeof(double), 1, file);
		init_decoder();
	}

  last_dir = point->scan_direction_flag;
	last_point = *point;
	*gps_time = last_gps_time.f64;

	return true;
}

LASpointReader1compressedArithmetic::LASpointReader1compressedArithmetic(FILE* file)
{
	this->file = file;
  last_dir = 0;
	last_x_diff[0][0] = last_x_diff[0][1] = last_x_diff[0][2] = last_x_diff[1][0] = last_x_diff[1][1] = last_x_diff[1][2] = 0;
	last_y_diff[0][0] = last_y_diff[0][1] = last_y_diff[0][2] = last_y_diff[1][0] = last_y_diff[1][1] = last_y_diff[1][2] = 0;
  last_incr[0] = last_incr[1] = 0;
	last_gps_time.f64 = 0;
	last_gps_time_diff = 0;
	dec = 0;
}

void LASpointReader1compressedArithmetic::init_decoder()
{
	dec = new ArithmeticDecoder(file);

	ic_dx = new ArithmeticIntegerCompressor();
	ic_dy = new ArithmeticIntegerCompressor();
	ic_z = new ArithmeticIntegerCompressor();

	ic_dx->SetPrecision(32); 
	ic_dy->SetPrecision(32); 
	ic_z->SetPrecision(32); 

  ic_dx->SetupDecompressor(dec, 2);
  ic_dy->SetupDecompressor(dec, 33);
  ic_z->SetupDecompressor(dec, 33);

	m_changed_values = new ArithmeticModel(128,0,0);

	ic_intensity = new ArithmeticIntegerCompressor();
	ic_intensity->SetPrecision(16);
  ic_intensity->SetupDecompressor(dec);

	m_bit_byte = new ArithmeticModel(256,0,0);
	m_classification = new ArithmeticModel(256,0,0);

	ic_scan_angle_rank = new ArithmeticIntegerCompressor();
	ic_scan_angle_rank->SetPrecision(8);
  ic_scan_angle_rank->SetupDecompressor(dec, 2);

	m_user_data = new ArithmeticModel(256,0,0);

	ic_point_source_ID = new ArithmeticIntegerCompressor();
	ic_point_source_ID->SetPrecision(16);
  ic_point_source_ID->SetupDecompressor(dec);

  m_gps_time_multi = new ArithmeticModel*[2];
  for (int i = 0; i < 2; i++) m_gps_time_multi[i] = new ArithmeticModel(MULTI_MAX,0,0);

  ic_gps_time = new ArithmeticIntegerCompressor();
	ic_gps_time->SetPrecision(32);
  ic_gps_time->SetupDecompressor(dec, 4);

	multi_extreme_counter = 0;
}

LASpointReader1compressedArithmetic::~LASpointReader1compressedArithmetic()
{
	if (dec)
	{
		dec->done();

		delete dec;
		delete ic_dx;
		delete ic_dy;
		delete ic_z;
		delete m_changed_values;
		delete ic_intensity;
		delete m_bit_byte;
		delete m_classification;
		delete ic_scan_angle_rank;
		delete m_user_data;
		delete ic_point_source_ID;
    for (int i = 0; i < 2; i++) delete m_gps_time_multi[i];
		delete [] m_gps_time_multi;
		delete ic_gps_time;
	}
}