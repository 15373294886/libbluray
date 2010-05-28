/*
 * This file is part of libbluray
 * Copyright (C) 2010  hpi1
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders of libbluray
 * gives permission to link the code of its release of libbluray with the
 * OpenSSL project's "OpenSSL" library (or with modified versions of it
 * that use the same license as the "OpenSSL" library), and distribute
 * the linked executables.  You must obey the GNU General Public License
 * in all respects for all of the code used other than "OpenSSL".  If you
 * modify this file, you may extend this exception to your version of the
 * file, but you are not obligated to do so.  If you do not wish to do
 * so, delete this exception statement from your version.
 */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "../file/file.h"
#include "../util/bits.h"
#include "../util/logging.h"
#include "../util/macro.h"

#include "mobj_parse.h"


#define MOBJ_SIG1  ('M' << 24 | 'O' << 16 | 'B' << 8 | 'J')
#define MOBJ_SIG2A ('0' << 24 | '2' << 16 | '0' << 8 | '0')
#define MOBJ_SIG2B ('0' << 24 | '1' << 16 | '0' << 8 | '0')

static int _mobj_parse_header(BITSTREAM *bs, int *extension_data_start)
{
    uint32_t sig1, sig2;

    bs_seek_byte(bs, 0);

    sig1 = bs_read(bs, 32);
    sig2 = bs_read(bs, 32);

    if (sig1 != MOBJ_SIG1 ||
       (sig2 != MOBJ_SIG2A &&
        sig2 != MOBJ_SIG2B)) {
     DEBUG(DBG_NAV, "MovieObject.bdmv failed signature match: expected MOBJ0100 got %8.8s\n", bs->buf);
     return 0;
    }

    *extension_data_start = bs_read(bs, 32);

    return 1;
}

static int _mobj_parse_object(BITSTREAM *bs, MOBJ_OBJECT *obj)
{
    uint16_t num_cmds;
    int      i;

    obj->resume_intention_flag = bs_read(bs, 1);
    obj->menu_call_mask = bs_read(bs, 1);
    obj->title_search_mask = bs_read(bs, 1);

    bs_skip(bs, 13); /* padding */

    num_cmds = bs_read(bs, 16);

    obj->cmds = calloc(num_cmds, sizeof(MOBJ_CMD));
    obj->num_cmds = num_cmds;

    for (i = 0; i < obj->num_cmds; i++) {

        HDMV_INSN *insn = &obj->cmds[i].insn;

        insn->op_cnt     = bs_read(bs, 3);
        insn->grp        = bs_read(bs, 2);
        insn->sub_grp    = bs_read(bs, 3);

        insn->imm_op1    = bs_read(bs, 1);
        insn->imm_op2    = bs_read(bs, 1);
        bs_skip(bs, 2);    /* reserved */
        insn->branch_opt = bs_read(bs, 4);

        bs_skip(bs, 4);    /* reserved */
        insn->cmp_opt    = bs_read(bs, 4);

        bs_skip(bs, 3);    /* reserved */
        insn->set_opt    = bs_read(bs, 5);

        obj->cmds[i].dst = bs_read(bs, 32);
        obj->cmds[i].src = bs_read(bs, 32);
    }

    return 1;
}

void mobj_free(MOBJ_OBJECTS *objects)
{
    if (objects) {

        int i;
        for (i = 0 ; i < objects->num_objects; i++) {
            X_FREE(objects->objects[i].cmds);
        }

        X_FREE(objects);
    }
}

MOBJ_OBJECTS *mobj_parse(const char *file_name)
{
    BITSTREAM     bs;
    FILE_H       *fp;
    MOBJ_OBJECTS *objects = NULL;
    uint16_t      num_objects;
    uint32_t      data_len;
    int           extension_data_start, i;

    fp = file_open(file_name, "rb");
    if (!fp) {
      DEBUG(DBG_NAV | DBG_CRIT, "error opening %s\n", file_name);
      return NULL;
    }

    bs_init(&bs, fp);

    if (!_mobj_parse_header(&bs, &extension_data_start)) {
        DEBUG(DBG_NAV | DBG_CRIT, "%s: invalid header\n", file_name);
        file_close(fp);
        return NULL;
    }

    bs_seek_byte(&bs, 40);

    data_len = bs_read(&bs, 32);
    bs_skip(&bs, 32); /* reserved */
    num_objects = bs_read(&bs, 16);

    objects = calloc(1, sizeof(MOBJ_OBJECTS) + num_objects * sizeof(MOBJ_OBJECT));
    objects->num_objects = num_objects;

    for (i = 0; i < objects->num_objects; i++) {
        if (!_mobj_parse_object(&bs, &objects->objects[i])) {
            DEBUG(DBG_NAV | DBG_CRIT, "%s: error parsing object %d\n", file_name, i);
            mobj_free(objects);
            file_close(fp);
            return NULL;
        }
    }

    file_close(fp);

    return objects;
}
