/*
 * panning.h - declaration of some types, concerning the
 *             panning of a note
 *
 * Copyright (c) 2004-2014 Tobias Doerffel <tobydox/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program (see COPYING); if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 */

#ifndef LMMS_PANNING_H
#define LMMS_PANNING_H

#include "LmmsTypes.h"
#include "Midi.h"
#include "volume.h"

#include <cmath>

namespace lmms
{

inline constexpr panning_t PanningRight   = 100;
inline constexpr panning_t PanningLeft    = -PanningRight;
inline constexpr panning_t PanningCenter  = 0;
inline constexpr panning_t DefaultPanning = PanningCenter;


// TODO C++23: constexpr since std::abs() will be constexpr
inline StereoVolumeVector panningToVolumeVector(panning_t pan, float amplitude = 1.0f) noexcept
{
	StereoVolumeVector v = { amplitude, amplitude };
	v[pan >= PanningCenter ? 0 : 1] *= 1.0f - std::abs(pan * 0.01f);
	return v;
}


constexpr int panningToMidi(panning_t pan) noexcept
{
	constexpr auto PanningDiff = static_cast<float>(PanningRight - PanningLeft);
	constexpr auto MidiPanningDiff = static_cast<float>(MidiMaxPanning - MidiMinPanning);
	constexpr auto PanningDiffRatio = MidiPanningDiff / PanningDiff;
	return MidiMinPanning + static_cast<int>((pan - PanningLeft) * PanningDiffRatio);
}

} // namespace lmms

#endif // LMMS_PANNING_H
