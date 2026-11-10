/* Decode ahead of the PSP audio mixer. Storage stalls must not hold the output thread. */
#include "game.h"
#ifdef PLATFORM_PSP
#include <limits.h>
#include <pspaudio.h>
#include <pspkernel.h>
#include <stdio.h>
#include <string.h>

#define MINIMP3_ONLY_MP3
#define MINIMP3_NO_SIMD
#define MINIMP3_IMPLEMENTATION
#include "vendor/minimp3.h"

enum { BLOCK_FRAMES = 1024, MUSIC_BLOCKS = 32 };
static volatile int audioRunning, readEvent, writeEvent;
static volatile unsigned musicFrames, audibleFrames, musicLoops, musicUnderruns;
static int events[16], channel = -1, mixer = -1, decoder = -1, emptyBlocks = -1, readyBlocks = -1;
static const char *launcherPath;
static short output[2][BLOCK_FRAMES * 2] __attribute__((aligned(64)));
static struct {
    short pcm[BLOCK_FRAMES * 2];
    unsigned wraps;
} musicQueue[MUSIC_BLOCKS];

static struct {
    FILE *file;
    long offset, length, position;
    int inputSize, cursor, samples, consumed;
    unsigned wraps;
    unsigned char input[16384];
    short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];
    mp3dec_t mp3;
} stream;

static FILE *open_eboot(void) {
    FILE *file = fopen(launcherPath, "rb");
    return file ? file : fopen("EBOOT.PBP", "rb");
}

static int open_soundtrack(void) {
    uint32_t header[10];
    stream.file = open_eboot();
    if (!stream.file || fread(header, sizeof(header), 1, stream.file) != 1 ||
        header[0] != 0x50425000u || header[1] != 0x10000u || header[8] < sizeof(header) ||
        header[9] <= header[8] || header[9] > LONG_MAX || fseek(stream.file, 0, SEEK_END) != 0)
        return 0;
    stream.offset = header[9];
    stream.length = ftell(stream.file) - stream.offset;
    mp3dec_init(&stream.mp3);
    return stream.length > 0 && fseek(stream.file, stream.offset, SEEK_SET) == 0;
}

/* Reopen at the same byte after a transient storage error, for example on resume. */
static int read_music(void) {
    int left = stream.inputSize - stream.cursor;
    memmove(stream.input, stream.input + stream.cursor, left);
    stream.inputSize = left;
    stream.cursor = 0;
    long remaining = stream.length - stream.position;
    int count = sizeof(stream.input) - left;
    if (count > remaining) count = remaining;
    if (!count) return 1;
    if (!stream.file) {
        stream.file = open_eboot();
        if (stream.file && fseek(stream.file, stream.offset + stream.position, SEEK_SET) != 0) {
            fclose(stream.file);
            stream.file = NULL;
        }
    }
    if (!stream.file) return 0;
    if (fread(stream.input + left, 1, count, stream.file) != (size_t)count) {
        fclose(stream.file);
        stream.file = NULL;
        return 0;
    }
    stream.inputSize += count;
    stream.position += count;
    return 1;
}

static void rewind_music(void) {
    if (stream.file && fseek(stream.file, stream.offset, SEEK_SET) != 0) {
        fclose(stream.file);
        stream.file = NULL;
    }
    stream.position = stream.cursor = stream.inputSize = 0;
    stream.wraps++;
    mp3dec_init(&stream.mp3);
}

static int decode_frame(void) {
    for (int attempt = 0; attempt < 12; attempt++) {
        if (stream.inputSize - stream.cursor < 4096 && !read_music())
            return 0;
        mp3dec_frame_info_t info = {0};
        int frames = mp3dec_decode_frame(&stream.mp3, stream.input + stream.cursor,
                                         stream.inputSize - stream.cursor, stream.pcm, &info);
        stream.cursor += info.frame_bytes;
        if (frames && info.hz == 44100 && info.channels == 2) {
            stream.samples = frames;
            stream.consumed = 0;
            return 1;
        }
        if (stream.position == stream.length && stream.cursor == stream.inputSize)
            rewind_music();
    }
    return 0;
}

static int decode_thread(SceSize n, void *arg) {
    (void)n;
    (void)arg;
    if (open_soundtrack()) {
        unsigned write = 0;
        while (audioRunning && sceKernelWaitSema(emptyBlocks, 1, NULL) >= 0 && audioRunning) {
            int filled = 0;
            unsigned before = stream.wraps;
            while (filled < BLOCK_FRAMES && audioRunning) {
                if (stream.consumed == stream.samples && !decode_frame()) {
                    sceKernelDelayThread(75000);
                    continue;
                }
                int count = stream.samples - stream.consumed;
                if (count > BLOCK_FRAMES - filled) count = BLOCK_FRAMES - filled;
                memcpy(musicQueue[write].pcm + filled * 2, stream.pcm + stream.consumed * 2,
                       count * 2 * sizeof(short));
                stream.consumed += count;
                filled += count;
            }
            if (!audioRunning) break;
            musicQueue[write].wraps = stream.wraps - before;
            write = (write + 1) % MUSIC_BLOCKS;
            sceKernelSignalSema(readyBlocks, 1);
        }
    }
    if (stream.file) fclose(stream.file);
    return 0;
}

