#include "Synchro.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "InstrumentTrack.h"
// #include "lmms_math.h"
#include "embed.h"
#include "plugin_export.h"

namespace // constants used by both Synchro and SynchroView
{
	// TODO: 1000 is arbitrary, look at other instruments for glide
	// duration upper bound
	inline constexpr auto MAX_GLIDE_MS = 1000;
	inline constexpr auto MAX_ENV_MS = 1000;
}

namespace // math stuff
{}

namespace lmms
{

extern "C"
{
	PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *m, void *)
	{
		return new Synchro(static_cast<InstrumentTrack*>(m));
	}

	const Plugin::Descriptor PLUGIN_EXPORT SYNCHRO_PLUGIN_DESCRIPTOR =
	{
		LMMS_STRINGIFY(PLUGIN_NAME),
		"Synchro",
		QT_TRANSLATE_NOOP("PluginBrowser", "2-oscillator PM synth"),
		"Fawn <rubiefawn/at/gmail/dot/com>",
		0x0100,
		Plugin::Type::Instrument,
		new PluginPixmapLoader("logo"),
		nullptr, nullptr
	};
}

// TODO #7623: Use continuous stepsize instead of .00001f
Synchro::Synchro(InstrumentTrack *track) :
	Instrument(track, &SYNCHRO_PLUGIN_DESCRIPTOR, nullptr, Flag::IsSingleStreamed),
	m_oversamplingFactor(2),
	m_modulation(0, 0, 1, .00001f, this, tr("Modulation amount")),
	m_modulationScale(1, -2, 2, .25f, this, tr("Modulation scale")),
	m_glideTime(0, 0, MAX_GLIDE_MS, 1, this, tr("Glide time (ms)")),
	m_carrierDrive(1, 1, 7, .01f, this, tr("Carrier drive")),
	m_carrierSync(1, 1, 16, .01f, this, tr("Carrier sync")),
	m_carrierPulse(0, 0, 4, .01f, this, tr("Carrier pulse")),
	m_carrierAttack(5, 1, MAX_ENV_MS, 1, this, tr("Carrier attack")),
	m_carrierDecay(20, 1, MAX_ENV_MS, 1, this, tr("Carrier decay")),
	m_carrierSustain(1, 0, 1, .00001f, this, tr("Carrier sustain")),
	m_carrierRelease(5, 1, MAX_ENV_MS, 1, this, tr("Carrier release")),
	m_modulatorDrive(1, 1, 7, .01f, this, tr("Modulator drive")),
	m_modulatorSync(1, 1, 16, .01f, this, tr("Modulator sync")),
	m_modulatorPulse(0, 0, 4, .01f, this, tr("Modulator pulse")),
	m_modulatorGrit(0, 0, 1, .00001f, this, tr("Modulator grit")),
	m_modulatorOctave(-1, -4, 0, 1, this, tr("Modulator octave")),
	m_modulatorAttack(5, 1, MAX_ENV_MS, 1, this, tr("Modulator attack")),
	m_modulatorDecay(20, 1, MAX_ENV_MS, 1, this, tr("Modulator decay")),
	m_modulatorSustain(1, 0, 1, .00001f, this, tr("Modulator sustain")),
	m_modulatorRelease(5, 1, MAX_ENV_MS, 1, this, tr("Modulator release"))
{
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(effectiveSampleRateChanged()));
	// TODO connect oversampling slot once it has UI controls
	// connect(&m_oversamplingMultiplier, SIGNAL(dataChanged()), this, SLOT(effectiveSampleRateChanged()));
	connect(&m_modulation, SIGNAL(dataChanged()), this, SLOT(eitherOscChanged()));
	connect(&m_modulationScale, SIGNAL(dataChanged()), this, SLOT(eitherOscChanged()));

	connect(&m_carrierDrive, SIGNAL(dataChanged()), this, SLOT(carrierOscChanged()));
	connect(&m_carrierSync, SIGNAL(dataChanged()), this, SLOT(carrierOscChanged()));
	connect(&m_carrierPulse, SIGNAL(dataChanged()), this, SLOT(carrierOscChanged()));
	connect(&m_carrierAttack, SIGNAL(dataChanged()), this, SLOT(carrierEnvChanged()));
	connect(&m_carrierDecay, SIGNAL(dataChanged()), this, SLOT(carrierEnvChanged()));
	connect(&m_carrierSustain, SIGNAL(dataChanged()), this, SLOT(carrierEnvChanged()));
	connect(&m_carrierRelease, SIGNAL(dataChanged()), this, SLOT(carrierEnvChanged()));

	connect(&m_modulatorDrive, SIGNAL(dataChanged()), this, SLOT(modulatorOscChanged()));
	connect(&m_modulatorSync, SIGNAL(dataChanged()), this, SLOT(modulatorOscChanged()));
	connect(&m_modulatorPulse, SIGNAL(dataChanged()), this, SLOT(modulatorOscChanged()));
	connect(&m_modulatorOctave, SIGNAL(dataChanged()), this, SLOT(carrierOscChanged()));
	connect(&m_modulatorGrit, SIGNAL(dataChanged()), this, SLOT(modulatorOscChanged()));
	connect(&m_modulatorAttack, SIGNAL(dataChanged()), this, SLOT(modulatorEnvChanged()));
	connect(&m_modulatorDecay, SIGNAL(dataChanged()), this, SLOT(modulatorEnvChanged()));
	connect(&m_modulatorSustain, SIGNAL(dataChanged()), this, SLOT(modulatorEnvChanged()));
	connect(&m_modulatorRelease, SIGNAL(dataChanged()), this, SLOT(modulatorEnvChanged()));

	// carrierOscChanged();
	// carrierEnvChanged();
	// modulatorOscChanged();
	// modulatorEnvChanged();
	effectiveSampleRateChanged();
}

