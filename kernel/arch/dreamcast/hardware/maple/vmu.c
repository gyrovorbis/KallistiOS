/* KallistiOS ##version##

   vmu.c
   Copyright (C)2002,2003 Dan Potter
   Copyright (C)2008 Donald Haase
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <kos/thread.h>
#include <kos/genwait.h>
#include <dc/maple.h>
#include <dc/maple/vmu.h>
#include <dc/biosfont.h>
#include <dc/vmufs.h>
#include <arch/timer.h>

/*
   This module deals with the VMU.  It provides functionality for
   filesystem, LCD screen, buzzer, and date/time access.

   Thanks to Marcus Comstedt for VMU/Maple information.
 */

static int vmu_attach(maple_driver_t *drv, maple_device_t *dev) {
    (void)drv;
    dev->status_valid = 1;
    return 0;
}

static void vmu_poll_reply(maple_frame_t* frm) { 
    maple_response_t   *resp;
    uint32             *respbuf;
    vmu_cond_t         *raw;
    vmu_state_t        *cooked;

    /* Unlock the frame now (it's ok, we're in an IRQ) */
    maple_frame_unlock(frm);

    /* Make sure we got a valid response */
    resp = (maple_response_t *)frm->recv_buf;

    if(resp->response != MAPLE_RESPONSE_DATATRF)
        return;

    respbuf = (uint32 *)resp->data;

    if(respbuf[0] != MAPLE_FUNC_CLOCK)
        return;

    /* Update the status area from the response */
    if(frm->dev) {
        /* Verify the size of the frame and grab a pointer to it */
        assert(sizeof(vmu_cond_t) == ((resp->data_len - 1) * 4));
        raw = (vmu_cond_t *)(respbuf + 1);

        /* Fill the "nice" struct from the raw data */
        cooked = (vmu_state_t *)(frm->dev->status);
        /* Invert raw struct as nice struct */
        *cooked = ~(*raw);
        frm->dev->status_valid = 1;
    }
}

static int vmu_poll(maple_device_t *dev) {
    uint32 * send_buf;

    /* Only query for button input on the front VMU of each controller. */
    if(dev->unit == 1) {

        if(maple_frame_lock(&dev->frame) < 0)
            return 0;

        maple_frame_init(&dev->frame);
        send_buf = (uint32 *)dev->frame.recv_buf;
        send_buf[0] = MAPLE_FUNC_CLOCK;
        dev->frame.cmd = MAPLE_COMMAND_GETCOND;
        dev->frame.dst_port = dev->port;
        dev->frame.dst_unit = dev->unit;
        dev->frame.length = 1;
        dev->frame.callback = vmu_poll_reply;
        dev->frame.send_buf = send_buf;
        maple_queue_frame(&dev->frame);

    }

    return 0;
}

static void vmu_periodic(maple_driver_t *drv) {
    maple_driver_foreach(drv, vmu_poll);
}

/* Device Driver Struct */
static maple_driver_t vmu_drv = {
    .functions = MAPLE_FUNC_MEMCARD | MAPLE_FUNC_LCD | MAPLE_FUNC_CLOCK,
    .name = "VMU Driver",
    .periodic = NULL,
    .attach = vmu_attach,
    .detach = NULL
};

/* Add the VMU to the driver chain */
int vmu_init() {
    if(!vmu_drv.drv_list.le_prev)
        return maple_driver_reg(&vmu_drv);
    return -1;
}

void vmu_shutdown() {
    maple_driver_unreg(&vmu_drv);
}

/* Dynamically add the periodic polling callback to the driver when button input is enabled. */
void vmu_set_buttons_enabled(maple_device_t* dev, int enable) {
    vmu_drv.periodic = (enable != 0)? vmu_periodic : NULL; 
}

/* Determine whether polling for button input is enabled or not by presence of periodic callback. */
int vmu_get_buttons_enabled() { 
    return vmu_drv.periodic? 1 : 0;
}

int vmu_use_custom_color(maple_device_t * dev, int enable) {
    vmu_root_t root;

    if(vmufs_root_read(dev, &root) < 0)
        return -1;

    /* 1 - Enables the use of the custom color. 0 - Disables */
    root.use_custom = (enable != 0) ? 1 : 0;

    if(vmufs_root_write(dev, &root) < 0)
        return -1;

    return 0;
}

