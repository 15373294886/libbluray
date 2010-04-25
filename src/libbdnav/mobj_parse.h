#if !defined(_MOBJ_PARSE_H_)
#define _MOBJ_PARSE_H_

#include <stdio.h>
#include <stdint.h>

typedef struct {
  uint8_t op_cnt     : 3;  /* operand count */
  uint8_t grp        : 2;  /* command group */
  uint8_t sub_grp    : 3;  /* command sub-group */

  uint8_t imm_op1    : 1;  /* I-flag for operand 1 */
  uint8_t imm_op2    : 1;  /* I-flag for operand 2 */
  uint8_t reserved1  : 2;
  uint8_t branch_opt : 4;  /* branch option */

  uint8_t reserved2  : 4;
  uint8_t cmp_opt    : 4;  /* compare option */

  uint8_t reserved3  : 3;
  uint8_t set_opt    : 5;  /* set option */
} HDMV_INSN;

typedef struct {
    HDMV_INSN insn;
    uint32_t  dst;
    uint32_t  src;
} MOBJ_CMD;

typedef struct {
    uint8_t     resume_intention_flag : 1;
    uint8_t     menu_call_mask        : 1;
    uint8_t     title_search_mask     : 1;

    uint16_t    num_cmds;
    MOBJ_CMD   *cmds;
} MOBJ_OBJECT;

typedef struct {
    uint16_t    num_objects;
    MOBJ_OBJECT objects[];
} MOBJ_OBJECTS;


MOBJ_OBJECTS* mobj_parse(const char *path); /* parse MovieObject.bdmv */
void mobj_free(MOBJ_OBJECTS *index);

#endif // _MOBJ_PARSE_H_

