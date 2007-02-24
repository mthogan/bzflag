/* bzflag
 * Copyright (c) 1993 - 2007 Tim Riker
 *
 * This package is free software;  you can redistribute it and/or
 * modify it under the terms of the license found in the file
 * named COPYING that should have accompanied this file.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* interface header */
#include "DynamicColor.h"

/* system implementation headers */
#include <math.h>
#include <string.h>
#include <string.h>
#include <stdlib.h>
#include <vector>

/* common implementation headers */
#include "GameTime.h"
#include "Pack.h"
#include "TimeKeeper.h"
#include "ParseColor.h"
#include "StateDatabase.h"


// this applies to all function periods
const float DynamicColor::minPeriod = 0.01f;


//
// Dynamic Color Manager
//

DynamicColorManager DYNCOLORMGR;


DynamicColorManager::DynamicColorManager()
{
  return;
}


DynamicColorManager::~DynamicColorManager()
{
  clear();
  return;
}


void DynamicColorManager::clear()
{
  std::vector<DynamicColor*>::iterator it;
  for (it = colors.begin(); it != colors.end(); it++) {
    delete *it;
  }
  colors.clear();
  return;
}


void DynamicColorManager::update()
{
  const double gameTime = GameTime::getStepTime();
  std::vector<DynamicColor*>::iterator it;
  for (it = colors.begin(); it != colors.end(); it++) {
    DynamicColor* color = *it;
    color->update(gameTime);
  }
  return;
}


int DynamicColorManager::addColor(DynamicColor* color)
{
  colors.push_back (color);
  return ((int)colors.size() - 1);
}


int DynamicColorManager::findColor(const std::string& dyncol) const
{
  if (dyncol.size() <= 0) {
    return -1;
  }
  else if ((dyncol[0] >= '0') && (dyncol[0] <= '9')) {
    int index = atoi (dyncol.c_str());
    if ((index < 0) || (index >= (int)colors.size())) {
      return -1;
    } else {
      return index;
    }
  }
  else {
    for (int i = 0; i < (int)colors.size(); i++) {
      if (colors[i]->getName() == dyncol) {
	return i;
      }
    }
    return -1;
  }
}


const DynamicColor* DynamicColorManager::getColor(int id) const
{
  if ((id >= 0) && (id < (int)colors.size())) {
    return colors[id];
  } else {
    return NULL;
  }
}


void * DynamicColorManager::pack(void *buf) const
{
  std::vector<DynamicColor*>::const_iterator it;
  buf = nboPackUInt(buf, (int)colors.size());
  for (it = colors.begin(); it != colors.end(); it++) {
    const DynamicColor* color = *it;
    buf = color->pack(buf);
  }
  return buf;
}


void * DynamicColorManager::unpack(void *buf)
{
  unsigned int i;
  uint32_t count;
  buf = nboUnpackUInt (buf, count);
  for (i = 0; i < count; i++) {
    DynamicColor* color = new DynamicColor;
    buf = color->unpack(buf);
    addColor(color);
  }
  return buf;
}


int DynamicColorManager::packSize() const
{
  int fullSize = sizeof (uint32_t);
  std::vector<DynamicColor*>::const_iterator it;
  for (it = colors.begin(); it != colors.end(); it++) {
    DynamicColor* color = *it;
    fullSize = fullSize + color->packSize();
  }
  return fullSize;
}


void DynamicColorManager::print(std::ostream& out, const std::string& indent) const
{
  std::vector<DynamicColor*>::const_iterator it;
  for (it = colors.begin(); it != colors.end(); it++) {
    DynamicColor* color = *it;
    color->print(out, indent);
  }
  return;
}


//
// Dynamic Color
//

DynamicColor::DynamicColor()
{
  // setup the channels
  for (int c = 0; c < 4; c++) {
    // the parameters are setup so that all channels
    // are at 1.0f, and that there are no variations
    color[c] = 1.0f;
    channels[c].minValue = 0.0f;
    channels[c].maxValue = 1.0f;
    channels[c].sequence.count = 0;
    channels[c].sequence.list = NULL;
  }
  possibleAlpha = false;
  name = "";

  varName = "";
  varUseAlpha = false;
  varTiming = 1.0f;

  const float white[4] = {1.0f, 1.0f, 1.0f, 1.0f};
  varInit = false;
  varTimingTmp = varTiming;
  varTransition = false;
  memcpy(varOldColor, white, sizeof(float[4]));
  memcpy(varNewColor, white, sizeof(float[4]));
  varLastChange = TimeKeeper::getSunGenesisTime();

  return;
}


