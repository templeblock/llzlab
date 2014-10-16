#include <stdio.h> // fputc(), etc
#include <stdlib.h> // exit()
#include <unistd.h> // write(), lseek()
#include <string.h> // strncmp(), strlen()
#include <fcntl.h> // open(), fcntl()
#include <sys/stat.h> // S_IRUSR, S_IWUSR
#include <math.h> // log()

#include "llz_midi.h"


/* write/read variable-length variable
*/
static int write_var_len (int fd, long value)
{
    unsigned char rep[4];
    int bytes;

    bytes = 1;
    rep[3] = value & 0x7f;
    value >>= 7;
    while (value >0)
    {
        rep[3 - bytes] = (value & 0x7f) | 0x80;
        bytes ++;
        value >>= 7;
    }

#ifdef WIN32
    return (_write (fd, &rep[4-bytes], bytes));
#else
    return (write (fd, &rep[4-bytes], bytes));
#endif
}

static int read_var_len (int fd, long *value)
{
    unsigned char c;
    int bytes;

    *value = 0;
    bytes = 0;
    do
    {
        (*value) <<= 7;
        bytes += read (fd, &c, 1);
        (*value) += (c & 0x7f);
    }
    while ( (c & 0x80) == 0x80 );

    return bytes;
}

/* Write long, big-endian: big end first.
 * RETURN VAULE
 *  written bytes
 */
static int wblong (int fd, unsigned long ul)
{
    unsigned char data[4];
    data[0] = (char) (ul >> 24) & 0xff;
    data[1] = (char) (ul >> 16) & 0xff;
    data[2] = (char) (ul >> 8) & 0xff;
    data[3] = (char) (ul) & 0xff;

#ifdef WIN32
    return _write (fd, data, 4);
#else
    return write (fd, data, 4);
#endif
}

/* Write short, big-endian: big end first.
 * RETURN VAULE
 *  written bytes
 */
static int wbshort (int fd, unsigned short us)
{
    unsigned char data[2];
    data[0] = (char) (us >> 8) & 0xff;
    data[1] = (char) (us) & 0xff;

#ifdef WIN32
    return _write (fd, data, 2);
#else 
    return write (fd, data, 2);
#endif
}


/** general midi-frequency stuff **/
double midi_to_freq (int midi)
{
    double f;

    f = exp (log (440.0) + (double)(midi - 69) * log (2.0) / 12.0);
    return (f);
}

int freq_to_midi (double f)
{
    int midi;

    midi = (int)(0.5 + 69.0 + 12.0 / log (2.0) * log (f / 440.0));
    return (midi);
}

double midi_to_logf (int midi)
{
    double logf;

    logf = log (440.0) + (double)(midi - 69) * log (2.0) / 12.0;
    return (logf);
}

int logf_to_midi (double logf)
{
    int midi;

    midi = (int)(0.5 + 69.0 + 12.0 / log (2.0) * (logf - log (440.0)));
    return (midi);
}

/* write midi header
*/
static int smf_header_fmt (int fd,
        unsigned short format,
        unsigned short tracks,
        unsigned short divisions)
{
    int num;

#ifdef WIN32
    num = _write (fd, "MThd", 4);
#else 
    num = write (fd, "MThd", 4);
#endif


    num += wblong (fd, 6); /* head data size (= 6)  */
    num += wbshort (fd, format);
    num += wbshort (fd, tracks);
    num += wbshort (fd, divisions);
    return num;
    /* num = 14  */
}

/* write program change
*/
static int smf_prog_change (int fd, char channel, char prog)
{
    unsigned char data[3];

    data[0] = 0x00; /* delta time  */
    data[1] = 0xC0 + channel;
    data[2] = prog;

#ifdef WIN32
    return _write (fd, data, 3);
#else 
    return write (fd, data, 3);
#endif
}

/* write tempo
 *  tempo : microseconds per quarter note
 *          0x07A120 (or 500,000) microseconds (= 0.5 sec) for 120 bpm
 */
static int smf_tempo (int fd, unsigned long tempo)
{
    unsigned char data[7];

    data[0] = 0x00; /* delta time  */
    data[1] = 0xff; /* meta */
    data[2] = 0x51; /* tempo */
    data[3] = 0x03; /* bytes */
    data[4] = (char) (tempo >> 16) & 0xff;
    data[5] = (char) (tempo >>  8) & 0xff;
    data[6] = (char) (tempo      ) & 0xff;

#ifdef WIN32
    return _write (fd, data, 7);
#else
    return write (fd, data, 7);
#endif
}

/* note on
*/
static int smf_note_on (int fd, long dtime, char note, char vel, char channel)
{
    unsigned char data[3];
    int num;

    num = write_var_len (fd, dtime);
    data[0] = 0x90 + channel;
    data[1] = note;
    data[2] = vel;

#ifdef WIN32 
    num += _write (fd, data, 3);
#else
    num += write (fd, data, 3);
#endif

    return num;
}

/* note off
*/
static int smf_note_off (int fd, long dtime, char note, char vel, char channel)
{
    unsigned char data[3];
    int num;

    num = write_var_len (fd, dtime);
    data[0] = 0x80 + channel;
    data[1] = note;
    data[2] = vel;

#ifdef WIN32
    num += _write (fd, data, 3);
#else 
    num += write (fd, data, 3);
#endif

    return num;
}

/* write track head
*/
static int smf_track_head (int fd, unsigned long size)
{
    int num;
#ifdef WIN32
    num = _write (fd, "MTrk", 4);
#else 
    num = write (fd, "MTrk", 4);
#endif
    num += wblong (fd, size);
    return num;
    /* num = 8  */
}

