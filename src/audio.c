/* Mix the streamed soundtrack and procedural effects on one PSP audio channel. */
#include "game.h"
#ifdef PLATFORM_PSP
#include <limits.h>
#include <pspaudio.h>
#include <pspkernel.h>
#include <pspmp3.h>
#include <psputility.h>
#include <stdio.h>
#include <string.h>

enum { BLOCK_FRAMES = 512 };
static const unsigned mp3End = 0x80671402u;
static volatile int audioRunning, readEvent, writeEvent;
static volatile unsigned musicFrames, audibleFrames, musicLoops;
static int events[16], channel = -1, thread = -1;
static const char *launcherPath;
static short buffer[BLOCK_FRAMES * 2] __attribute__((aligned(64)));
static unsigned char mp3Input[16384] __attribute__((aligned(64)));
static unsigned char mp3Output[9216] __attribute__((aligned(64)));
static char fileBuffer[16384];

typedef struct {
    FILE *file;
    long offset, length, next;
    int avLoaded, mp3Loaded, resource, handle;
    short *pcm;
    int frames;
} Music;

static void music_close(Music *m) {
    if (m->handle >= 0) sceMp3ReleaseMp3Handle(m->handle);
    if (m->resource) sceMp3TermResource();
    if (m->mp3Loaded) sceUtilityUnloadModule(PSP_MODULE_AV_MP3);
    if (m->avLoaded) sceUtilityUnloadModule(PSP_MODULE_AV_AVCODEC);
    if (m->file) fclose(m->file);
    *m = (Music){.handle = -1};
}

static int music_refill(Music *m) {
    unsigned char *dst;
    SceInt32 space, position;
    if (sceMp3GetInfoToAddStreamData(m->handle, &dst, &space, &position) < 0 ||
        position < 0 || position > m->length)
        return 0;
    if (space <= 0 || position == m->length)
        return 1;
    int wrapping = position == 0 && m->next == m->length;
    if (position != m->next && fseek(m->file, m->offset + position, SEEK_SET) != 0)
        return 0;
    int remaining = m->length - position;
    int count = space < remaining ? space : remaining;
    if (fread(dst, 1, count, m->file) != (size_t)count ||
        sceMp3NotifyAddStreamData(m->handle, count) < 0)
        return 0;
    m->next = position + count;
    if (wrapping) musicLoops++;
    return 1;
}

static int music_open(Music *m) {
    m->file = fopen(launcherPath, "rb");
    if (!m->file)
        m->file = fopen("EBOOT.PBP", "rb");
    if (!m->file)
        return 0;

    setvbuf(m->file, fileBuffer, _IOFBF, sizeof(fileBuffer));
    /* DATA.PSAR contains the complete MP3. Stream only that section from disk. */
    uint32_t header[10];
    if (fread(header, sizeof(header), 1, m->file) != 1 || header[0] != 0x50425000u ||
        header[1] != 0x10000u || header[8] < sizeof(header) || header[9] <= header[8] ||
        header[9] > LONG_MAX || fseek(m->file, 0, SEEK_END) != 0)
        return 0;
    m->offset = header[9];
    long end = ftell(m->file);
    if (end <= m->offset)
        return 0;
    m->length = end - m->offset;
    m->next = -1;

    if (sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC) < 0)
        return 0;
    m->avLoaded = 1;
    if (sceUtilityLoadModule(PSP_MODULE_AV_MP3) < 0)
        return 0;
    m->mp3Loaded = 1;
    if (sceMp3InitResource() < 0)
        return 0;
    m->resource = 1;
    SceMp3InitArg args = {
        .mp3StreamStart = 0, .mp3StreamEnd = m->length,
        .mp3Buf = mp3Input, .mp3BufSize = sizeof(mp3Input),
        .pcmBuf = mp3Output, .pcmBufSize = sizeof(mp3Output),
    };
    m->handle = sceMp3ReserveMp3Handle(&args);
    return m->handle >= 0 && music_refill(m) && sceMp3Init(m->handle) >= 0 &&
           sceMp3GetSamplingRate(m->handle) == 44100 && sceMp3GetMp3ChannelNum(m->handle) == 2 &&
           sceMp3SetLoopNum(m->handle, -1) >= 0;
}

static int music_decode(Music *m) {
    for (int attempt = 0; attempt < 2; attempt++) {
        int need = sceMp3CheckStreamDataNeeded(m->handle);
        if (need < 0 || (need && !music_refill(m)))
            return -1;
        int bytes = sceMp3Decode(m->handle, &m->pcm);
        if (bytes > 0) {
            if (!m->pcm || bytes > (int)sizeof(mp3Output) / 2 || bytes % (2 * sizeof(short)))
                return -1;
            m->frames = bytes / (2 * sizeof(short));
            return 1;
        }
        if (bytes < 0 && (unsigned)bytes != mp3End)
            return -1;
    }
    return 0;
}