typedef struct {
    int kind, left, length, filtered;
    unsigned phase, step, noise;
} Voice;
static Voice voices[4];
void audio_event(int kind) {
    if (kind < SFX_SWING || kind > SFX_MINE) return;
    int next = (writeEvent + 1) & 15;
    if (next == readEvent)
        return;
    events[writeEvent] = kind;
    writeEvent = next;
}
static int sound_thread(SceSize n, void *arg) {
    (void)n;
    (void)arg;
    unsigned serial = 0, read = 0, back = 0;
    while (audioRunning) {
        short *buffer = output[back];
        int audible = 0;
        int playing = readyBlocks >= 0 && sceKernelPollSema(readyBlocks, 1) >= 0;
        if (playing) {
            for (int i = 0; i < BLOCK_FRAMES * 2; i++) {
                buffer[i] = musicQueue[read].pcm[i] / 2;
                audible |= buffer[i];
            }
            musicLoops += musicQueue[read].wraps;
            read = (read + 1) % MUSIC_BLOCKS;
            sceKernelSignalSema(emptyBlocks, 1);
        } else {
            memset(buffer, 0, sizeof(output[0]));
            if (musicFrames) musicUnderruns++;
        }
        while (readEvent != writeEvent) {
            int kind = events[readEvent];
            readEvent = (readEvent + 1) & 15;
            Voice *v = &voices[serial++ % 4];
            static const unsigned short length[] = {0,5500,7000,10000,9000,1250,2300,4300,3700,2850,3550};
            static const unsigned short pitch[] = {0,700,190,2200,110,920,1160,1670,500,102,3200};
            v->kind = kind;
            v->length = v->left = length[kind];
            v->phase = 0;
            v->filtered = 0;
            v->step = pitch[kind];
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
                int sample;
                v->filtered += (noise - v->filtered) / 10;
                switch (v->kind) {
                case SFX_SWING: sample = noise / 3; break;
                case SFX_HIT: sample = wave + noise / 2; break;
                case SFX_BLOCK: sample = wave + noise / 4; break;
                case SFX_MENU_MOVE: sample = wave / 7; break;
                case SFX_MENU_PICK: sample = (wave + v->filtered) / 5; break;
                case SFX_DRAW: sample = (noise - v->filtered) / 6 + wave / 8; break;
                case SFX_SHEATHE: sample = v->filtered / 2 + wave / 7; break;
                case SFX_STEP: sample = v->filtered / 2 + wave / 11; break;
                case SFX_MINE: sample = wave / 4 + noise / 8; break;
                default: sample = noise; break;
                }
                int amp = v->left * 256 / v->length;
                int attack = (v->length - v->left) * 4;
                if (amp > attack) amp = attack;
                mixed += sample * amp / 256;
                v->left--;
            }
            for (int side = 0; side < 2; side++) {
                int sample = mixed + buffer[2 * i + side];
                buffer[2 * i + side] = sample > 30000 ? 30000 : sample < -30000 ? -30000 : sample;
            }
        }
        sceKernelDcacheWritebackRange(buffer, sizeof(output[0]));
        if (sceAudioOutputBlocking(channel, 0x5800, buffer) >= 0) {
            if (playing) musicFrames += BLOCK_FRAMES;
            if (audible) audibleFrames += BLOCK_FRAMES;
            back ^= 1;
        } else {
            sceKernelDelayThread(10000);
        }
    }
    return 0;
}

static int start_audio_thread(const char *name, SceKernelThreadEntry fn, int priority, int stack) {
    int id = sceKernelCreateThread(name, fn, priority, stack, 0, NULL);
    if (id < 0) return -1;
    if (sceKernelStartThread(id, 0, NULL) >= 0) return id;
    sceKernelDeleteThread(id);
    return -1;
}

void audio_init(const char *eboot) {
    launcherPath = eboot && *eboot ? eboot : "EBOOT.PBP";
    channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, BLOCK_FRAMES, PSP_AUDIO_FORMAT_STEREO);
    if (channel < 0) return;
    emptyBlocks = sceKernelCreateSema("Dust music free", 0, MUSIC_BLOCKS, MUSIC_BLOCKS, NULL);
    readyBlocks = sceKernelCreateSema("Dust music ready", 0, 0, MUSIC_BLOCKS, NULL);
    audioRunning = 1;
    if (emptyBlocks >= 0 && readyBlocks >= 0)
        decoder = start_audio_thread("Dust music", decode_thread, 0x19, 0x8000);
    mixer = start_audio_thread("Dust mixer", sound_thread, 0x16, 0x4000);
    if (mixer < 0) audio_shutdown();
}

void audio_shutdown(void) {
    audioRunning = 0;
    if (decoder >= 0) {
        sceKernelSignalSema(emptyBlocks, 1);
        sceKernelWaitThreadEnd(decoder, NULL);
        sceKernelDeleteThread(decoder);
    }
    if (mixer >= 0) {
        sceKernelWaitThreadEnd(mixer, NULL);
        sceKernelDeleteThread(mixer);
    }
    if (emptyBlocks >= 0) sceKernelDeleteSema(emptyBlocks);
    if (readyBlocks >= 0) sceKernelDeleteSema(readyBlocks);
    if (channel >= 0) sceAudioChRelease(channel);
    decoder = mixer = emptyBlocks = readyBlocks = channel = -1;
}

void audio_debug(unsigned stats[4]) {
    stats[0] = musicFrames;
    stats[1] = audibleFrames;
    stats[2] = musicLoops;
    stats[3] = musicUnderruns;
}
#else
void audio_event(int kind) { (void)kind; }
void audio_init(const char *eboot) { (void)eboot; }
void audio_shutdown(void) {}
void audio_debug(unsigned stats[4]) { stats[0] = stats[1] = stats[2] = stats[3] = 0; }
#endif
