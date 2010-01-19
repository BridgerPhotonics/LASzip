/*
===============================================================================

  FILE:  arithmeticencoder.cpp
  
  CONTENTS:
      
    see header file

  PROGRAMMERS:
  
    martin isenburg@cs.unc.edu
  
  COPYRIGHT:
  
    copyright (C) 2009 martin isenburg (isenburg@cs.unc.edu)
    
    This software is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    see header file
  
===============================================================================
*/
#include "arithmeticencoder.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

ArithmeticEncoder::ArithmeticEncoder(FILE* fp, bool store_bytes)
{
  if (fp && !store_bytes)
  {
    this->fp = fp;
    this->store_bytes = false;
    outbuffer = (U8*)malloc(sizeof(U8)*2*AC_BUFFER_SIZE);
    endbuffer = outbuffer + 2 * AC_BUFFER_SIZE;
  }
  else
  {
    this->fp = 0;
    this->store_bytes = true;
    outbuffer = (U8*)malloc(sizeof(U8)*store_bytes);
    endbuffer = outbuffer + store_bytes;
  }
  this->fp = fp;
  outbyte = outbuffer;
  endbyte = endbuffer;
  bytes_output = 0;
  base   = 0;
  length = AC__MaxLength;
}

void ArithmeticEncoder::encode(ArithmeticBitModel* m, U32 sym)
{
  assert(m && (sym <= 1));

  U32 x = m->bit_0_prob * (length >> BM__LengthShift);       // product l x p0
                                                            // update interval
  if (sym == 0) {
    length = x;
    ++m->bit_0_count;
  }
  else {
    U32 init_base = base;
    base += x;
    length -= x;
    if (init_base > base) propagate_carry();               // overflow = carry
  }

  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
  if (--m->bits_until_update == 0) m->update();       // periodic model update
}

void ArithmeticEncoder::encode(ArithmeticModel* m, U32 sym)
{
  assert(m && (sym <= m->last_symbol));

  U32 x, init_base = base;
                                                           // compute products
  if (sym == m->last_symbol) {
    x = m->distribution[sym] * (length >> DM__LengthShift);
    base   += x;                                            // update interval
    length -= x;                                          // no product needed
  }
  else {
    x = m->distribution[sym] * (length >>= DM__LengthShift);
    base   += x;                                            // update interval
    length  = m->distribution[sym+1] * length - x;
  }

  if (init_base > base) propagate_carry();                 // overflow = carry
  if (length < AC__MinLength) renorm_enc_interval();        // renormalization

  ++m->symbol_count[sym];
  if (--m->symbols_until_update == 0) m->update(true); // periodic model update
}

