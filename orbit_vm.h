#ifndef ORBIT_VM_H__
#define ORBIT_VM_H__

#include <stdint.h>

#define ORBIT_VM_ADDRESS_SPACE_SIZE (2 << 14)

// TODO(gkelly): This really shouldn't be this transparent, but time is tight.
#define ORBIT_VM_D_OPCODE_S_OP     0x00
#define ORBIT_VM_D_OPCODE_ADD      0x01
#define ORBIT_VM_D_OPCODE_SUB      0x02
#define ORBIT_VM_D_OPCODE_MULT     0x03
#define ORBIT_VM_D_OPCODE_DIV      0x04
#define ORBIT_VM_D_OPCODE_OUTPUT   0x05
#define ORBIT_VM_D_OPCODE_PHI      0x06
#define ORBIT_VM_S_OPCODE_NOOP     0x00
#define ORBIT_VM_S_OPCODE_CMPZ     0x01
#define ORBIT_VM_S_OPCODE_SQRT     0x02
#define ORBIT_VM_S_OPCODE_COPY     0x03
#define ORBIT_VM_S_OPCODE_INPUT    0x04
#define ORBIT_VM_S_CMPZ_LTZ        0x00
#define ORBIT_VM_S_CMPZ_LEZ        0x01
#define ORBIT_VM_S_CMPZ_EQZ        0x02
#define ORBIT_VM_S_CMPZ_GEZ        0x03
#define ORBIT_VM_S_CMPZ_GTZ        0x04

#define ORBIT_VM_D_OPCODE_MASK(_x) ((_x >> 28) & 0x0F)
#define ORBIT_VM_D_R1_MASK(_x)     ((_x >> 14) & 0x3FFF)
#define ORBIT_VM_D_R2_MASK(_x)     (_x & 0x3FFF)

#define ORBIT_VM_S_OPCODE_MASK(_x) ((_x >> 24) & 0x0F)
#define ORBIT_VM_S_IMM_MASK(_x)    ((_x >> 14) & 0x3F)
#define ORBIT_VM_S_CMPZ_MASK(_x)   ((_x >> 21) & 0x07)
#define ORBIT_VM_S_R1_MASK(_x)     (_x & 0x3FFF)

// Input Ports
#define ORBIT_VM_PORT_ACC_X          0x02
#define ORBIT_VM_PORT_ACC_Y          0x03
#define ORBIT_VM_PORT_CONFIG         0x3E80

// Output Ports
#define ORBIT_VM_PORT_SCORE          0x00
#define ORBIT_VM_PORT_FUEL           0x01
#define ORBIT_VM_PORT_POS_X          0x02
#define ORBIT_VM_PORT_POS_Y          0x03
#define ORBIT_VM_PORT_TARGET_RADIUS  0x04 // P 1
#define ORBIT_VM_PORT_TARGET_X       0x04 // P 2
#define ORBIT_VM_PORT_TARGET_Y       0x05 // P 2

struct orbit_vm;

struct orbit_vm *orbit_vm_alloc();
void   orbit_vm_free(struct orbit_vm *vm);
void   orbit_vm_load(struct orbit_vm *vm, const char *path);
void   orbit_vm_run_iteration(struct orbit_vm *vm);
double orbit_vm_read_port(struct orbit_vm *vm, int port);
void   orbit_vm_write_port(struct orbit_vm *vm, int port, double value);
void   orbit_vm_set_scenario(struct orbit_vm *vm, uint32_t scenario);
void   orbit_vm_print_status(struct orbit_vm *vm);
void   orbit_vm_report(struct orbit_vm *vm);
void   orbit_vm_print_port(struct orbit_vm *vm, int port);

#endif
