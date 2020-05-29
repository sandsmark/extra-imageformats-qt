/*//////////////////////////////////////////////////////////////////////////////////////////////////////

Copyright 2019, Jon Sneyers, Cloudinary (jon@cloudinary.com)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

//////////////////////////////////////////////////////////////////////////////////////////////////////*/

#pragma once
#include "../image/image.h"

// gif decoder from https://github.com/lecram/gifdec

#include <stdint.h>
#include <sys/types.h>

typedef struct gd_Palette {
    int size;
    uint8_t colors[0x100 * 3];
} gd_Palette;

typedef struct gd_GCE {
    uint16_t delay;
    uint8_t tindex;
    uint8_t disposal;
    int input;
    int transparency;
} gd_GCE;

typedef struct gd_GIF {
    int fd;
    off_t anim_start;
    uint16_t width, height;
    uint16_t depth;
    uint16_t loop_count;
    gd_GCE gce;
    gd_Palette *palette;
    gd_Palette lct, gct;
    void (*plain_text)(
        struct gd_GIF *gif, uint16_t tx, uint16_t ty,
        uint16_t tw, uint16_t th, uint8_t cw, uint8_t ch,
        uint8_t fg, uint8_t bg
    );
    void (*comment)(struct gd_GIF *gif);
    void (*application)(struct gd_GIF *gif, char id[8], char auth[3]);
    uint16_t fx, fy, fw, fh;
    uint8_t bgindex;
    uint8_t *canvas, *frame;
} gd_GIF;

gd_GIF *gd_open_gif(const char *fname);
int gd_get_frame(gd_GIF *gif);
void gd_render_frame(gd_GIF *gif, uint8_t *buffer);
void gd_rewind(gd_GIF *gif);
void gd_close_gif(gd_GIF *gif);