DynamicColor::~DynamicColor()
{
  // free the sequence values
  for (int c = 0; c < 4; c++) {
    sequenceParams& seq = channels[c].sequence;
    delete[] seq.list;
  }
  if (varInit) {
    BZDB.removeCallback(varName, bzdbCallback, this);
  }
  return;
}


void DynamicColor::finalize()
{
  // variables take priority
  if (varName.size() > 0) {
    possibleAlpha = varUseAlpha;
    return; // nothing else matters
  }

  // sanitize the sequence value, and check for alpha middle values
  bool noAlphaSeqMid = true;
  for (int c = 0; c < 4; c++) {
    sequenceParams& seq = channels[c].sequence;
    for (unsigned int i = 0; i < seq.count; i++) {
      if ((int)seq.list[i] < (int)colorMin) {
	seq.list[i] = colorMin;
      } else if ((int)seq.list[i] > (int)colorMax) {
	seq.list[i] = colorMax;
      } else if ((c == 3) && (seq.list[i] == colorMid)) {
	noAlphaSeqMid = false;
      }
    }
  }

  // check if there might be translucency
  possibleAlpha = true;
  const ChannelParams& p = channels[3]; // the alpha channel

  if ((p.minValue == 1.0f) && (p.maxValue == 1.0f)) {
    // opaque regardless of functions
    possibleAlpha = false;
  }
  else {

    // check the a special sequence case
    const sequenceParams& seq = p.sequence;
    if ((seq.count > 0) &&
	(noAlphaSeqMid && (p.minValue == 0.0f) && (p.maxValue == 1.0f))) {
      // transparency, not translucency
      possibleAlpha = false;
    }
    else {
      // check for the common case
      if ((p.sinusoids.size() == 0) &&
	  (p.clampUps.size() == 0) &&
	  (p.clampDowns.size() == 0) &&
	  (p.sequence.count == 0)) {
	// not using any functions
	if (p.maxValue == 1.0f) {
	  possibleAlpha = false;
	}
      }
    }
  }

  return;
}


bool DynamicColor::setName(const std::string& dyncol)
{
  if (dyncol.size() <= 0) {
    name = "";
    return false;
  }
  else if ((dyncol[0] >= '0') && (dyncol[0] <= '9')) {
    name = "";
    return false;
  }
  else {
    name = dyncol;
  }
  return true;
}


const std::string& DynamicColor::getName() const
{
  return name;
}


void DynamicColor::setVariableName(const std::string& vName)
{
  varName = vName;
  return;
}


void DynamicColor::setVariableTiming(float seconds)
{
  varTiming = seconds;
  return;
}


void DynamicColor::setVariableUseAlpha(bool value)
{
  varUseAlpha = value;
  return;
}


void DynamicColor::setLimits(int channel, float min, float max)
{
  if (min < 0.0f) {
    min = 0.0f;
  }
  else if (min > 1.0f) {
    min = 1.0f;
  }

  if (max < 0.0f) {
    max = 0.0f;
  }
  else if (max > 1.0f) {
    max = 1.0f;
  }

  channels[channel].minValue = min;
  channels[channel].maxValue = max;

  return;
}


void DynamicColor::setSequence(int channel,float period, float offset,
				std::vector<char>& list)
{
  sequenceParams& seq = channels[channel].sequence;

  delete[] seq.list;
  seq.list = NULL;
  seq.count = 0;

  if (period >= minPeriod) {
    seq.period = period;
    seq.offset = offset;
    seq.count = (unsigned int)list.size();
    seq.list = new char[seq.count];
    for (unsigned int i = 0; i < seq.count; i++) {
      seq.list[i] = list[i];
    }
  }

  return;
}


void DynamicColor::addSinusoid(int channel, const float sinusoid[3])
{
  sinusoidParams params;
  // check period and weight
  if ((sinusoid[0] >= minPeriod) && (sinusoid[2] > 0.0f)) {
    params.period = sinusoid[0];
    params.offset = sinusoid[1];
    params.weight = sinusoid[2];
    channels[channel].sinusoids.push_back(params);
  }
  return;
}


void DynamicColor::addClampUp(int channel, const float clampUp[3])
{
  clampParams params;
  // check period and width
  if ((clampUp[0] >= minPeriod) && (clampUp[2] > 0.0f)) {
    params.period = clampUp[0];
    params.offset = clampUp[1];
    params.width = clampUp[2];
    channels[channel].clampUps.push_back(params);
  }
  return;
}