/* The custom color is used while navigating the Dreamcast's file manager.
   You set the RGBA parameters, each with valid range of 0-255 */
int vmu_set_custom_color(maple_device_t * dev, uint8 red, uint8 green, uint8 blue, uint8 alpha) {
    vmu_root_t root;

    if(vmufs_root_read(dev, &root) < 0)
        return -1;

    /* 1 - Enables the use of the custom color. 0 - Disables */
    root.use_custom = 1;
    root.custom_color[0] = blue;
    root.custom_color[1] = green;
    root.custom_color[2] = red;
    root.custom_color[3] = alpha;

    if(vmufs_root_write(dev, &root) < 0)
        return -1;

    return 0;
}

/* The icon shape is used while navigating the BIOS menu. The values
   for icon_shape are listed in the biosfont.h and start with
   BFONT_ICON_VMUICON. */
int vmu_set_icon_shape(maple_device_t * dev, uint8 icon_shape) {
#ifdef _arch_sub_naomi
    vmu_root_t root;

    if(icon_shape < BFONT_ICON_VMUICON || icon_shape > BFONT_ICON_EMBROIDERY)
        return -1;

    if(vmufs_root_read(dev, &root) < 0)
        return -1;

    /* Valid value range is 0-123 and starts with BFONT_ICON_VMUICON which
       has a value of 5.  This is because we cant use the first 5 icons
       found in the bios so we must subtract 5 */
    root.icon_shape = icon_shape - BFONT_ICON_VMUICON;

    if(vmufs_root_write(dev, &root) < 0)
        return -1;

    return 0;
#else
    (void)dev;
    (void)icon_shape;
    return -1;
#endif
}

/* These interfaces will probably change eventually, but for now they
   can stay the same */

/* Callback that unlocks the frame, general use */
static void vmu_gen_callback(maple_frame_t * frame) {
    /* Unlock the frame for the next usage */
    maple_frame_unlock(frame);

    /* Wakey, wakey! */
    genwait_wake_all(frame);
}

/* Set the tone to be generated by the VMU's speaker.
   Only last two bytes are used. Actual parameters unknown
   except that the last byte must be larger than the second
   to last byte. This might necessitate refactoring as the
   clock is a seperate device from the screen and storage. */
int vmu_beep_raw(maple_device_t * dev, uint32 beep) {
    uint32 *    send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CLOCK;
    send_buf[1] = beep;
    dev->frame.cmd = MAPLE_COMMAND_SETCOND;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = vmu_gen_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the timer to accept it */
    if(genwait_wait(&dev->frame, "vmu_beep_raw", 500, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_VACANT)  {
            /* Something went wrong.... */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_beep_raw: timeout to unit %c%c, beep: %lu\n",
                   dev->port + 'A', dev->unit + '0', beep);
            return MAPLE_ETIMEOUT;
        }
    }

    return MAPLE_EOK;
}

int vmu_beep(maple_device_t* dev, uint8_t period, uint8_t pulseWidth) {
    const uint32_t rawBeep = ((period<<24)|((period-pulsewidth))<<16);
    return vmu_beep_raw(dev, rawBeep);
}

/* Draw a 1-bit bitmap on the LCD screen (48x32). return a -1 if
   an error occurs */
int vmu_draw_lcd(maple_device_t * dev, void *bitmap) {
    uint32 *    send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_LCD;
    send_buf[1] = 0;    /* Block / phase / partition */
    memcpy(send_buf + 2, bitmap, 48 * 4);
    dev->frame.cmd = MAPLE_COMMAND_BWRITE;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2 + 48;
    dev->frame.callback = vmu_gen_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the LCD to accept it */
    if(genwait_wait(&dev->frame, "vmu_draw_lcd", 500, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_VACANT) {
            /* It's probably never coming back, so just unlock the frame */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_draw_lcd: timeout to unit %c%c\n",
                   dev->port + 'A', dev->unit + '0');
            return MAPLE_ETIMEOUT;
        }
    }

    return MAPLE_EOK;
}

