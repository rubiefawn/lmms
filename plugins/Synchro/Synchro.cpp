#include "Synchro.h"
#include "AudioEngine.h"
#include "Engine.h"
#include "InstrumentTrack.h"
#include "Plugin.h"
#include "embed.h"
#include "plugin_export.h"

namespace // math functions
{

using lmms::sample_t;

constexpr float reduce(float x) noexcept { return x - floor(x); }

template<unsigned int N>
constexpr float sqr_n_times(float x) noexcept
{
	return sqr_n_times<N - 1>(x * x);
}

template<>
constexpr float sqr_n_times<0>(float x) noexcept { return x; }

// In addition to -O3 (/O2 or /Ox for MSVC) One of the following
// compilation conditions must be met for this function to be optimized
// properly:
// - GCC 14 or higher
// - GCC 9 or higher using -ffinite-math-only
// - Clang 17 or higher
// - Clang 15 or higher using -ffinite-math-only
// - zig c++ 0.12 or higher
// - MSVC (version minimum untested) using /fp:fast
// If this is not the case, the compiler will insert some form of check
// after every single call to sqrt() to check for non-normal values.
template<unsigned int N>
constexpr float sqrt_n_times(float x) noexcept
{
	// TODO C++23 remove this macro nonsense and just use the
	// [[assume()]] attribute as soon as MSVC supports it
	#if __cplusplus >= 202302L && !defined(_MSC_VER)
	[[assume(x >= 0)]];
	#elif defined(_MSC_VER) && !defined(__clang__)
	__assume(x >= 0);
	#elif defined(__clang__) || defined(__GNUG__)
	if (x < 0) __builtin_unreachable();
	#endif
	return sqrt_n_times<N - 1>(sqrt(x));
}

template<>
constexpr float sqrt_n_times<0>(float x) noexcept { return x; }

// TODO C++20 use `requires` instead of
// static_assert() in the body
// template<unsigned int Q>
// constexpr float ln1(float x) noexcept
// {
// 	static_assert(Q > 2);
// 	return (sqrt_n_times<Q>(x) - 1.f) * (2 << Q);
// }

// TODO C++20 use `requires` instead of
// static_assert() in the body
template<unsigned int Q>
constexpr float exp1(float x) noexcept
{
	static_assert(Q > 2);
	constexpr float r = 1.f / (2 << Q);
	return sqr_n_times<Q>(1.f + x * r);
}

// TODO C++20 use `requires` instead of
// static_assert() in the body
template<unsigned int Q>
constexpr float pow1(float x, float y) noexcept
{
	static_assert(Q > 2);
	return sqr_n_times<Q>(1.f + y * sqrt_n_times<Q>(x) - y);
}

constexpr sample_t parabol(float phase) noexcept
{
	const float x = 4.f * phase - 2.f;
	return x * (2.f - abs(x));
}

constexpr sample_t triangle(float phase) noexcept
{
	phase += .25f;
	const float tri01 = abs(2.f * (phase - floor(phase + .5f)));
	return 2.f * tri01 - 1.f;
}

// This is a more readable version of `saturate()` that actually shows
// what the function is intended to do. The other version is an attempt
// to reduce cost by doing less computation. Microbenchmarks show that
// version is nearly 2x faster. Microbenchmarks aren't representative
// of real-world usage though so it may be worth benchmarking in LMMS.
[[maybe_unused]]
constexpr sample_t saturate_naive(sample_t x, float t, float drive, float pulse)
{
	const float attenuation = pow(1.f - t, pulse);
	return attenuation * tanh(drive * x) / tanh(drive);
}

constexpr sample_t saturate(sample_t x, float t, float drive, float pulse) noexcept
{
	// Q is the math approximation function quality. Increasing Q by 1
	// will add approximately 3 more multiplication instructions and
	// 1 more sqrt() call to this function.
	// This function is called a minimum of 88200 times per second so
	// don't increase the value of Q too much.
	constexpr auto Q = 7;
	drive *= 2.f;
	const float a = exp1<Q>(x * drive);
	const float b = exp1<Q>(drive);
	const float attenuation = pow1<Q>(1.f - t, pulse);
	return attenuation * (a - 1.f) * (b + 1.f) / ((a + 1.f) * (b - 1.f));
}

constexpr sample_t carrier(float phase, float drive, float sync, float pulse) noexcept
{
	const sample_t it = triangle(phase * sync);
	return saturate(it, reduce(phase), drive, pulse);
}

constexpr sample_t modulator(float phase, float drive, float sync, float pulse, float grit) noexcept
{
	const sample_t it = triangle(phase * sync);
	const sample_t gr = .50f * parabol(reduce(phase * 32.f))
		+ .03f * parabol(reduce(phase * 38.f));
	return saturate(it + gr * grit, reduce(phase), drive, pulse);
}

}

