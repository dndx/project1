#ifndef SOUND_H_
#define SOUND_H_
struct soundfile {
    unsigned int sample_num;
    unsigned int sample_rate;
    unsigned short channels;
    unsigned short bit_depth;
};
#endif /* !SOUND_H_ */