/* This function converts a xbm image to a 1-bit bitmap that can
   be displayed on LCD screen of VMU */
static void vmu_xbm_to_bitmap(uint8 *bitmap, const char *vmu_icon) {
    int x, y, xi, xb;
    memset(bitmap, 0, 48 * 32 / 8);

    if(vmu_icon) {
        for(y = 0; y < 32; y++)
            for(x = 0; x < 48; x++) {
                xi = x / 8;
                xb = 0x80 >> (x % 8);

                if(vmu_icon[(31 - y) * 48 + (47 - x)] == '.')
                    bitmap[y * (48 / 8) + xi] |= xb;
            }
    }
}

int vmu_draw_lcd_xbm(maple_device_t * dev, const char *vmu_icon) {
    uint8   bitmap[48 * 32 / 8];
    vmu_xbm_to_bitmap(bitmap, vmu_icon);

    return vmu_draw_lcd(dev, bitmap);
}

/* Read the data in block blocknum into buffer, return a -1
   if an error occurs, for now we ignore MAPLE_RESPONSE_FILEERR,
   which will be changed shortly */
static void vmu_block_read_callback(maple_frame_t *frm) {
    /* Wakey, wakey! */
    genwait_wake_all(frm);
}

int vmu_block_read(maple_device_t * dev, uint16 blocknum, uint8 *buffer) {
    maple_response_t    *resp;
    int         rv;
    uint32          *send_buf;
    uint32          blkid;

    assert(dev != NULL);

    /* Lock the frame. XXX: Priority inversion issues here. */
    while(maple_frame_lock(&dev->frame) < 0)
        thd_pass();

    /* This is (block << 24) | (phase << 8) | (partition (0 for all vmu)) */
    blkid = ((blocknum & 0xff) << 24) | ((blocknum >> 8) << 16);

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MEMCARD;
    send_buf[1] = blkid;
    dev->frame.cmd = MAPLE_COMMAND_BREAD;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = vmu_block_read_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the VMU to accept it */
    if(genwait_wait(&dev->frame, "vmu_block_read", 100, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_RESPONDED) {
            /* It's probably never coming back, so just unlock the frame */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_block_read: timeout to unit %c%c, block %d\n",
                   dev->port + 'A', dev->unit + '0', (int)blocknum);
            return MAPLE_ETIMEOUT;
        }
    }

    if(dev->frame.state != MAPLE_FRAME_RESPONDED) {
        dbglog(DBG_ERROR, "vmu_block_read: incorrect state for unit %c%c, block %d (%d)\n",
               dev->port + 'A', dev->unit + '0', (int)blocknum, dev->frame.state);
        dev->frame.state = MAPLE_FRAME_VACANT;
        return MAPLE_EFAIL;
    }

    /* Copy out the response */
    resp = (maple_response_t *)dev->frame.recv_buf;
    send_buf = (uint32 *)resp->data;

    if(resp->response != MAPLE_RESPONSE_DATATRF
            || send_buf[0] != MAPLE_FUNC_MEMCARD
            || send_buf[1] != blkid) {
        rv = MAPLE_EFAIL;
        dbglog(DBG_ERROR, "vmu_block_read failed: %s(%d)/%08lx\r\n",
               maple_perror(resp->response), resp->response, send_buf[0]);
    }
    else {
        rv = MAPLE_EOK;
        memcpy(buffer, send_buf + 2, (resp->data_len - 2) * 4);
    }

    maple_frame_unlock(&dev->frame);

    return rv;
}

/* writes buffer into block blocknum.  ret a -1 on error.  We don't do anything about the
   maple bus returning file errors, etc, right now, but that will change soon. */