Image read_GIF_file(const char *filename)
{
    gd_GIF *gif;
    gif = gd_open_gif(filename);

    if (!gif) {
//        v_printf(3,"%s could not be loaded as GIF\n",filename);
        return Image();
    }

    int w = gif->width;
    int h = gif->height;
    uint8_t *frame = (uint8_t *) calloc(1, w * h * 4);

    Image image(w, h, 255, 4);
    image.den = 100;
    int ret = 1;
    int f = 0;

    while (ret) {
        ret = gd_get_frame(gif);

        if (ret < 1) {
            break;
        }

        // make room
        image.channel[0].resize(w, h + h * f);
        image.channel[1].resize(w, h + h * f);
        image.channel[2].resize(w, h + h * f);
        image.channel[3].resize(w, h + h * f);

        gd_render_frame(gif, frame);
        image.num.push_back(gif->gce.delay);
//        printf("Frame %i: ret=%i, delay=%i\n",f,ret,gif->gce.delay);

        for (int y = 0; y < h; y++) {
            for (int x = 0; x < w; x++) {
                image.channel[0].value(y + h * f, x) = frame[y * 4 * w + x * 4];
                image.channel[1].value(y + h * f, x) = frame[y * 4 * w + x * 4 + 1];
                image.channel[2].value(y + h * f, x) = frame[y * 4 * w + x * 4 + 2];
                image.channel[3].value(y + h * f, x) = frame[y * 4 * w + x * 4 + 3];
            }
        }

        f++;
    }

    free(frame);
    gd_close_gif(gif);
    image.nb_frames = f;
    image.h = h * f;

    return image;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

typedef struct Entry {
    uint16_t length;
    uint16_t prefix;
    uint8_t  suffix;
} Entry;

typedef struct Table {
    int bulk;
    int nentries;
    Entry *entries;
} Table;

static uint16_t
read_num(int fd)
{
    uint8_t bytes[2];

    read(fd, bytes, 2);
    return bytes[0] + (((uint16_t) bytes[1]) << 8);
}

gd_GIF *
gd_open_gif(const char *fname)
{
    int fd;
    uint8_t sigver[3];
    uint16_t width, height, depth;
    uint8_t fdsz, bgidx, aspect;
    int gct_sz;
    gd_GIF *gif;

    fd = open(fname, O_RDONLY);

    if (fd == -1) {
        return NULL;
    }

    /* Header */
    read(fd, sigver, 3);

    if (memcmp(sigver, "GIF", 3) != 0) {
//        fprintf(stderr, "invalid signature\n");
        goto fail;
    }

    /* Version */
    read(fd, sigver, 3);

    if (memcmp(sigver, "89a", 3) != 0) {
        fprintf(stderr, "invalid GIF version\n");
        goto fail;
    }

    /* Width x Height */
    width  = read_num(fd);
    height = read_num(fd);
    /* FDSZ */
    read(fd, &fdsz, 1);

    /* Presence of GCT */
    if (!(fdsz & 0x80)) {
        fprintf(stderr, "no global color table\n");
        goto fail;
    }

    /* Color Space's Depth */
    depth = ((fdsz >> 4) & 7) + 1;
    /* Ignore Sort Flag. */
    /* GCT Size */
    gct_sz = 1 << ((fdsz & 0x07) + 1);
    /* Background Color Index */
    read(fd, &bgidx, 1);
    /* Aspect Ratio */
    read(fd, &aspect, 1);
    /* Create gd_GIF Structure. */
    gif = (gd_GIF *) calloc(1, sizeof(*gif) + 5 * width * height);

    if (!gif) {
        goto fail;
    }

    gif->fd = fd;
    gif->width  = width;
    gif->height = height;
    gif->depth  = depth;
    /* Read GCT */
    gif->gct.size = gct_sz;
    read(fd, gif->gct.colors, 3 * gif->gct.size);
    gif->palette = &gif->gct;
    gif->bgindex = bgidx;
    gif->canvas = (uint8_t *) &gif[1];
    gif->frame = &gif->canvas[4 * width * height];

    if (gif->bgindex) {
        memset(gif->frame, gif->bgindex, gif->width * gif->height);
    }

    gif->anim_start = lseek(fd, 0, SEEK_CUR);
    goto ok;
fail:
    close(fd);
ok:
    return gif;
}

static void
discard_sub_blocks(gd_GIF *gif)
{
    uint8_t size;

    do {
        read(gif->fd, &size, 1);
        lseek(gif->fd, size, SEEK_CUR);
    } while (size);
}

static void
read_plain_text_ext(gd_GIF *gif)
{
    if (gif->plain_text) {
        uint16_t tx, ty, tw, th;
        uint8_t cw, ch, fg, bg;
        off_t sub_block;
        lseek(gif->fd, 1, SEEK_CUR); /* block size = 12 */
        tx = read_num(gif->fd);
        ty = read_num(gif->fd);
        tw = read_num(gif->fd);
        th = read_num(gif->fd);
        read(gif->fd, &cw, 1);
        read(gif->fd, &ch, 1);
        read(gif->fd, &fg, 1);
        read(gif->fd, &bg, 1);
        sub_block = lseek(gif->fd, 0, SEEK_CUR);
        gif->plain_text(gif, tx, ty, tw, th, cw, ch, fg, bg);
        lseek(gif->fd, sub_block, SEEK_SET);
    } else {
        /* Discard plain text metadata. */
        lseek(gif->fd, 13, SEEK_CUR);
    }

    /* Discard plain text sub-blocks. */
    discard_sub_blocks(gif);
}

static void
read_graphic_control_ext(gd_GIF *gif)
{
    uint8_t rdit;

    /* Discard block size (always 0x04). */
    lseek(gif->fd, 1, SEEK_CUR);
    read(gif->fd, &rdit, 1);
    gif->gce.disposal = (rdit >> 2) & 3;
    gif->gce.input = rdit & 2;
    gif->gce.transparency = rdit & 1;
    gif->gce.delay = read_num(gif->fd);
    read(gif->fd, &gif->gce.tindex, 1);
    /* Skip block terminator. */
    lseek(gif->fd, 1, SEEK_CUR);
}

static void
read_comment_ext(gd_GIF *gif)
{
    if (gif->comment) {
        off_t sub_block = lseek(gif->fd, 0, SEEK_CUR);
        gif->comment(gif);
        lseek(gif->fd, sub_block, SEEK_SET);
    }

    /* Discard comment sub-blocks. */
    discard_sub_blocks(gif);
}

static void
read_application_ext(gd_GIF *gif)
{
    char app_id[8];
    char app_auth_code[3];

    /* Discard block size (always 0x0B). */
    lseek(gif->fd, 1, SEEK_CUR);
    /* Application Identifier. */
    read(gif->fd, app_id, 8);
    /* Application Authentication Code. */
    read(gif->fd, app_auth_code, 3);

    if (!strncmp(app_id, "NETSCAPE", sizeof(app_id))) {
        /* Discard block size (0x03) and constant byte (0x01). */
        lseek(gif->fd, 2, SEEK_CUR);
        gif->loop_count = read_num(gif->fd);
        /* Skip block terminator. */
        lseek(gif->fd, 1, SEEK_CUR);
    } else if (gif->application) {
        off_t sub_block = lseek(gif->fd, 0, SEEK_CUR);
        gif->application(gif, app_id, app_auth_code);
        lseek(gif->fd, sub_block, SEEK_SET);
        discard_sub_blocks(gif);
    } else {
        discard_sub_blocks(gif);
    }
}

static void
read_ext(gd_GIF *gif)
{
    uint8_t label;

    read(gif->fd, &label, 1);

    switch (label) {
    case 0x01:
        read_plain_text_ext(gif);
        break;

    case 0xF9:
        read_graphic_control_ext(gif);
        break;

    case 0xFE:
        read_comment_ext(gif);
        break;

    case 0xFF:
        read_application_ext(gif);
        break;

    default:
        fprintf(stderr, "unknown extension: %02X\n", label);
    }
}

static Table *
new_table(int key_size)
{
    int key;
    int init_bulk = MAX(1 << (key_size + 1), 0x100);
    Table *table = (Table *) malloc(sizeof(*table) + sizeof(Entry) * init_bulk);

    if (table) {
        table->bulk = init_bulk;
        table->nentries = (1 << key_size) + 2;
        table->entries = (Entry *) &table[1];

        for (key = 0; key < (1 << key_size); key++)
            table->entries[key] = (Entry) {
            1, 0xFFF, (uint8_t) key
        };
    }

    return table;
}

/* Add table entry. Return value:
 *  0 on success
 *  +1 if key size must be incremented after this addition
 *  -1 if could not realloc table */
static int
add_entry(Table **tablep, uint16_t length, uint16_t prefix, uint8_t suffix)
{
    Table *table = *tablep;

    if (table->nentries == table->bulk) {
        table->bulk *= 2;
        table = (Table *) realloc(table, sizeof(*table) + sizeof(Entry) * table->bulk);

        if (!table) {
            return -1;
        }

        table->entries = (Entry *) &table[1];
        *tablep = table;
    }

    table->entries[table->nentries] = (Entry) {
        length, prefix, suffix
    };
    table->nentries++;

    if ((table->nentries & (table->nentries - 1)) == 0) {
        return 1;
    }

    return 0;
}

static uint16_t
get_key(gd_GIF *gif, int key_size, uint8_t *sub_len, uint8_t *shift, uint8_t *byte)
{
    int bits_read;
    int rpad;
    int frag_size;
    uint16_t key;

    key = 0;

    for (bits_read = 0; bits_read < key_size; bits_read += frag_size) {
        rpad = (*shift + bits_read) % 8;

        if (rpad == 0) {
            /* Update byte. */
            if (*sub_len == 0) {
                read(gif->fd, sub_len, 1);    /* Must be nonzero! */
            }

            read(gif->fd, byte, 1);
            (*sub_len)--;
        }

        frag_size = MIN(key_size - bits_read, 8 - rpad);
        key |= ((uint16_t)((*byte) >> rpad)) << bits_read;
    }

    /* Clear extra bits to the left. */
    key &= (1 << key_size) - 1;
    *shift = (*shift + key_size) % 8;
    return key;
}

/* Decompress image pixels.
 * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
static int
read_image_data(gd_GIF *gif)
{
    uint8_t sub_len, shift, byte;
    int init_key_size, key_size, table_is_full;
    int frm_off, str_len, p;
    uint16_t key, clear, stop;
    int ret;
    Table *table;
    Entry entry;
    off_t start, end;

    read(gif->fd, &byte, 1);
    key_size = (int) byte;
    start = lseek(gif->fd, 0, SEEK_CUR);
    discard_sub_blocks(gif);
    end = lseek(gif->fd, 0, SEEK_CUR);
    lseek(gif->fd, start, SEEK_SET);
    clear = 1 << key_size;
    stop = clear + 1;
    table = new_table(key_size);
    key_size++;
    init_key_size = key_size;
    sub_len = shift = 0;
    key = get_key(gif, key_size, &sub_len, &shift, &byte); /* clear code */
    frm_off = 0;
    ret = 0;

    while (1) {
        if (key == clear) {
            key_size = init_key_size;
            table->nentries = (1 << (key_size - 1)) + 2;
            table_is_full = 0;
        } else if (!table_is_full) {
            ret = add_entry(&table, str_len + 1, key, entry.suffix);

            if (ret == -1) {
                free(table);
                return -1;
            }

            if (table->nentries == 0x1000) {
                ret = 0;
                table_is_full = 1;
            }
        }

        key = get_key(gif, key_size, &sub_len, &shift, &byte);

        if (key == clear) {
            continue;
        }

        if (key == stop) {
            break;
        }

        if (ret == 1) {
            key_size++;
        }

        entry = table->entries[key];
        str_len = entry.length;

        while (1) {
            p = frm_off + entry.length - 1;
            gif->frame[(gif->fy + p / gif->fw) * gif->width + gif->fx + p % gif->fw] = entry.suffix;

            if (entry.prefix == 0xFFF) {
                break;
            } else {
                entry = table->entries[entry.prefix];
            }
        }

        frm_off += str_len;

        if (key < table->nentries - 1 && !table_is_full) {
            table->entries[table->nentries - 1].suffix = entry.suffix;
        }
    }

    free(table);
    read(gif->fd, &sub_len, 1); /* Must be zero! */
    lseek(gif->fd, end, SEEK_SET);
    return 0;
}