static int music_mix(Music *m, int *audible) {
    int frames = 0;
    memset(buffer, 0, sizeof(buffer));
    while (frames < BLOCK_FRAMES && m->handle >= 0) {
        if (!m->frames) {
            int decoded = music_decode(m);
            if (decoded < 0) music_close(m);
            if (decoded <= 0) break;
        }
        int count = BLOCK_FRAMES - frames;
        if (count > m->frames) count = m->frames;
        for (int i = 0; i < count * 2; i++) {
            short sample = m->pcm[i] / 2;
            buffer[frames * 2 + i] = sample;
            *audible |= sample;
        }
        m->pcm += count * 2;
        m->frames -= count;
        frames += count;
    }
    return frames;
}

typedef struct {
    int kind, left, length;
    unsigned phase, step, noise;
} Voice;
static Voice voices[4];
void audio_event(int kind) {
    int next = (writeEvent + 1) & 15;
    if (next == readEvent)
        return;
    events[writeEvent] = kind;
    writeEvent = next;
}
static int sound_thread(SceSize n, void *p) {
    (void)n;
    (void)p;
    Music music = {.handle = -1};
    if (!music_open(&music))
        music_close(&music);
    unsigned serial = 0;
    while (audioRunning) {
        int audible = 0;
        int played = music_mix(&music, &audible);
        while (readEvent != writeEvent) {
            int kind = events[readEvent];
            readEvent = (readEvent + 1) & 15;
            Voice *v = &voices[serial++ % 4];
            v->kind = kind;
            v->length = v->left = kind == 1 ? 5500 : kind == 2 ? 7000 : kind == 3 ? 10000 : 9000;
            v->phase = 0;
            v->step = kind == 2 ? 190 : kind == 3 ? 2200 : kind == 4 ? 110 : 700;
            v->noise = 73819 + serial * 93;
        }
        for (int i = 0; i < BLOCK_FRAMES; i++) {
            int mixed = 0;
            for (int j = 0; j < 4; j++) {
                Voice *v = &voices[j];
                if (v->left <= 0)
                    continue;
                v->noise ^= v->noise << 13;
                v->noise ^= v->noise >> 17;
                v->noise ^= v->noise << 5;
                v->phase += v->step;
                int wave = (int)(v->phase & 65535);
                wave = wave < 32768 ? wave : -wave + 65535;
                wave = (wave - 16384) / 3;
                int noise = (int)(v->noise & 16383) - 8192;
                int sample = v->kind == 1   ? noise / 3
                             : v->kind == 2 ? (wave * 2 + noise) / 2
                             : v->kind == 3 ? wave + noise / 4
                                            : noise;
                int amp = v->left * 256 / v->length;
                mixed += sample * amp / 256;
                v->left--;
            }
            for (int side = 0; side < 2; side++) {
                int sample = mixed + buffer[2 * i + side];
                buffer[2 * i + side] = sample > 30000 ? 30000 : sample < -30000 ? -30000 : sample;
            }
        }
        if (sceAudioOutputBlocking(channel, 0x5800, buffer) >= 0) {
            musicFrames += played;
            if (audible) audibleFrames += played;
        } else {
            sceKernelDelayThread(10000);
        }
    }
    music_close(&music);
    return 0;
}
void audio_init(const char *eboot) {
    launcherPath = eboot && *eboot ? eboot : "EBOOT.PBP";
    channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, BLOCK_FRAMES, PSP_AUDIO_FORMAT_STEREO);
    if (channel < 0)
        return;
    audioRunning = 1;
    thread = sceKernelCreateThread("Dustbound sound", sound_thread, 0x18, 0x4000, 0, 0);
    if (thread >= 0 && sceKernelStartThread(thread, 0, 0) >= 0)
        return;
    audioRunning = 0;
    if (thread >= 0) sceKernelDeleteThread(thread);
    sceAudioChRelease(channel);
    channel = thread = -1;
}
void audio_shutdown(void) {
    audioRunning = 0;
    if (thread >= 0) {
        sceKernelWaitThreadEnd(thread, 0);
        sceKernelDeleteThread(thread);
    }
    if (channel >= 0)
        sceAudioChRelease(channel);
}
void audio_debug(unsigned stats[3]) {
    stats[0] = musicFrames;
    stats[1] = audibleFrames;
    stats[2] = musicLoops;
}
#else
void audio_event(int kind) { (void)kind; }
void audio_init(const char *eboot) { (void)eboot; }
void audio_shutdown(void) {}
void audio_debug(unsigned stats[3]) { stats[0] = stats[1] = stats[2] = 0; }
#endif
