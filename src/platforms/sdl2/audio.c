#include "gb.h"
#include "main.h"
#include "audio.h"
#include <util.h>
#include <mgb.h>
#include <stddef.h>
#include <stdint.h>
#include <assert.h>


static SDL_AudioDeviceID audio_device = 0;
static SDL_AudioSpec audio_spec = {0};
static SDL_AudioStream* audio_stream = NULL;
static uint32_t elapsed_cycles = 0;
static int volume = VOLUME;


static void mix_audio_u8(uint8_t* dst, const uint8_t* src, size_t len, int vol)
{
    assert(AUDIO_FORMAT == AUDIO_U8);

    Uint8 src_sample;
    #define ADJUST_VOLUME_U8(s, v)  (s = (((s-128)*v)/SDL_MIX_MAXVOLUME)+128)
    while (len--) {
        src_sample = *src;
        ADJUST_VOLUME_U8(src_sample, vol);
        *dst = *dst + src_sample;
        // *dst = mix8[*dst + src_sample];
        ++dst;
        ++src;
    }
    #undef ADJUST_VOLUME_U8
}

static void core_auido_callback(void* user, struct GB_ApuCallbackData* data)
{
    UNUSED(user);

    if (!audio_device || !audio_stream)
    {
        return;
    }

    const uint8_t unmixed_audio[2] =
    {
        // | 0x80 because i think the range for u8 audio is 128-255
        [0] = ((data->ch1[0] + data->ch2[0] + data->ch3[0] + data->ch4[0]) / 4) | 0x80,
        [1] = ((data->ch1[1] + data->ch2[1] + data->ch3[1] + data->ch4[1]) / 4) | 0x80,
    };

    uint8_t mixed_audio[2] = {0};

    // sdl impl for u8 is broken, the mix8 table makes zero sense
    if (AUDIO_FORMAT == AUDIO_U8)
    {
        mix_audio_u8(mixed_audio, unmixed_audio, sizeof(mixed_audio), volume);
    }
    else
    {
        SDL_MixAudioFormat(mixed_audio, unmixed_audio, AUDIO_FORMAT, sizeof(mixed_audio), volume);
    }

    SDL_AudioStreamPut(audio_stream, mixed_audio, sizeof(mixed_audio));
}

static void sdl2_audio_callback(void* user, uint8_t* data, int len)
{
    emu_t* emu = SDL_static_cast(emu_t*, user);

    if (emu->rewinding || !emu->running || !mgb_has_rom() || get_menu_type() != MenuType_ROM)
    {
        SDL_memset(data, audio_spec.silence, len);
        return;
    }

    const uint32_t tcyles = (GB_CPU_CYCLES / audio_spec.freq) * len;

    audio_update_core_sample_rate(emu);

    lock_core();
        while (SDL_AudioStreamAvailable(audio_stream) < len)
        {
            GB_run(&emu->gb, tcyles);
            elapsed_cycles += tcyles;
        }
    unlock_core();

    if (SDL_AudioStreamGet(audio_stream, data, len) == -1)
    {
        log_error("SDL_AudioStreamGet(): %s\n", SDL_GetError());
    }
}