void Synchro::playNote(NotePlayHandle *nph, SampleFrame buf[])
{
	// TODO
}

void Synchro::play(SampleFrame buf[])
{
	// TODO
}

gui::PluginView *Synchro::instantiateView(QWidget *parent) { return new gui::SynchroView(this, parent); }

QString Synchro::nodeName() const { return SYNCHRO_PLUGIN_DESCRIPTOR.displayName; }

void Synchro::saveSettings(QDomDocument &doc, QDomElement &parent)
{
	// TODO
}

void Synchro::loadSettings(const QDomElement &thisElement)
{
	// TODO
}

void Synchro::effectiveSampleRateChanged()
{
	// TODO
}

void Synchro::carrierOscChanged()
{
	// TODO
}

void Synchro::modulatorOscChanged()
{
	// TODO
}

void Synchro::eitherOscChanged()
{
	// TODO
}

void Synchro::carrierEnvChanged()
{
	// TODO
}

void Synchro::modulatorEnvChanged()
{
	// TODO
}

void Synchro::eitherEnvChanged()
{
	// TODO
}

// TODO remove this in favor of sample-exact ValueBuffer once added
// see https://github.com/LMMS/lmms/pull/7297#issuecomment-2146162420
// Update: #7297 is cancelled in favor of the concept of a plan to
// rework sample-exactness as a whole. Revisit this when there is a
// nonzero amount of information about that.
inline float Synchro::sampleExact(FloatModel& it, int offset)
{
	return it.valueBuffer() ? it.valueBuffer()->value(offset) : it.value();
}

inline fpp_t Synchro::framesPerPeriod()
{
	return Engine::audioEngine()->framesPerPeriod() * m_oversamplingFactor;
}

} // namespace lmms


namespace lmms::gui
{

SynchroView::SynchroView(Instrument *instrument, QWidget *parent) :
	InstrumentViewFixedSize(instrument, parent)
{
	// TODO
}

void SynchroView::modelChanged()
{
	const auto m = castModel<Synchro>();
	m_modulation->setModel(&m->m_modulation);
	m_modulationScale->setModel(&m->m_modulationScale);
	m_glideTime->setModel(&m->m_glideTime);
	m_carrierDrive->setModel(&m->m_carrierDrive);
	m_carrierSync->setModel(&m->m_carrierSync);
	m_carrierPulse->setModel(&m->m_carrierPulse);
	m_carrierAttack->setModel(&m->m_carrierAttack);
	m_carrierDecay->setModel(&m->m_carrierDecay);
	m_carrierSustain->setModel(&m->m_carrierSustain);
	m_carrierRelease->setModel(&m->m_carrierRelease);
	m_modulatorDrive->setModel(&m->m_modulatorDrive);
	m_modulatorSync->setModel(&m->m_modulatorSync);
	m_modulatorPulse->setModel(&m->m_modulatorPulse);
	m_modulatorGrit->setModel(&m->m_modulatorGrit);
	m_modulatorOctave->setModel(&m->m_modulatorOctave);
	m_modulatorAttack->setModel(&m->m_modulatorAttack);
	m_modulatorDecay->setModel(&m->m_modulatorDecay);
	m_modulatorSustain->setModel(&m->m_modulatorSustain);
	m_modulatorRelease->setModel(&m->m_modulatorRelease);
}

} // namespace lmms::gui