void DynamicColor::addClampDown(int channel, const float clampDown[3])
{
  clampParams params;
  // check period and width
  if ((clampDown[0] >= minPeriod) && (clampDown[2] > 0.0f)) {
    params.period = clampDown[0];
    params.offset = clampDown[1];
    params.width = clampDown[2];
    channels[channel].clampDowns.push_back(params);
  }
  return;
}


void DynamicColor::bzdbCallback(const std::string& /*varName*/, void* data)
{
  ((DynamicColor*)data)->updateVariable();
  return;
}


void DynamicColor::updateVariable()
{
  // setup the basics
  varTransition = true;
  varLastChange = TimeKeeper::getTick();
  memcpy(varOldColor, color, sizeof(float[4]));
  std::string expr = BZDB.get(varName);

  // parse the optional delay timing
  std::string::size_type atpos = expr.find_first_of('@');
  if (atpos == std::string::npos) {
    varTimingTmp = varTiming;
  }
  else {
    char* end;
    const char* start = expr.c_str() + atpos + 1;
    varTimingTmp = (float)strtod(start, &end);
    if (end == start) {
      varTimingTmp = varTiming; // conversion failed
    }
    expr.resize(atpos); // strip everything after '@'
  }

  parseColorString(expr, varNewColor);
  return;
}


void DynamicColor::update (double t)
{
  // variables take priority
  if (varName.size() > 0) {
    // process the variable value
    if (!varInit) {
      varInit = true;
      varTransition = false;
      std::string expr = BZDB.get(varName);
      std::string::size_type atpos = expr.find_first_of('@');
      if (atpos != std::string::npos) {
	expr.resize(atpos);
      }
      parseColorString(expr, color);
      memcpy(varOldColor, color, sizeof(float[4]));
      memcpy(varNewColor, color, sizeof(float[4]));
      BZDB.addCallback(varName, bzdbCallback, this);
    }

    // setup the color value
    if (varTransition) {
      const float diffTime = (float)(TimeKeeper::getTick() - varLastChange);
      if (diffTime < varTimingTmp) {
	const float newScale = (varTimingTmp > 0.0f) ? (diffTime / varTimingTmp) : 1.0f;
	const float oldScale = 1.0f - newScale;
	color[0] = (oldScale * varOldColor[0]) + (newScale * varNewColor[0]);
	color[1] = (oldScale * varOldColor[1]) + (newScale * varNewColor[1]);
	color[2] = (oldScale * varOldColor[2]) + (newScale * varNewColor[2]);
	color[3] = (oldScale * varOldColor[3]) + (newScale * varNewColor[3]);
      }
      else {
	// make sure the final color is set exactly
	varTransition = false;
	memcpy(color, varNewColor, sizeof(float[4]));
      }
    }

    return; // nothing else matters
  }

  // update by color channel
  for (int c = 0; c < 4; c++) {
    const ChannelParams& channel = channels[c];
    unsigned int i;

    bool clampUp = false;
    bool clampDown = false;

    // sequence rules over the clamps
    const sequenceParams& seq = channel.sequence;
    if (seq.count > 0) {
      const double seqPeriod = (double)seq.period;
      const double fullPeriod = (double)seq.count * seqPeriod;
      double indexTime = (t - (double)seq.offset);
      if (indexTime < 0.0) {
	indexTime -= fullPeriod * floor(indexTime / fullPeriod);
      }
      indexTime = fmod (indexTime, fullPeriod);
      const unsigned int index = (unsigned int)(indexTime / seqPeriod);
      if (seq.list[index] == colorMin) {
	clampDown = true;
      }
      else if (seq.list[index] == colorMax) {
	clampUp = true;
      }
    }
    else {
      // check for active clampUp
      for (i = 0; i < channel.clampUps.size(); i++) {
	const clampParams& clamp = channel.clampUps[i];
	double upTime = (t - (double)clamp.offset);
	const double clampPeriod = (double)clamp.period;
	if (upTime < 0.0) {
	  upTime -= (double)clamp.period * floor(upTime / clampPeriod);
	}
	upTime = fmod (upTime, clampPeriod);
	if (upTime < (double)clamp.width) {
	  clampUp = true;
	  break;
	}
      }

      // check for active clampDown
      for (i = 0; i < channel.clampDowns.size(); i++) {
	const clampParams& clamp = channel.clampDowns[i];
	double downTime = (t - (double)clamp.offset);
	const double clampPeriod = (double)clamp.period;
	if (downTime < 0.0) {
	  downTime -= clampPeriod * floor(downTime / clampPeriod);
	}
	downTime = fmod (downTime, clampPeriod);
	if (downTime < (double)clamp.width) {
	  clampDown = true;
	  break;
	}
      }
    }

    // the amount of 'max' in the resultant channel's value
    float factor = 1.0f;

    // check the clamps
    if (clampUp && clampDown) {
      factor = 0.5f;
    }
    else if (clampUp) {
      factor = 1.0f;
    }
    else if (clampDown) {
      factor = 0.0f;
    }
    // no clamps, try sinusoids
    else if (channel.sinusoids.size() > 0) {
      float value = 0.0f;
      for (i = 0; i < channel.sinusoids.size(); i++) {
	const sinusoidParams& s = channel.sinusoids[i];
	const double phase = ((t - (double)s.offset) / (double)s.period);
	const double clampedPhase = fmod(phase, 1.0);
	value += s.weight * (float)cos(clampedPhase * (M_PI * 2.0));
      }
      // center the factor
      factor = 0.5f + (0.5f * value);
      if (factor < 0.0f) {
	factor = 0.0f;
      }
      else if (factor > 1.0f) {
	factor = 1.0f;
      }
    }

    color[c] = (channel.minValue * (1.0f - factor)) +
	       (channel.maxValue * factor);
  }

  return;
}