bool audio_init(emu_t* emu)
{
    // check if we are already init!
    if (audio_device)
    {
        audio_exit();
    }

    SDL_AudioSpec wanted_spec =
    {
        .freq = 0, // we set this later on
        .format = AUDIO_U8,
        .channels = CHANNELS,
        .samples = SAMPLES,
        .callback = sdl2_audio_callback,
        .userdata = emu,
    };

    // loop through list of frequencies until we find one that's supported!
    // start from high to low
    static const int frequencies[] =
    {
        #ifdef __SWITCH__
        AUDIO_FREQ_48k,
        #endif
        AUDIO_FREQ_192k, // 192000
        AUDIO_FREQ_96k,  // 96000
        AUDIO_FREQ_48k,  // 48000
        AUDIO_FREQ_44k,  // 44100
        AUDIO_FREQ_22k,  // 22050
        16000,
        AUDIO_FREQ_11k,  // 11025
    };

    for (size_t i = 0; i < SDL_arraysize(frequencies); i++)
    {
        wanted_spec.freq = frequencies[i];
        audio_device = SDL_OpenAudioDevice(NULL, 0, &wanted_spec, &audio_spec, AUDIO_FLAGS);

        // exit loop if we have opened audio!
        if (audio_device)
        {
            break;
        }

        log_info("[SDL-AUDIO] failed to open audio device with freq %d\n", frequencies[i]);
    }

    // we failed to open any audio device...
    if (audio_device == 0)
    {
        goto fail;
    }

    log_info("[SDL-AUDIO] driver: %s\n", SDL_GetCurrentAudioDriver());
    log_info("[SDL-AUDIO] format: %d\n", audio_spec.format);
    log_info("[SDL-AUDIO] freq: %d\n", audio_spec.freq);
    log_info("[SDL-AUDIO] channels: %d\n", audio_spec.channels);
    log_info("[SDL-AUDIO] samples: %d\n", audio_spec.samples);
    log_info("[SDL-AUDIO] size: %d\n", audio_spec.size);
    log_info("[SDL-AUDIO] silence: %u\n", audio_spec.silence);
    log_info("\n");

    const int core_sample_rate = audio_get_core_sample_rate(emu);

    audio_stream = SDL_NewAudioStream(AUDIO_FORMAT, CHANNELS, core_sample_rate, audio_spec.format, audio_spec.channels, audio_spec.freq);
    if (!audio_stream)
    {
        goto fail;
    }

    GB_set_apu_callback(&emu->gb, core_auido_callback, emu, core_sample_rate);
    audio_update_core_sample_rate(emu);
    SDL_PauseAudioDevice(audio_device, 0);

    return true;

fail:
    log_error("[SDL-AUDIO] failed to open: %s\n", SDL_GetError());
    return false;
}

void audio_exit(void)
{
    if (audio_device)
    {
        SDL_CloseAudioDevice(audio_device);
        audio_device = 0;
    }

    if (audio_stream)
    {
        SDL_FreeAudioStream(audio_stream);
        audio_stream = NULL;
    }
}

void audio_lock(void)
{
    SDL_LockAudioDevice(audio_device);
}

void audio_unlock(void)
{
    SDL_UnlockAudioDevice(audio_device);
}

uint32_t audio_get_elapsed_cycles(void)
{
    const uint32_t cycles = elapsed_cycles;
    elapsed_cycles = 0;
    return cycles;
}

int audio_get_core_sample_rate(const emu_t* emu)
{
    UNUSED(emu);

    // for some reason, when use sdl_stream, it stretches the audio too much
    // if downsampling, meaning we end up with a lot more samples
    // so we end up running the core less often, so it runs slow...
    // for now, use the same sample rate the audio drive wants
#if 0
    const int freq = 1053360 * 2;
#else
    const int freq = audio_spec.freq * 2;
#endif

    return SDL_max(freq, audio_spec.freq);
}

void audio_update_core_sample_rate(emu_t* emu)
{
    int sample_rate = audio_get_core_sample_rate(emu);

    // this was meant to reduce the sample rate slightly if we drifted
    // too far forward.
    // this didn't really work...
    // a better option would be a combition of lowering the sample rate
    // and skipping samples.

    // if (audio_stream)
    // {
    //     const int half_frame_of_audio = (sample_rate / audio_spec.samples) * 30;

    //     if (SDL_AudioStreamAvailable(audio_stream) > half_frame_of_audio)
    //     {
    //         // adjust sample rate so that we don't go too far forward!
    //         const double max_delta = 0.010;
    //         // const double max_delta = 0.005;
    //         sample_rate -= (double)sample_rate * max_delta;
    //         // log_info("[AUDIO] audio is too far ahead: %d, updating sample rate to: %d\n", SDL_AudioStreamAvailable(audio_stream), sample_rate);
    //     }
    // }

    // adjust the sample rate based on the playback speed
    if (emu->speed > 1)
    {
        sample_rate /= emu->speed;
    }
    else if (emu->speed < -1)
    {
        sample_rate *= emu->speed * -1;
    }

    lock_core();
        GB_set_apu_freq(&emu->gb, sample_rate);
    unlock_core();
}

SDL_AudioSpec audio_get_spec(void)
{
    return audio_spec;
}