/* Read image.
 * Return 0 on success or -1 on out-of-memory (w.r.t. LZW code table). */
static int
read_image(gd_GIF *gif)
{
    uint8_t fisrz;
    int interlace;

    /* Image Descriptor. */
    gif->fx = read_num(gif->fd);
    gif->fy = read_num(gif->fd);
    gif->fw = read_num(gif->fd);
    gif->fh = read_num(gif->fd);
    read(gif->fd, &fisrz, 1);
    interlace = fisrz & 0x40;

    /* Ignore Sort Flag. */
    /* Local Color Table? */
    if (fisrz & 0x80) {
        /* Read LCT */
        gif->lct.size = 1 << ((fisrz & 0x07) + 1);
        read(gif->fd, gif->lct.colors, 3 * gif->lct.size);
        gif->palette = &gif->lct;
    } else {
        gif->palette = &gif->gct;
    }

    /* Image Data. */
    if (read_image_data(gif)) {
        return -1;
    }

    if (interlace) {
        // reorganize rows of interlaced frame
        uint8_t *tmp = (uint8_t *) malloc(gif->width * gif->height);
        memcpy(tmp, gif->frame, gif->width * gif->height);
        int y = 0, scan = 0, step = 8;

        for (int iy = 0; iy < gif->fh; iy++) {
            memcpy(&gif->frame[(gif->fy + y) * gif->width + gif->fx], &tmp[(gif->fy + iy) * gif->width + gif->fx], gif->fw);
            y += step;

            if (y >= gif->fh) {
                scan++;
                step = 16 >> scan;
                y = step >> 1;
            }
        }

        free(tmp);
    }

    return 0;
}