void * DynamicColor::pack(void *buf) const
{
  buf = nboPackStdString(buf, name);

  buf = nboPackStdString(buf, varName);
  buf = nboPackUByte(buf, varUseAlpha ? 1 : 0);
  buf = nboPackFloat(buf, varTiming);

  for (int c = 0; c < 4; c++) {
    const ChannelParams& p = channels[c];
    unsigned int i;

    buf = nboPackFloat (buf, p.minValue);
    buf = nboPackFloat (buf, p.maxValue);

    // sinusoids
    buf = nboPackUInt (buf, (unsigned int)p.sinusoids.size());
    for (i = 0; i < p.sinusoids.size(); i++) {
      buf = nboPackFloat (buf, p.sinusoids[i].period);
      buf = nboPackFloat (buf, p.sinusoids[i].offset);
      buf = nboPackFloat (buf, p.sinusoids[i].weight);
    }
    // clampUps
    buf = nboPackUInt (buf, (unsigned int)p.clampUps.size());
    for (i = 0; i < p.clampUps.size(); i++) {
      buf = nboPackFloat (buf, p.clampUps[i].period);
      buf = nboPackFloat (buf, p.clampUps[i].offset);
      buf = nboPackFloat (buf, p.clampUps[i].width);
    }
    // clampDowns
    buf = nboPackUInt (buf, (unsigned int)p.clampDowns.size());
    for (i = 0; i < p.clampDowns.size(); i++) {
      buf = nboPackFloat (buf, p.clampDowns[i].period);
      buf = nboPackFloat (buf, p.clampDowns[i].offset);
      buf = nboPackFloat (buf, p.clampDowns[i].width);
    }
    // sequence
    const sequenceParams& seq = p.sequence;
    buf = nboPackUInt (buf, seq.count);
    if (seq.count > 0) {
      buf = nboPackFloat (buf, seq.period);
      buf = nboPackFloat (buf, seq.offset);
      for (i = 0; i < seq.count; i++) {
	buf = nboPackUByte (buf, (uint8_t)seq.list[i]);
      }
    }
  }

  return buf;
}


void * DynamicColor::unpack(void *buf)
{
  buf = nboUnpackStdString(buf, name);

  uint8_t u8;
  buf = nboUnpackStdString(buf, varName);
  buf = nboUnpackUByte(buf, u8);
  varUseAlpha = (u8 != 0);
  buf = nboUnpackFloat(buf, varTiming);

  for (int c = 0; c < 4; c++) {
    ChannelParams& p = channels[c];
    unsigned int i;
    uint32_t size;

    buf = nboUnpackFloat (buf, p.minValue);
    buf = nboUnpackFloat (buf, p.maxValue);

    // sinusoids
    buf = nboUnpackUInt (buf, size);
    p.sinusoids.resize(size);
    for (i = 0; i < size; i++) {
      buf = nboUnpackFloat (buf, p.sinusoids[i].period);
      buf = nboUnpackFloat (buf, p.sinusoids[i].offset);
      buf = nboUnpackFloat (buf, p.sinusoids[i].weight);
    }
    // clampUps
    buf = nboUnpackUInt (buf, size);
    p.clampUps.resize(size);
    for (i = 0; i < size; i++) {
      buf = nboUnpackFloat (buf, p.clampUps[i].period);
      buf = nboUnpackFloat (buf, p.clampUps[i].offset);
      buf = nboUnpackFloat (buf, p.clampUps[i].width);
    }
    // clampDowns
    buf = nboUnpackUInt (buf, size);
    p.clampDowns.resize(size);
    for (i = 0; i < size; i++) {
      buf = nboUnpackFloat (buf, p.clampDowns[i].period);
      buf = nboUnpackFloat (buf, p.clampDowns[i].offset);
      buf = nboUnpackFloat (buf, p.clampDowns[i].width);
    }
    // sequence
    buf = nboUnpackUInt (buf, size);
    sequenceParams& seq = p.sequence;
    seq.count = size;
    if (size > 0) {
      buf = nboUnpackFloat (buf, seq.period);
      buf = nboUnpackFloat (buf, seq.offset);
      seq.list = new char[size];
      for (i = 0; i < size; i++) {
	uint8_t value;
	buf = nboUnpackUByte (buf, value);
	seq.list[i] = value;
      }
    } else {
      seq.list = NULL;
    }
  }

  finalize();

  return buf;
}