/* write track end
*/
static int smf_track_end (int fd)
{
    unsigned char data[4];
    data[0] = 0x00; /* delta time  */
    data[1] = 0xff;
    data[2] = 0x2f;
    data[3] = 0x00;

#ifdef WIN32 
    return _write (fd, data, 4);
#else
    return write (fd, data, 4);
#endif
}
double onetick_duration(int tempo, int div)
{
    double duration;

    duration = (double)tempo / (double) div;

    duration /= 1000.0;

    return duration;
}

typedef struct _llz_midi_t {
    int fd;

    int tempo;
    int div;

    double tick_duration;

    int offset_header;
    int header_len;

    int cur_size;
} llz_midi_t;

unsigned long llz_midi_init()
{
    llz_midi_t *f = (llz_midi_t *)malloc(sizeof(llz_midi_t));
    memset(f, 0, sizeof(llz_midi_t));

    f->fd = -1;

    f->tempo = 500000;
    f->div   = 120;
    f->tick_duration = onetick_duration(f->tempo, f->div);

    f->offset_header = 0;
    f->header_len    = 0;
    f->cur_size      = 0;

    return (unsigned long)f;
}

void llz_midi_uninit(unsigned long handle)
{
    llz_midi_t *f = (llz_midi_t *)handle;

    if (f) {
        if (f->fd >= 0) {
#ifdef WIN32
            _close(f->fd);
#else 
            close(f->fd);
#endif
        }

        free(f);
        f = NULL;
    }
}

int llz_midi_open(unsigned long handle, char *filename, int tempo, int div)
{
    int ret;
    llz_midi_t *f = (llz_midi_t *)handle;

    if (NULL == filename)
        return -1;

    f->fd = -1;

#ifdef WIN32
    f->fd = _open (filename, _O_RDWR| _O_CREAT| _O_TRUNC, _S_IREAD| _S_IWRITE);
#else
    f->fd = open (filename, O_RDWR| O_CREAT| O_TRUNC, S_IRUSR| S_IWUSR);
#endif

    if (f->fd < 0)
        goto fail;

    f->tempo = tempo;
    f->div   = div;
    f->tick_duration = onetick_duration(tempo, div);

    f->offset_header = 0;
    f->header_len    = 0;
    f->cur_size      = 0;

    ret = smf_header_fmt (f->fd, 0, 1, div);
    if (ret != 14) {
        fprintf (stderr, "Error during writing mid! %d (header)\n", f->cur_size);
        goto fail;
    }
    f->cur_size += ret;

    f->offset_header = f->cur_size; /* pointer of track-head  */
    ret = smf_track_head (f->fd, (7+4*100));
    if (ret != 8) {
        fprintf (stderr, "Error during writing mid! %d (track header)\n", f->cur_size);
        goto fail;
    }
    f->cur_size += ret;

    /* head of data  */
    f->header_len = f->cur_size;

    /* tempo set  */
    ret = smf_tempo (f->fd, tempo); // if tempo = 500000, 0.5 sec => 120 bpm for 4/4
    if (ret != 7) {
        fprintf (stderr, "Error during writing mid! %d (tempo)\n", f->cur_size);
        goto fail;
    }
    f->cur_size += ret;

    /* ch.0 prog. 0  */
    ret = smf_prog_change (f->fd, 0, 0);
    /*ret = smf_prog_change (f->fd, 0, 1);*/
    /*ret = smf_prog_change (f->fd, 0, 65);*/
    /*ret = smf_prog_change (f->fd, 0, 40);*/
    if (ret != 3) {
        fprintf (stderr, "Error during writing mid! %d (prog change)\n", f->cur_size);
        goto fail;
    }
    f->cur_size += ret;

    return 0;

fail:
    if (f->fd >= 0) {
#ifdef WIN32 
        _close(f->fd);
#else 
        close(f->fd);
#endif
        f->fd = -1;
    }

    return -1;
}

int llz_midi_close(unsigned long handle)
{
    int ret;
    llz_midi_t *f = (llz_midi_t *)handle;

    ret = smf_track_end(f->fd);
    if (ret != 4) {
        fprintf (stderr, "Error during writing mid! %d (track end)\n",
                 f->cur_size);
        return -1;
    }
    f->cur_size += ret;
    
    /* re-calculate # of data in track  */
    if (lseek (f->fd, f->offset_header, SEEK_SET) < 0) {
        fprintf (stderr, "Error during lseek %d (re-calc)\n", f->offset_header);
        return -1;
    }
    ret = smf_track_head (f->fd, (f->cur_size - f->header_len));
    if (ret != 8) {
        fprintf (stderr, "Error during write %d (re-calc)\n", f->cur_size);
        return -1;
    }

#ifdef WIN32 
    _close(f->fd);
#else 
    close(f->fd);
#endif

    f->fd = -1;

    return 0;
}

int llz_midi_addnote(unsigned long handle, 
                     int note, int on, double duration, double amp)
{
    int ret;
    llz_midi_t *f = (llz_midi_t *)handle;
    int  idt = 0;
    char vel;

    idt = (int)(duration / f->tick_duration);
    /*if (idt > 127)*/
        /*idt = 127;*/

    vel = (char) (amp * 127);

    /*printf(">>> idt = %d, midi duration=%lf\n", idt, idt * f->tick_duration);*/
    if (on) {
        ret = smf_note_on(f->fd, idt, note, vel, 0);
        f->cur_size += ret;
    } else {
        ret = smf_note_off(f->fd, idt, note, vel, 0);
        f->cur_size += ret;
    }

    return 0;
}