static void
render_frame_rect(gd_GIF *gif, uint8_t *buffer)
{
    int i, j, k;
    uint8_t index, *color;
    i = gif->fy * gif->width + gif->fx;

    for (j = 0; j < gif->fh; j++) {
        for (k = 0; k < gif->fw; k++) {
            index = gif->frame[(gif->fy + j) * gif->width + gif->fx + k];
            color = &gif->palette->colors[index * 3];

            if (!gif->gce.transparency || index != gif->gce.tindex) {
                memcpy(&buffer[(i + k) * 4], color, 3);
                buffer[(i + k) * 4 + 3] = 255; // opaque
            }
        }

        i += gif->width;
    }
}

static void
dispose(gd_GIF *gif)
{
    int i, j, k;
    uint8_t *bgcolor;

    switch (gif->gce.disposal) {
    case 2: /* Restore to background color. */
        bgcolor = &gif->palette->colors[gif->bgindex * 3];
        i = gif->fy * gif->width + gif->fx;

        for (j = 0; j < gif->fh; j++) {
            for (k = 0; k < gif->fw; k++) {
                memcpy(&gif->canvas[(i + k) * 4], bgcolor, 3);

                if (!gif->gce.transparency || gif->bgindex != gif->gce.tindex) {
                    gif->canvas[(i + k) * 4 + 3] = 255;    // opaque
                } else {
                    gif->canvas[(i + k) * 4 + 3] = 0;    // transparent
                }
            }

            i += gif->width;
        }

        break;

    case 3: /* Restore to previous, i.e., don't update canvas.*/
        break;

    default:
        /* Add frame non-transparent pixels to canvas. */
        render_frame_rect(gif, gif->canvas);
    }
}

/* Return 1 if got a frame; 0 if got GIF trailer; -1 if error. */
int
gd_get_frame(gd_GIF *gif)
{
    char sep;

    dispose(gif);
    read(gif->fd, &sep, 1);

    while (sep != ',') {
        if (sep == ';') {
            return 0;
        }

        if (sep == '!') {
            read_ext(gif);
        } else {
            return -1;
        }

        read(gif->fd, &sep, 1);
    }

    if (read_image(gif) == -1) {
        return -1;
    }

    return 1;
}

void
gd_render_frame(gd_GIF *gif, uint8_t *buffer)
{
    memcpy(buffer, gif->canvas, gif->width * gif->height * 4);
    render_frame_rect(gif, buffer);
}

void
gd_rewind(gd_GIF *gif)
{
    lseek(gif->fd, gif->anim_start, SEEK_SET);
}

void
gd_close_gif(gd_GIF *gif)
{
    close(gif->fd);
    free(gif);
}

#pragma GCC diagnostic pop
