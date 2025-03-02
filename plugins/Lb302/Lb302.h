/*
 * Lb302.h - declaration of class Lb302 which is a bass synth attempting to
 *           emulate the Roland TB-303 bass synth
 *
 * Copyright (c) 2006-2008 Paul Giblock <pgib/at/users.sourceforge.net>
 *
 * This file is part of LMMS - https://lmms.io
 *
 * Lb302FilterIIR2 is based on the gsyn filter code by Andy Sloane.
 *
 * Lb302Filter3Pole is based on the TB-303 instrument written by
 *   Josep M Comajuncosas for the CSounds library
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


#ifndef LB302_H
#define LB302_H

#include "DspEffectLibrary.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "NotePlayHandle.h"
#include <QMutex>

namespace lmms
{


namespace gui
{
class automatableButtonGroup;
class Knob;
class Lb302SynthView;
class LedCheckBox;
}


struct Lb302FilterKnobState { float cutoff, reso, envmod, envdecay, dist; };


class Lb302Filter
{
public:
	Lb302Filter(Lb302FilterKnobState *p_fs);
	virtual ~Lb302Filter() = default;

	virtual void recalc();
	virtual void envRecalc();
	virtual float process(float samp) = 0;
	virtual void playNote();

protected:
	Lb302FilterKnobState *fs;

	// Filter Decay
	// c0 = e1 on retrigger; c0 *= ed every sample; cutoff = e0 + c0
	// e0 and e1 for interpolation
	float vcf_c0 = 0.f,	vcf_e0 = 0.f, vcf_e1 = 0.f;
	// Resonance coefficient [0.30, 9.54]
	float vcf_rescoeff;
};


class Lb302FilterIIR2 : public Lb302Filter
{
public:
	Lb302FilterIIR2(Lb302FilterKnobState *p_fs);
	~Lb302FilterIIR2() override;

	void recalc() override;
	void envRecalc() override;
	float process(float samp) override;

protected:
	// d1 and d2 are added back into the sample with vcf_a and b as
	// coefficients. IIR2 resonance loop.
	float vcf_d1 = 0, vcf_d2 = 0;

	// IIR2 Coefficients for mixing dry and delay.
	// Mixing coefficients for the final sound.
	float vcf_a = 0, vcf_b = 0, vcf_c = 1;

	DspEffectLibrary::Distortion *m_dist;
};


class Lb302Filter3Pole : public Lb302Filter
{
public:
	Lb302Filter3Pole(Lb302FilterKnobState *p_fs);

	//virtual void recalc();
	void envRecalc() override;
	void recalc() override;
	float process(float samp) override;

protected:
	float kfcn, kp, kp1, kp1h, kres;
	float ay1, ay2, aout, lastin, value;
};


class Lb302Synth : public Instrument
{
	Q_OBJECT
	friend class gui::Lb302SynthView;
public:
	Lb302Synth(InstrumentTrack *track);
	~Lb302Synth() override;

	void play(SampleFrame *outbuf) override;
	void playNote(NotePlayHandle *nph, SampleFrame*) override;
	void deleteNotePluginData(NotePlayHandle *nph) override;

	void saveSettings(QDomDocument& doc, QDomElement& thisElement) override;
	void loadSettings(const QDomElement& thisElement) override;

	QString nodeName() const override;

	gui::PluginView* instantiateView(QWidget *parent) override;

public slots:
	void filterChanged();
	void db24Toggled();

private:
	enum class VcaMode { Attack, Decay, Idle, NeverPlayed };
	enum class VcoShape { Sawtooth, Square, Triangle, Moog,
		RoundSquare, Sine, Exponential, WhiteNoise, BLSawtooth,
		BLSquare, BLTriangle, BLMoog };

	void processNote(NotePlayHandle *nph);
	void initNote(float p_vco_inc, bool dead);
	void initSlide();
	void recalcFilter();
	void process(SampleFrame* outbuf, const std::size_t size);

	FloatModel vcf_cut_knob, vcf_res_knob, vcf_mod_knob, vcf_dec_knob;
	FloatModel vco_fine_detune_knob;
	FloatModel dist_knob;
	IntModel wave_shape;
	FloatModel slide_dec_knob;

	BoolModel slideToggle;
	BoolModel accentToggle;
	BoolModel deadToggle;
	BoolModel db24Toggle;

	// Oscillator
	float vco_inc = 0, // Sample increment for the frequency. Creates Sawtooth.
	      vco_k   = 0, // Raw oscillator sample [-0.5,0.5]
	      vco_c   = 0; // Raw oscillator sample [-0.5,0.5]

	float vco_slide     = 0, //* Current value of slide exponential curve. Nonzero=sliding
	      vco_slideinc  = 0, //* Slide base to use in next node. Nonzero=slide next note
	      vco_slidebase = 0; //* The base vco_inc while sliding.

	VcoShape vco_shape = VcoShape::BLSawtooth;

	// Filters (just keep both loaded and switch)
	std::array<Lb302Filter*, 2> vcfs;

	// User settings
	Lb302FilterKnobState fs = {};
	QAtomicPointer<Lb302Filter> vcf;

	size_t release_frame = 0;

	// More States
	int vcf_envpos; // Update counter. Updates when >= ENVINC

	float vca_attack = 1.f - 0.96406088f, // Amp attack
	      vca_a0     = 0.5f,              // Initial amplifier coefficient
	      vca_a      = 0.f;               // Amplifier coefficient.

	// Envelope State
	VcaMode vca_mode = VcaMode::NeverPlayed;

	// My hacks
	int sample_cnt = 0;
	int catch_decay = 0;

	bool new_freq = false;
	float true_freq;

	NotePlayHandle *m_playingNote;
	NotePlayHandleList m_notes;
	QMutex m_notesMutex;
};


namespace gui
{


class Lb302SynthView : public InstrumentViewFixedSize
{
	Q_OBJECT
public:
	Lb302SynthView(Instrument *instrument, QWidget *parent);
	~Lb302SynthView() override = default;

private:
	void modelChanged() override;

	Knob *m_vcfCutKnob, *m_vcfResKnob, *m_vcfDecKnob, *m_vcfModKnob;

	Knob *m_distKnob;
	Knob *m_slideDecKnob;
	automatableButtonGroup *m_waveBtnGrp;

	LedCheckBox *m_slideToggle;
	// LedCheckBox *m_accentToggle; // TODO implement accents
	LedCheckBox *m_deadToggle;
	LedCheckBox *m_db24Toggle;
};


} // namespace gui

} // namespace lmms

#endif // LB302_H
