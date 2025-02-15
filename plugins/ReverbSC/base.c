#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "base.h"

int sp_create(sp_data **spp) { return sp_createn(spp, 1); }

int sp_createn(sp_data **spp, int nchan)
{
	const uint32_t sr = 44100; // TODO C23: constexpr auto
	const unsigned long len_seconds = 5; // TODO C23: constexpr auto
	*spp = malloc(sizeof(sp_data));
	**spp = (sp_data){ .out = calloc(nchan, sizeof(float)), .sr = sr,
		.nchan = nchan, .len = len_seconds * sr, .pos = 0,
		.filename = "test.wav", .rand = 0 };
	return 0;
}

int sp_destroy(sp_data **spp)
{
	free((*spp)->out);
	free(*spp);
	return 0;
}

#ifndef NO_LIBSNDFILE

int sp_process(sp_data *sp, void *ud, void (*callback)(sp_data *, void *))
{
    SNDFILE *sf[sp->nchan];
    char tmp[140];
    SF_INFO info;
    memset(&info, 0, sizeof(SF_INFO));
    SPFLOAT buf[sp->nchan][SP_BUFSIZE];
    info.samplerate = sp->sr;
    info.channels = 1;
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_24;
    int numsamps, i, chan;
    if(sp->nchan == 1) {
        sf[0] = sf_open(sp->filename, SFM_WRITE, &info);
    } else {
        for(chan = 0; chan < sp->nchan; chan++) {
            snprintf(tmp, sizeof(tmp), "%02d_%s", chan, sp->filename);
            sf[chan] = sf_open(tmp, SFM_WRITE, &info);
        }
    }

    while(sp->len > 0){
        if(sp->len < SP_BUFSIZE) {
            numsamps = sp->len;
        }else{
            numsamps = SP_BUFSIZE;
        }
        for(i = 0; i < numsamps; i++){
            callback(sp, ud);
            for(chan = 0; chan < sp->nchan; chan++) {
                buf[chan][i] = sp->out[chan];
            }
            sp->pos++;
        }
        for(chan = 0; chan < sp->nchan; chan++) {
#ifdef USE_DOUBLE
            sf_write_double(sf[chan], buf[chan], numsamps);
#else
            sf_write_float(sf[chan], buf[chan], numsamps);
#endif
        }
        sp->len -= numsamps;
    }
    for(i = 0; i < sp->nchan; i++) {
        sf_close(sf[i]);
    }
    return 0;
}

#endif

int sp_process_raw(sp_data *sp, void *ud, void (*callback)(sp_data *, void *))
{
    int chan;
    if(sp->len == 0) {
        while(1) {
            callback(sp, ud);
            for (chan = 0; chan < sp->nchan; chan++) {
                fwrite(&sp->out[chan], sizeof(SPFLOAT), 1, stdout);
            }
            sp->len--;
        }
    } else {
        while(sp->len > 0) {
            callback(sp, ud);
            for (chan = 0; chan < sp->nchan; chan++) {
                fwrite(&sp->out[chan], sizeof(SPFLOAT), 1, stdout);
            }
            sp->len--;
            sp->pos++;
        }
    }
    return SP_OK;
}

#ifdef USE_SPA
int sp_process_spa(sp_data *sp, void *ud, void (*callback)(sp_data *, void *))
{
    sp_audio spa;
    if(spa_open(sp, &spa, sp->filename, SPA_WRITE) == SP_NOT_OK) {
        fprintf(stderr, "Error: could not open file %s.\n", sp->filename);    
    }
    while(sp->len > 0) {
        callback(sp, ud);
        spa_write_buf(sp, &spa, sp->out, sp->nchan);
        sp->len--;
        sp->pos++;
    }
    spa_close(&spa);
    return SP_OK;
}
#endif

int sp_process_plot(sp_data *sp, void *ud, void (*callback)(sp_data *, void *))
{
    int chan;
    fprintf(stdout, "sp_out =  [ ... \n");
    while(sp->len > 0) {
        callback(sp, ud);
        for (chan = 0; chan < sp->nchan; chan++) {
            /* fwrite(&sp->out[chan], sizeof(SPFLOAT), 1, stdout); */
            fprintf(stdout, "%g ", sp->out[chan]);
        }
        fprintf(stdout, "; ...\n");
        sp->len--;
        sp->pos++;
    }
    fprintf(stdout, "];\n");

    fprintf(stdout, "plot(sp_out);\n");
    fprintf(stdout, "title('Plot generated by Soundpipe');\n");
    fprintf(stdout, "xlabel('Time (samples)');\n");
    fprintf(stdout, "ylabel('Amplitude');\n");
    return SP_OK;
}

int sp_auxdata_alloc(sp_auxdata *aux, size_t size)
{
    aux->ptr = malloc(size);
    aux->size = size;
    memset(aux->ptr, 0, size);
    return SP_OK;
}

int sp_auxdata_free(sp_auxdata *aux)
{
    free(aux->ptr);
    return SP_OK;
}


SPFLOAT sp_midi2cps(SPFLOAT nn)
{
    return pow(2, (nn - 69.0) / 12.0) * 440.0;
}

int sp_set(sp_param *p, SPFLOAT val) {
    p->state = 1;
    p->val = val;
    return SP_OK;
}

int sp_out(sp_data *sp, uint32_t chan, SPFLOAT val)
{
    if (chan >= (uint32_t) sp->nchan) {
        fprintf(stderr, "sp_out: Invalid channel\n");
        return SP_NOT_OK;
    }
    sp->out[chan] = val;
    return SP_OK;
}

/* 
uint32_t sp_rand(sp_data *sp)
{
    uint32_t val = (1103515245 * sp->rand + 12345) % SP_RANDMAX;
    sp->rand = val;
    return val;
}
*/

void sp_srand(sp_data *sp, uint32_t val)
{
    sp->rand = val;
}


