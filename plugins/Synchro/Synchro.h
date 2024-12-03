// #pragma once
#ifndef LMMS_SYNCHRO_H
#define LMMS_SYNCHRO_H
#include "AutomatableModel.h"
#include "Graph.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"

namespace
{
	// Rendering the "grit" parameter with its true value makes a
	// complete visual mess in the graph
	constexpr float GRIT_VISUAL_REDUCTION = 0.5;
	constexpr int SYNCHRO_GRAPH_WIDTH = 168;
	constexpr int SYNCHRO_DEFAULT_MATH_QUALITY = 7;
}

namespace lmms
{

namespace gui { class SynchroView; }

class Synchro : public Instrument
{
Q_OBJECT	
public:
	Synchro(InstrumentTrack *track);
	void play(SampleFrame *buf) override;
	gui::PluginView *instantiateView(QWidget *parent) override;
	QString nodeName() const override;
	void saveSettings(QDomDocument &doc, QDomElement &parent) override;
	void loadSettings(const QDomElement &thisElement) override;
protected slots:
	void effectiveSampleRateChanged();
	void carrierChanged();
	void modulatorChanged();
	void eitherOscChanged();
private:
	inline float sampleExact(FloatModel& it, int offset);
	inline fpp_t framesPerPeriod();
	std::vector<sample_t> m_buf; // I want (*sample_t)[2][fpp], but I'm not sure what the safer C++ would be
	int m_oversamplingMultiplier;
	FloatModel m_modulation;
	FloatModel m_modulationScale;
	FloatModel m_carrierDrive;
	FloatModel m_carrierSync;
	FloatModel m_carrierPulse;
	FloatModel m_modulatorDrive;
	FloatModel m_modulatorSync;
	FloatModel m_modulatorPulse;
	FloatModel m_modulatorGrit;
	FloatModel m_modulatorOctave;
	graphModel m_carrierWaveform;
	graphModel m_modulatorWaveform;
	graphModel m_resultingWaveform;
friend class gui::SynchroView;
};

class gui::SynchroView : public InstrumentViewFixedSize
{
Q_OBJECT
public:
	SynchroView(Instrument *instrument, QWidget *parent);
	QSize sizeHint() const override { return QSize(480, 360); }
protected slots:
	void modelChanged() override;
private:
	static constexpr QColor YELLOW { QColor::Rgb, 0xff * 0x101, 0xff * 0x101 , 0xb9 * 0x101, 0x00 * 0x101 }; // #ffb900
	static constexpr QColor CYAN   { QColor::Rgb, 0xff * 0x101, 0x0d * 0x101 , 0xcc * 0x101, 0xda * 0x101 }; // #0dccda
	static constexpr QColor RED    { QColor::Rgb, 0xff * 0x101, 0xf6 * 0x101 , 0x5b * 0x101, 0x74 * 0x101 }; // #f65b74

	// int m_oversamplingMultiplier; //TODO
	Knob *m_modulation;
	Knob *m_modulationScale;
	Knob *m_carrierDrive;
	Knob *m_carrierSync;
	Knob *m_carrierPulse;
	Knob *m_modulatorDrive;
	Knob *m_modulatorSync;
	Knob *m_modulatorPulse;
	Knob *m_modulatorGrit;
	Knob *m_modulatorOctave;
	Graph *m_carrierWaveform;
	Graph *m_modulatorWaveform;
	Graph *m_resultingWaveform;
};

} // namespace lmms
#endif
