#include <genesis.h>

enum FmParameters {
    PARAMETER_G_LFO_ON,
    PARAMETER_G_LFO_FREQ,
    PARAMETER_NOTE,
    PARAMETER_FREQ,
    PARAMETER_OCTAVE,
    PARAMETER_ALGORITHM,
    PARAMETER_FEEDBACK,
    PARAMETER_LFO_AMS,
    PARAMETER_LFO_FMS,
    PARAMETER_STEREO
};

typedef struct {
    const char name[10];
    const u16 minSize;
    u16 value;
    const u16 maxValue;
    const u8 step;
    const void (*onUpdate)(void);
} FmParameter;

void playFmNote(void);
void stopFmNote(void);
FmParameter* fmParameter(u16 index);
u16 maxFmParameters(void);