static void vmu_block_write_callback(maple_frame_t *frm) {
    /* Reset the frame status (but still keep it for us to use) */
    frm->state = MAPLE_FRAME_UNSENT;

    /* Wakey, wakey! */
    genwait_wake_all(frm);
}
static int vmu_block_write_internal(maple_device_t * dev, uint16 blocknum, uint8 *buffer) {
    maple_response_t    *resp;
    int         rv, phase, r;
    uint32          *send_buf;
    uint32          blkid;

    assert(dev != NULL);

    /* Assume success */
    rv = MAPLE_EOK;

    /* Lock the frame. XXX: Priority inversion issues here. */
    while(maple_frame_lock(&dev->frame) < 0)
        thd_pass();

    /* Writes have to occur in four phases per block -- this is the
       way of flash memory, which you must erase an entire block
       at once to write; the blocks in this case are 128 bytes long. */
    for(phase = 0; phase < 4; phase++) {
        /* this is (block << 24) | (phase << 8) | (partition (0 for all vmu)) */
        blkid = ((blocknum & 0xff) << 24) | ((blocknum >> 8) << 16) | (phase << 8);

        /* Reset the frame */
        maple_frame_init(&dev->frame);
        send_buf = (uint32 *)dev->frame.recv_buf;
        send_buf[0] = MAPLE_FUNC_MEMCARD;
        send_buf[1] = blkid;
        memcpy(send_buf + 2, buffer + 128 * phase, 128);
        dev->frame.cmd = MAPLE_COMMAND_BWRITE;
        dev->frame.dst_port = dev->port;
        dev->frame.dst_unit = dev->unit;
        dev->frame.length = 2 + (128 / 4);
        dev->frame.callback = vmu_block_write_callback;
        dev->frame.send_buf = send_buf;
        maple_queue_frame(&dev->frame);

        /* Wait for the VMU to accept it */
        if(genwait_wait(&dev->frame, "vmu_block_write", 100, NULL) < 0) {
            if(dev->frame.state != MAPLE_FRAME_UNSENT) {
                /* It's probably never coming back, so just unlock the frame */
                dev->frame.state = MAPLE_FRAME_VACANT;
                dbglog(DBG_ERROR, "vmu_block_write: timeout to unit %c%c, block %d\n",
                       dev->port + 'A', dev->unit + '0', (int)blocknum);
                return MAPLE_ETIMEOUT;
            }
        }

        if(dev->frame.state != MAPLE_FRAME_UNSENT) {
            dbglog(DBG_ERROR, "vmu_block_read: incorrect state for unit %c%c, block %d (%d)\n",
                   dev->port + 'A', dev->unit + '0', (int)blocknum, dev->frame.state);
            dev->frame.state = MAPLE_FRAME_VACANT;
            return MAPLE_EFAIL;
        }

        /* Check the response */
        resp = (maple_response_t *)dev->frame.recv_buf;
        r = resp->response;

        if(r != MAPLE_RESPONSE_OK) {
            rv = MAPLE_EFAIL;
            dbglog(DBG_ERROR, "Incorrect response writing phase %d:\n", phase);
            dbglog(DBG_ERROR, "response:      %s(%d)\n", maple_perror(resp->response), resp->response);
            dbglog(DBG_ERROR, "datalen:       %d\n", resp->data_len);
        }
    }

    /* Finally a "sync" command -- thanks Nagra */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_MEMCARD;
    send_buf[1] = ((blocknum & 0xff) << 24)
                  | (((blocknum >> 8) & 0xff) << 16)
                  | (4 << 8);
    dev->frame.cmd = MAPLE_COMMAND_BSYNC;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = vmu_block_write_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the VMU to accept it */
    if(genwait_wait(&dev->frame, "vmu_block_write", 100, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_UNSENT) {
            /* It's probably never coming back, so just unlock the frame */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_block_write: timeout to unit %c%c, block %d\n",
                   dev->port + 'A', dev->unit + '0', (int)blocknum);
            return MAPLE_ETIMEOUT;
        }
    }

    if(dev->frame.state != MAPLE_FRAME_UNSENT) {
        dbglog(DBG_ERROR, "vmu_block_read: incorrect state for unit %c%c, block %d (%d)\n",
               dev->port + 'A', dev->unit + '0', (int)blocknum, dev->frame.state);
        dev->frame.state = MAPLE_FRAME_VACANT;
        return MAPLE_EFAIL;
    }

    dev->frame.state = MAPLE_FRAME_VACANT;

    return rv;
}

/* Sometimes a flaky or stubborn card can be recovered by trying a couple
   of times... */