namespace lmms
{

extern "C"
{
	PLUGIN_EXPORT Plugin *lmms_plugin_main(Model *m, void *)
	{
		return new Synchro(static_cast<InstrumentTrack*>(m));
	}

	Plugin::Descriptor PLUGIN_EXPORT synchro_plugin_descriptor =
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

Synchro::Synchro(InstrumentTrack *track) :
	Instrument(track, &synchro_plugin_descriptor, nullptr, Flag::IsSingleStreamed),
	m_oversamplingMultiplier(2),
	m_modulation(0, 0, 1, .00001f, this, tr("modulation amount")),
	m_modulationScale(1, -2, 2, .25f, this, tr("modulation scale")),
	m_carrierDrive(1, 1, 7, 0.01f, this, tr("carrier drive")),
	m_carrierSync(1, 1, 16, 0.01f, this, tr("carrier sync")),
	m_carrierPulse(0, 0, 4, 0.01f, this, tr("carrier pulse")),
	m_modulatorDrive(2, 1, 7, 0.01f, this, tr("modulator drive")),
	m_modulatorSync(1, 1, 16, 0.01f, this, tr("modulator sync")),
	m_modulatorPulse(0, 0, 4, 0.01f, this, tr("modulator pulse")),
	m_modulatorGrit(0, 0, 1, 0.00001f, this, tr("modulator grit")),
	m_modulatorOctave(-1, -4, 0, 1, this, tr("octave ratio")),
	m_carrierWaveform(-1, 1, SYNCHRO_GRAPH_WIDTH, this),
	m_modulatorWaveform(-1, 1, SYNCHRO_GRAPH_WIDTH, this),
	m_resultingWaveform(-1, 1, SYNCHRO_GRAPH_WIDTH, this)
{
	connect(Engine::audioEngine(), SIGNAL(sampleRateChanged()), this, SLOT(effectiveSampleRateChanged()));
	// TODO connect oversampling slot once it has UI controls
	// connect(&m_oversamplingMultiplier, SIGNAL(dataChanged()), this, SLOT(effectiveSampleRateChanged()));
	connect(&m_carrierDrive, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrierSync, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_carrierPulse, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_modulatorDrive, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulatorSync, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulatorPulse, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulation, SIGNAL(dataChanged()), this, SLOT(eitherOscChanged()));
	connect(&m_modulatorOctave, SIGNAL(dataChanged()), this, SLOT(carrierChanged()));
	connect(&m_modulatorGrit, SIGNAL(dataChanged()), this, SLOT(modulatorChanged()));
	connect(&m_modulationScale, SIGNAL(dataChanged()), this, SLOT(eitherOscChanged()));

	carrierChanged();
	modulatorChanged();
	effectiveSampleRateChanged();	
}

void Synchro::play(SampleFrame *buf)
{
}

gui::PluginView *Synchro::instantiateView(QWidget *parent) { return new gui::SynchroView(this, parent); }

QString Synchro::nodeName() const { return synchro_plugin_descriptor.displayName; }

void Synchro::saveSettings(QDomDocument &doc, QDomElement &parent) {}
void Synchro::loadSettings(const QDomElement &thisElement) {}

void Synchro::effectiveSampleRateChanged()
{
	m_buf.resize(2 * framesPerPeriod());
}

void Synchro::carrierChanged()
{
	// While the pitch difference is for the modulator oscillator, it
	// is rendered in the carrier and resulting previews instead. This
	// is so that the modulation across the full period of the
	// modulator can be shown in the resulting preview, and so that the
	// carrier waveform preview is consistent with that.
	const float pitchDiff = exp2(-m_modulatorOctave.value());
	for (int i = 0; i < SYNCHRO_GRAPH_WIDTH; ++i)
	{
		m_carrierWaveform.setSampleAt(i, carrier(
			(float)i / SYNCHRO_GRAPH_WIDTH * pitchDiff,
			m_carrierDrive.value(),
			m_carrierSync.value(),
			m_carrierPulse.value()
		));
	}
	eitherOscChanged();
}

void Synchro::modulatorChanged()
{
	for (int i = 0; i < SYNCHRO_GRAPH_WIDTH; ++i)
	{
		m_modulatorWaveform.setSampleAt(i, modulator(
			(float)i / SYNCHRO_GRAPH_WIDTH,
			m_modulatorDrive.value(),
			m_modulatorSync.value(),
			m_modulatorPulse.value(),
			m_modulatorGrit.value() * GRIT_VISUAL_REDUCTION
		));
	}
	eitherOscChanged();
}

void Synchro::eitherOscChanged()
{
	// While the pitch difference is for the modulator oscillator, it
	// is rendered in the carrier and resulting previews instead. This
	// is so that the modulation across the full period of the
	// modulator can be shown in the resulting preview, and so that the
	// carrier waveform preview is consistent with that.
	const float pitchDiff = exp2(-m_modulatorOctave.value());
	for (int i = 0; i < SYNCHRO_GRAPH_WIDTH; ++i)
	{
		const float modulation = modulator(
			(float)i / SYNCHRO_GRAPH_WIDTH,
			m_modulatorDrive.value(),
			m_modulatorSync.value(),
			m_modulatorPulse.value(),
			m_modulatorGrit.value() * GRIT_VISUAL_REDUCTION
		) * m_modulationScale.value() * m_modulation.value();

		m_resultingWaveform.setSampleAt(i, carrier(
			(float)i / SYNCHRO_GRAPH_WIDTH * pitchDiff + modulation,
			m_carrierDrive.value(),
			m_carrierSync.value(),
			m_carrierPulse.value()
		));
	}
}

// TODO remove this in favor of sample-exact ValueBuffer once added
// see https://github.com/LMMS/lmms/pull/7297#issuecomment-2146162420
inline float Synchro::sampleExact(FloatModel& it, int offset)
{
	return it.valueBuffer() ? it.valueBuffer()->value(offset) : it.value();
}

inline fpp_t Synchro::framesPerPeriod()
{
	return Engine::audioEngine()->framesPerPeriod() * m_oversamplingMultiplier;
}

gui::SynchroView::SynchroView(Instrument *instrument, QWidget *parent) :
	InstrumentViewFixedSize(instrument, parent)
{
	setAutoFillBackground(true);
	QPalette pal;
	// TODO use svg background once svg support is complete
	pal.setBrush(backgroundRole(), PLUGIN_NAME::getIconPixmap("artwork"));
	setPalette(pal);

	constexpr int graph_w = SYNCHRO_GRAPH_WIDTH, graph_h = 77, graph_x = 18;
	#define SYNCHRO_GRAPH_INIT(IT) do {\
	IT->setAutoFillBackground(false);\
	IT->setEnabled(false); } while (0)
	
	m_carrierWaveform = new Graph(this, Graph::Style::LinearNonCyclic, graph_w, graph_h);
	m_carrierWaveform->setGraphColor(CYAN);
	m_carrierWaveform->move(graph_x, 165);
	SYNCHRO_GRAPH_INIT(m_carrierWaveform);

	m_modulatorWaveform = new Graph(this, Graph::Style::LinearNonCyclic, graph_w, graph_h);
	m_modulatorWaveform->setGraphColor(RED);
	m_modulatorWaveform->move(graph_x, 262);
	SYNCHRO_GRAPH_INIT(m_modulatorWaveform);

	m_resultingWaveform = new Graph(this, Graph::Style::LinearNonCyclic, graph_w, graph_h);
	m_resultingWaveform->setGraphColor(YELLOW);
	m_resultingWaveform->move(graph_x, 68);
	SYNCHRO_GRAPH_INIT(m_resultingWaveform);

	constexpr int knob_xy = -3; //HACK offset for knob outer ring
	constexpr int knob_x[] = { 220, 285, 350, 416 };
	constexpr int knob_y[] = { 86, 183, 280 };

	// TODO custom styled knobs that use the colors of their corresponding UI section
	m_modulation = new Knob(KnobType::Dark28, this);
	m_modulation->move(knob_x[0] + knob_xy, knob_y[0] + knob_xy);
	m_modulation->setHintText(tr("modulation amount"), "×"); // TODO make the UI show 0-100%

	m_modulationScale = new Knob(KnobType::Dark28, this);
	m_modulationScale->move(knob_x[1] + knob_xy, knob_y[0] + knob_xy);
	m_modulationScale->setHintText(tr("modulation scale"), "×"); // TODO make the UI show 0-100%

	m_carrierDrive = new Knob(KnobType::Dark28, this);
	m_carrierDrive->move(knob_x[0] + knob_xy, knob_y[1] + knob_xy);
	m_carrierDrive->setHintText(tr("carrier drive"), "×");

	m_carrierSync = new Knob(KnobType::Dark28, this);
	m_carrierSync->move(knob_x[1] + knob_xy, knob_y[1] + knob_xy);
	m_carrierSync->setHintText(tr("carrier sync"), "×");

	m_carrierPulse = new Knob(KnobType::Dark28, this);
	m_carrierPulse->move(knob_x[2] + knob_xy, knob_y[1] + knob_xy);
	m_carrierPulse->setHintText(tr("carrier pulse"), "^");

	m_modulatorOctave = new Knob(KnobType::Dark28, this);
	m_modulatorOctave->move(knob_x[3] + knob_xy, knob_y[1] + knob_xy);
	m_modulatorOctave->setHintText(tr("octave ratio"), "octaves");

	m_modulatorDrive = new Knob(KnobType::Dark28, this);
	m_modulatorDrive->move(knob_x[0] + knob_xy, knob_y[2] + knob_xy);
	m_modulatorDrive->setHintText(tr("modulator drive"), "×");

	m_modulatorSync = new Knob(KnobType::Dark28, this);
	m_modulatorSync->move(knob_x[1] + knob_xy, knob_y[2] + knob_xy);
	m_modulatorSync->setHintText(tr("modulator sync"), "×");

	m_modulatorPulse = new Knob(KnobType::Dark28, this);
	m_modulatorPulse->move(knob_x[2] + knob_xy, knob_y[2] + knob_xy);
	m_modulatorPulse->setHintText(tr("modulator pulse"), "^");

	m_modulatorGrit = new Knob(KnobType::Dark28, this);
	m_modulatorGrit->move(knob_x[3] + knob_xy, knob_y[2] + knob_xy);
	m_modulatorGrit->setHintText(tr("harmonics"), "×"); // TODO make the UI show 0-100%
}

void gui::SynchroView::modelChanged()
{
	Synchro *model = castModel<Synchro>();	
	m_carrierWaveform->setModel(&model->m_carrierWaveform);
	m_modulatorWaveform->setModel(&model->m_modulatorWaveform);
	m_resultingWaveform->setModel(&model->m_resultingWaveform);
	m_modulation->setModel(&model->m_modulation);
	m_modulationScale->setModel(&model->m_modulationScale);
	m_carrierDrive->setModel(&model->m_carrierDrive);
	m_carrierSync->setModel(&model->m_carrierSync);
	m_modulatorOctave->setModel(&model->m_modulatorOctave);
	m_carrierPulse->setModel(&model->m_carrierPulse);
	m_modulatorDrive->setModel(&model->m_modulatorDrive);
	m_modulatorSync->setModel(&model->m_modulatorSync);
	m_modulatorPulse->setModel(&model->m_modulatorPulse);
	m_modulatorGrit->setModel(&model->m_modulatorGrit);
}

} // namespace lmms