int DynamicColor::packSize() const
{
  int fullSize = 0;
  fullSize += nboStdStringPackSize(name);
  fullSize += nboStdStringPackSize(varName);
  fullSize += sizeof(uint8_t); // varUseAlpha
  fullSize += sizeof(float); // varTiming
  for (int c = 0; c < 4; c++) {
    fullSize += sizeof(float) * 2; // the limits
    fullSize += sizeof(uint32_t);
    fullSize += (int)(channels[c].sinusoids.size() * (sizeof(sinusoidParams)));
    fullSize += sizeof(uint32_t);
    fullSize += (int)(channels[c].clampUps.size() * (sizeof(clampParams)));
    fullSize += sizeof(uint32_t);
    fullSize += (int)(channels[c].clampDowns.size() * (sizeof(clampParams)));
    fullSize += sizeof(uint32_t);
    const sequenceParams& seq = channels[c].sequence;
    if (seq.count > 0) {
      fullSize += sizeof(float) * 2; // period and offset
      fullSize += seq.count * sizeof(char);
    }
  }
  return fullSize;
}


void DynamicColor::print(std::ostream& out, const std::string& indent) const
{
  const char *colorStrings[4] = { "red", "green", "blue", "alpha" };

  out << indent << "dynamicColor" << std::endl;

  if (name.size() > 0) {
    out << indent << "  name " << name << std::endl;
  }

  if (varName.size() > 0) {
    out << indent << "  variable " << varName << std::endl;
    if (varUseAlpha) {
      out << indent << "  varUseAlpha " << varName << std::endl;
    }
    if (varTiming != 1.0f) {
      out << indent << "  varTiming " << varTiming << std::endl;
    }
  }

  for (int c = 0; c < 4; c++) {
    const char *colorStr = colorStrings[c];
    const ChannelParams& p = channels[c];
    if ((p.minValue != 0.0f) || (p.maxValue != 1.0f)) {
      out << indent << "  " << colorStr << " limits "
	  << p.minValue << " " << p.maxValue << std::endl;
    }
    unsigned int i;
    if (p.sequence.count > 0) {
      out << indent << "  " << colorStr << " sequence " << p.sequence.period << " "
					      << p.sequence.offset;
      for (i = 0; i < p.sequence.count; i++) {
	out << " " << (int)p.sequence.list[i];
      }
      out << std::endl;
    }
    for (i = 0; i < p.sinusoids.size(); i++) {
      const sinusoidParams& f = p.sinusoids[i];
      out << indent << "  " << colorStr << " sinusoid "
	  << f.period << " " << f.offset << " " << f.weight << std::endl;
    }
    for (i = 0; i < p.clampUps.size(); i++) {
      const clampParams& f = p.clampUps[i];
      out << indent << "  " << colorStr << " clampup "
	  << f.period << " " << f.offset << " " << f.width << std::endl;
    }
    for (i = 0; i < p.clampDowns.size(); i++) {
      const clampParams& f = p.clampDowns[i];
      out << indent << "  " << colorStr << " clampdown "
	  << f.period << " " << f.offset << " " << f.width << std::endl;
    }
  }

  out << indent << "end" << std::endl << std::endl;

  return;
}


// Local Variables: ***
// mode:C++ ***
// tab-width: 8 ***
// c-basic-offset: 2 ***
// indent-tabs-mode: t ***
// End: ***
// ex: shiftwidth=2 tabstop=8
