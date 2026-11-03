/* Original procedural sound effects. Fixed memory, no runtime audio assets. */
#include "game.h"
#ifdef PLATFORM_PSP
#include <pspaudio.h>
#include <pspkernel.h>
static volatile int audioRunning, readEvent, writeEvent;
static int events[16], channel = -1, thread = -1;
static short buffer[512] __attribute__((aligned(64)));
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
    unsigned serial = 0;
    while (audioRunning) {
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
        for (int i = 0; i < 512; i++) {
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
            if (mixed > 30000)
                mixed = 30000;
            if (mixed < -30000)
                mixed = -30000;
            buffer[i] = (short)mixed;
        }
        sceAudioOutputBlocking(channel, 0x5800, buffer);
    }
    return 0;
}
void audio_init(void) {
    channel = sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL, 512, PSP_AUDIO_FORMAT_MONO);
    if (channel < 0)
        return;
    audioRunning = 1;
    thread = sceKernelCreateThread("Dustbound sound", sound_thread, 0x18, 0x1000, 0, 0);
    if (thread >= 0)
        sceKernelStartThread(thread, 0, 0);
    else {
        audioRunning = 0;
        sceAudioChRelease(channel);
        channel = -1;
    }
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
#else
void audio_event(int kind) { (void)kind; }
void audio_init(void) {}
void audio_shutdown(void) {}
#endif
