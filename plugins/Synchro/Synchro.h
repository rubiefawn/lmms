#ifndef LMMS_SYNCHRO_H
#define LMMS_SYNCHRO_H

#include "AutomatableModel.h"
#include "Instrument.h"
#include "InstrumentView.h"
#include "Knob.h"
namespace lmms
{

namespace gui { class SynchroView; }

class Synchro : public Instrument
{
Q_OBJECT
friend class gui::SynchroView;
public:
	Synchro(InstrumentTrack *track);
	void playNote(NotePlayHandle *nph, SampleFrame buf[]) override;
	void play(SampleFrame buf[]) override;
	gui::PluginView *instantiateView(QWidget *parent) override;
	QString nodeName() const override;
	void saveSettings(QDomDocument &doc, QDomElement &parent) override;
	void loadSettings(const QDomElement &thisElement) override;
protected slots:
	void effectiveSampleRateChanged();
	void carrierOscChanged();
	void modulatorOscChanged();
	void eitherOscChanged();
	void carrierEnvChanged();
	void modulatorEnvChanged();
	void eitherEnvChanged();
private:
	inline float sampleExact(FloatModel& it, int offset);
	inline fpp_t framesPerPeriod();
	std::array<NotePlayHandle*, 2> m_nphs; // Current and previous note
	std::vector<sample_t> m_buf;
	int m_oversamplingFactor;
	FloatModel m_modulation;
	FloatModel m_modulationScale;
	FloatModel m_glideTime;
	FloatModel m_carrierDrive;
	FloatModel m_carrierSync;
	FloatModel m_carrierPulse;
	FloatModel m_carrierAttack;
	FloatModel m_carrierDecay;
	FloatModel m_carrierSustain;
	FloatModel m_carrierRelease;
	FloatModel m_modulatorDrive;
	FloatModel m_modulatorSync;
	FloatModel m_modulatorPulse;
	FloatModel m_modulatorGrit;
	FloatModel m_modulatorOctave;
	FloatModel m_modulatorAttack;
	FloatModel m_modulatorDecay;
	FloatModel m_modulatorSustain;
	FloatModel m_modulatorRelease;
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
	// #ffb900
	static constexpr QColor YELLOW { QColor::Rgb, 0xff * 0x101, 0xff * 0x101 , 0xb9 * 0x101, 0x00 * 0x101 };
	// #0dccda
	static constexpr QColor CYAN   { QColor::Rgb, 0xff * 0x101, 0x0d * 0x101 , 0xcc * 0x101, 0xda * 0x101 };
	// #f65b74
	static constexpr QColor RED    { QColor::Rgb, 0xff * 0x101, 0xf6 * 0x101 , 0x5b * 0x101, 0x74 * 0x101 };
	// Rendering the harmonics at full strength makes for a very noisy
	// waveform preview, so the harmonics are reduced when displaying
	// the waveform preview
	static constexpr float GRIT_VISUAL_REDUCTION = .5f;
	Knob *m_modulation;
	Knob *m_modulationScale;
	Knob *m_glideTime;
	Knob *m_carrierDrive;
	Knob *m_carrierSync;
	Knob *m_carrierPulse;
	Knob *m_carrierAttack;
	Knob *m_carrierDecay;
	Knob *m_carrierSustain;
	Knob *m_carrierRelease;
	Knob *m_modulatorDrive;
	Knob *m_modulatorSync;
	Knob *m_modulatorPulse;
	Knob *m_modulatorGrit;
	Knob *m_modulatorOctave;
	Knob *m_modulatorAttack;
	Knob *m_modulatorDecay;
	Knob *m_modulatorSustain;
	Knob *m_modulatorRelease;
};

} // namespace lmms
#endif