int vmu_block_write(maple_device_t * dev, uint16 blocknum, uint8 *buffer) {
    int i, rv;

    for(i = 0; i < 4; i++) {
        // Try the write.
        rv = vmu_block_write_internal(dev, blocknum, buffer);

        if(rv == MAPLE_EOK)
            return rv;

        /* It failed -- wait a bit and try again. */
        thd_sleep(100);
    }

    /* Well, looks like it's really toasty... return the most recent
       error. */
    return rv;
}

/* Utility function which sets the icon on all available VMUs
   from an Xwindows XBM. Imported from libdcutils. */
void vmu_set_icon(const char *vmu_icon) {
    int i = 0;
    maple_device_t * dev;
    uint8   bitmap[48 * 32 / 8];

    vmu_xbm_to_bitmap(bitmap, vmu_icon);

    while((dev = maple_enum_type(i++, MAPLE_FUNC_LCD))) {
        vmu_draw_lcd(dev, bitmap);
    }
}

int vmu_set_time(maple_device_t * dev, vmu_time_t* time) {
    uint32 *    send_buf;

    assert(dev != NULL);

    /* Lock the frame */
    if(maple_frame_lock(&dev->frame) < 0)
        return MAPLE_EAGAIN;

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CLOCK;
    memcpy(send_buf + 2, time, sizeof(vmu_time));

    dev->frame.cmd = MAPLE_COMMAND_BWRITE;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 4;
    dev->frame.callback = vmu_gen_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the timer to accept it */
    if(genwait_wait(&dev->frame, "vmu_set_time", 500, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_VACANT)  {
            /* Something went wrong.... */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_set_time: timeout to unit %c%c\n",
                   dev->port + 'A', dev->unit + '0');
            return MAPLE_ETIMEOUT;
        }
    }

    return MAPLE_EOK;
}

int vmu_get_time(maple_device_t * dev, vmu_time_t* time) {
    maple_response_t *resp;
    int               rv;
    uint32           *send_buf;

    assert(dev != NULL);

    /* Lock the frame. XXX: Priority inversion issues here. */
    while(maple_frame_lock(&dev->frame) < 0)
        thd_pass();

    /* Reset the frame */
    maple_frame_init(&dev->frame);
    send_buf = (uint32 *)dev->frame.recv_buf;
    send_buf[0] = MAPLE_FUNC_CLOCK;

    dev->frame.cmd = MAPLE_COMMAND_BREAD;
    dev->frame.dst_port = dev->port;
    dev->frame.dst_unit = dev->unit;
    dev->frame.length = 2;
    dev->frame.callback = vmu_gen_callback;
    dev->frame.send_buf = send_buf;
    maple_queue_frame(&dev->frame);

    /* Wait for the VMU to accept it */
    if(genwait_wait(&dev->frame, "vmu_get_time", 100, NULL) < 0) {
        if(dev->frame.state != MAPLE_FRAME_RESPONDED) {
            /* It's probably never coming back, so just unlock the frame */
            dev->frame.state = MAPLE_FRAME_VACANT;
            dbglog(DBG_ERROR, "vmu_get_time: timeout to unit %c%c\n",
                   dev->port + 'A', dev->unit + '0');
            return MAPLE_ETIMEOUT;
        }
    }

    if(dev->frame.state != MAPLE_FRAME_RESPONDED) {
        dbglog(DBG_ERROR, "vmu_get_time: incorrect state for unit %c%c (%d)\n",
               dev->port + 'A', dev->unit + '0', dev->frame.state);
        dev->frame.state = MAPLE_FRAME_VACANT;
        return MAPLE_EFAIL;
    }

    /* Copy out the response */
    resp = (maple_response_t *)dev->frame.recv_buf;
    send_buf = (uint32 *)resp->data;

    if(resp->response != MAPLE_RESPONSE_DATATRF
            || send_buf[0] != MAPLE_FUNC_CLOCK) {
        rv = MAPLE_EFAIL;
        dbglog(DBG_ERROR, "vmu_get_time failed: %s(%d)/%08lx\r\n",
               maple_perror(resp->response), resp->response, send_buf[0]);
    }
    else {
        rv = MAPLE_EOK;
        memcpy(time, send_buf + 2, sizeof(vmu_time_t));
    }

    maple_frame_unlock(&dev->frame);

    return rv;
}