void ArithmeticEncoder::writeBit(U32 sym)
{
  assert(sym < 2);

  U32 init_base = base;
  base += sym * (length >>= 1);                // new interval base and length

  if (init_base > base) propagate_carry();                 // overflow = carry
  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

void ArithmeticEncoder::writeBits(U32 bits, U32 sym)
{
  assert(bits && (bits <= 32) && (sym < (1u<<bits)));

  if (bits > 19)
  {
    writeShort(sym&U16_MAX);
    sym = sym >> 16;
    bits = bits - 16;
  }

  U32 init_base = base;
  base += sym * (length >>= bits);             // new interval base and length

  if (init_base > base) propagate_carry();                 // overflow = carry
  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

void ArithmeticEncoder::writeByte(U8 sym)
{
  U32 init_base = base;
  base += (U32)(sym) * (length >>= 8);           // new interval base and length

  if (init_base > base) propagate_carry();                 // overflow = carry
  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

void ArithmeticEncoder::writeShort(U16 sym)
{
  U32 init_base = base;
  base += (U32)(sym) * (length >>= 16);          // new interval base and length

  if (init_base > base) propagate_carry();                 // overflow = carry
  if (length < AC__MinLength) renorm_enc_interval();        // renormalization
}

void ArithmeticEncoder::writeInt(U32 sym)
{
  writeShort((U16)(sym % U16_MAX_PLUS_ONE)); // lower 16 bits
  writeShort((U16)(sym / U16_MAX_PLUS_ONE)); // UPPER 16 bits
}

void ArithmeticEncoder::writeFloat(F32 sym)
{
  U32F32 u32f32;
  u32f32.f32 = sym;
  writeInt(u32f32.u32);
}

void ArithmeticEncoder::writeInt64(U64 sym)
{
  writeInt((U32)(sym % U32_MAX_PLUS_ONE)); // lower 32 bits
  writeInt((U32)(sym / U32_MAX_PLUS_ONE)); // UPPER 32 bits
}

void ArithmeticEncoder::writeDouble(F64 sym)
{
  U64F64 u64f64;
  u64f64.f64 = sym;
  writeInt64(u64f64.u64);
}

U32 ArithmeticEncoder::done()
{
  U32 init_base = base;                 // done encoding: set final data bytes

  if (length > 2 * AC__MinLength) {
    base  += AC__MinLength;                                     // base offset
    length = AC__MinLength >> 1;             // set new length for 1 more byte
  }
  else {
    base  += AC__MinLength >> 1;                                // base offset
    length = AC__MinLength >> 9;            // set new length for 2 more bytes
  }

  if (init_base > base) propagate_carry();                 // overflow = carry
  renorm_enc_interval();                // renormalization = output last bytes

  if (fp)
  {
    if (endbyte != endbuffer)
    {
      assert(outbyte < outbuffer + AC_BUFFER_SIZE);
      fwrite(outbuffer + AC_BUFFER_SIZE, 1, AC_BUFFER_SIZE, fp);
      bytes_output += AC_BUFFER_SIZE;
    }
    U32 buffer_size = outbyte - outbuffer;
    fwrite(outbuffer, 1, buffer_size, fp);
    bytes_output += buffer_size;
  }
  else if (store_bytes == false)
  {
    bytes_output += (outbyte - outbuffer);
  }

  return getNumberBytes();                             // number of bytes used
}

ArithmeticEncoder::~ArithmeticEncoder()
{
//  free(outbuffer);
}

U8* ArithmeticEncoder::getBytes()
{
  if (store_bytes)
    return outbuffer;
  else
    return 0;
}
  
U32 ArithmeticEncoder::getNumberBytes()
{
  if (store_bytes)
    return outbyte - outbuffer;
  else
    return bytes_output;
}

int max_carry = 0;

inline void ArithmeticEncoder::propagate_carry()
{
  int carry = 0;
  U8 * p;
  if (outbyte == outbuffer)
    p = endbuffer - 1;
  else
    p = outbyte - 1;
  while (*p == 0xFFU)
  {
    *p = 0;
    if (p == outbuffer)
      p = endbuffer - 1;
    else
      p--;
    assert(outbuffer <= p);
    assert(p < endbuffer);    
    assert(outbyte < endbuffer);    
    carry++;
  }
  ++*p;
  if (carry > max_carry) max_carry = carry;
}

inline void ArithmeticEncoder::renorm_enc_interval()
{
  do {                                          // output and discard top byte
    assert(outbuffer <= outbyte);
    assert(outbyte < endbuffer);    
    assert(outbyte < endbyte);    
    *outbyte++ = (U8)(base >> 24);
    if (outbyte == endbyte) manage_outbuffer();
    base <<= 8;
  } while ((length <<= 8) < AC__MinLength);        // length multiplied by 256
}

inline void ArithmeticEncoder::manage_outbuffer()
{
  if (fp)
  {
    if (outbyte == endbuffer) outbyte = outbuffer;
    fwrite(outbyte, 1, AC_BUFFER_SIZE, fp);
    bytes_output += AC_BUFFER_SIZE;
    endbyte = outbyte + AC_BUFFER_SIZE;
    assert(endbyte > outbyte);
    assert(outbyte < endbuffer);    
  }
  else if (store_bytes)
  {
    U32 buffer_size = endbyte - outbuffer;
    U8* newbuffer = (U8*) malloc(sizeof(U8)*buffer_size*2);
    memcpy(newbuffer,outbuffer,sizeof(U8)*buffer_size);
    free(outbuffer);
    outbuffer = newbuffer;
    endbuffer = newbuffer + buffer_size * 2;
    outbyte = outbuffer + buffer_size;
    endbyte = endbuffer;
  }
  else
  {
    bytes_output += 2 * AC_BUFFER_SIZE;
    outbyte = outbuffer;
  }
}