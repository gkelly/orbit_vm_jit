#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <jit/jit.h>
#include <jit/jit-dump.h>

#include "orbit_vm.h"

struct orbit_vm {
  uint32_t status_register;
  double data[ORBIT_VM_ADDRESS_SPACE_SIZE];
  double input_port[ORBIT_VM_ADDRESS_SPACE_SIZE];
  double output_port[ORBIT_VM_ADDRESS_SPACE_SIZE];

  jit_context_t context;
  jit_type_t cycle_function_parameters[4];
  jit_type_t cycle_function_signature;
  jit_function_t cycle_function;

  jit_value_t jit_program_value;
  jit_value_t jit_data_value;
  jit_value_t jit_input_port_value;
  jit_value_t jit_output_port_value;
  jit_value_t jit_status_register_value;

  void *data_ptr;
  void *input_port_ptr;
  void *output_port_ptr;
  void *call_parameters[4];
};

struct orbit_vm_instruction {
  uint32_t d_opcode;
  uint32_t s_opcode;
  uint32_t s_cmpz;
  jit_value_t offset;
  jit_value_t d_r1;
  jit_value_t d_r2;
  jit_value_t s_imm;
  jit_value_t s_r1;
};

typedef void (*orbit_vm_opcode_emitter_t)(struct orbit_vm *,
    struct orbit_vm_instruction *);

struct orbit_vm *orbit_vm_alloc() {
  struct orbit_vm *vm = malloc(sizeof(struct orbit_vm));

  vm->context = jit_context_create();
  jit_context_build_start(vm->context);

  int i;
  for (i = 0; i < 4; ++i) {
    vm->cycle_function_parameters[i] = jit_type_void_ptr;
  }
  vm->cycle_function_signature = jit_type_create_signature(jit_abi_cdecl,
      jit_type_void, vm->cycle_function_parameters, 4, 1);
  vm->cycle_function = jit_function_create(vm->context,
      vm->cycle_function_signature);

  vm->jit_data_value = jit_value_get_param(vm->cycle_function, 0);
  vm->data_ptr = vm->data;
  vm->call_parameters[0] = &vm->data_ptr;

  vm->jit_input_port_value = jit_value_get_param(vm->cycle_function, 1);
  vm->input_port_ptr = vm->input_port;
  vm->call_parameters[1] = &vm->input_port_ptr;

  vm->jit_output_port_value = jit_value_get_param(vm->cycle_function, 2);
  vm->output_port_ptr = vm->output_port;
  vm->call_parameters[2] = &vm->output_port_ptr;

  vm->jit_status_register_value = jit_value_get_param(vm->cycle_function, 3);
  vm->call_parameters[3] = &vm->status_register;

  return vm;
}

// TODO(gkelly): It takes more than this to actually free all of the memory
// associated with the JIT context.
void orbit_vm_free(struct orbit_vm *vm) {
  free(vm);
}

static void orbit_vm_emit_noop(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  // Oh my, look at this wonderful optimization.
}

static void orbit_vm_emit_cmpz(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_value_t zero_value = jit_value_create_float64_constant(vm->cycle_function,
      jit_type_float64, 0.0);
  jit_value_t value = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->s_r1, jit_type_float64);

  jit_value_t test_value;
  switch (instruction->s_cmpz) {
    case ORBIT_VM_S_CMPZ_LTZ:
      test_value = jit_insn_lt(vm->cycle_function, value, zero_value);
      break;
    case ORBIT_VM_S_CMPZ_LEZ:
      test_value = jit_insn_le(vm->cycle_function, value, zero_value);
      break;
    case ORBIT_VM_S_CMPZ_EQZ:
      test_value = jit_insn_eq(vm->cycle_function, value, zero_value);
      break;
    case ORBIT_VM_S_CMPZ_GEZ:
      test_value = jit_insn_ge(vm->cycle_function, value, zero_value);
      break;
    case ORBIT_VM_S_CMPZ_GTZ:
      test_value = jit_insn_gt(vm->cycle_function, value, zero_value);
      break;
  }

  jit_insn_store(vm->cycle_function, vm->jit_status_register_value, test_value);
}

static void orbit_vm_emit_sqrt(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_value_t value = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->s_r1, jit_type_float64);
  jit_value_t sqrt_value = jit_insn_sqrt(vm->cycle_function, value);
  jit_insn_store_elem(vm->cycle_function, vm->jit_data_value,
      instruction->offset, sqrt_value);
}

static void orbit_vm_emit_copy(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_value_t value = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->s_r1, jit_type_float64);
  jit_insn_store_elem(vm->cycle_function, vm->jit_data_value,
      instruction->offset, value);
}

static void orbit_vm_emit_input(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_value_t value = jit_insn_load_elem(vm->cycle_function,
      vm->jit_input_port_value, instruction->s_r1, jit_type_float64);
  jit_insn_store_elem(vm->cycle_function, vm->jit_data_value,
      instruction->offset, value);
}

static void orbit_vm_emit_add(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_value_t left = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->d_r1, jit_type_float64);
  jit_value_t right = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->d_r2, jit_type_float64);
  jit_value_t result = jit_insn_add(vm->cycle_function, left, right);
  jit_insn_store_elem(vm->cycle_function, vm->jit_data_value,
      instruction->offset, result);
}

static void orbit_vm_emit_sub(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_value_t left = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->d_r1, jit_type_float64);
  jit_value_t right = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->d_r2, jit_type_float64);
  jit_value_t result = jit_insn_sub(vm->cycle_function, left, right);
  jit_insn_store_elem(vm->cycle_function, vm->jit_data_value,
      instruction->offset, result);
}

static void orbit_vm_emit_mult(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_value_t left = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->d_r1, jit_type_float64);
  jit_value_t right = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->d_r2, jit_type_float64);
  jit_value_t result = jit_insn_mul(vm->cycle_function, left, right);
  jit_insn_store_elem(vm->cycle_function, vm->jit_data_value,
      instruction->offset, result);
}

static void orbit_vm_emit_div(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_value_t right = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->d_r2, jit_type_float64);
  jit_value_t zero = jit_value_create_float64_constant(vm->cycle_function,
      jit_type_float64, 0.0);
  jit_value_t right_zero = jit_insn_eq(vm->cycle_function, right, zero);
  
  jit_label_t skip_label = jit_label_undefined;
  jit_insn_branch_if(vm->cycle_function, right_zero, &skip_label);

  jit_value_t left = jit_insn_load_elem(vm->cycle_function, vm->jit_data_value,
      instruction->d_r1, jit_type_float64);
  jit_value_t result = jit_insn_div(vm->cycle_function, left, right);
  jit_insn_store_elem(vm->cycle_function, vm->jit_data_value,
      instruction->offset, result);

  jit_insn_label(vm->cycle_function, &skip_label);
}

static void orbit_vm_emit_output(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_value_t output = jit_insn_load_elem(vm->cycle_function,
      vm->jit_data_value, instruction->d_r2, jit_type_float64);
  jit_insn_store_elem(vm->cycle_function, vm->jit_output_port_value,
      instruction->d_r1, output);
}

static void orbit_vm_emit_phi(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  jit_label_t done_label = jit_label_undefined;
  jit_label_t skip_label = jit_label_undefined;

  jit_insn_branch_if_not(vm->cycle_function, vm->jit_status_register_value,
      &skip_label);
  jit_value_t d_r1_value = jit_insn_load_elem(vm->cycle_function,
      vm->jit_data_value, instruction->d_r1, jit_type_float64);
  jit_insn_store_elem(vm->cycle_function, vm->jit_data_value,
      instruction->offset, d_r1_value);
  jit_insn_branch(vm->cycle_function, &done_label);

  jit_insn_label(vm->cycle_function, &skip_label);
  jit_value_t d_r2_value = jit_insn_load_elem(vm->cycle_function,
      vm->jit_data_value, instruction->d_r2, jit_type_float64);
  jit_insn_store_elem(vm->cycle_function, vm->jit_data_value,
      instruction->offset, d_r2_value);

  jit_insn_label(vm->cycle_function, &done_label);
}

static orbit_vm_opcode_emitter_t orbit_vm_s_opcode_emitter[] = {
  orbit_vm_emit_noop,
  orbit_vm_emit_cmpz,
  orbit_vm_emit_sqrt,
  orbit_vm_emit_copy,
  orbit_vm_emit_input,
};

static void orbit_vm_emit_s_instruction(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  orbit_vm_s_opcode_emitter[instruction->s_opcode](vm, instruction);
}

static orbit_vm_opcode_emitter_t orbit_vm_d_opcode_emitter[] = {
  orbit_vm_emit_s_instruction,
  orbit_vm_emit_add,
  orbit_vm_emit_sub,
  orbit_vm_emit_mult,
  orbit_vm_emit_div,
  orbit_vm_emit_output,
  orbit_vm_emit_phi,
};

static void orbit_vm_emit_instruction(struct orbit_vm *vm,
    struct orbit_vm_instruction *instruction) {
  orbit_vm_d_opcode_emitter[instruction->d_opcode](vm, instruction); 
}

void orbit_vm_load(struct orbit_vm *vm, const char *path) {
  FILE *program = fopen(path, "rb"); 
  if (!program) {
    fprintf(stderr, "Can't load %s\n", path);
    return;
  }

  uint32_t instruction;
  int read_offset = 0;
  for (read_offset = 0; !feof(program); ++read_offset) {
    int read_size = 0;
    if (read_offset % 2 != 0) {
      read_size += fread(&instruction, sizeof(uint32_t), 1, program);
      read_size += fread(vm->data + read_offset, sizeof(double), 1, program);
    } else {
      read_size += fread(vm->data + read_offset, sizeof(double), 1, program);
      read_size += fread(&instruction, sizeof(uint32_t), 1, program);
    }
    if (read_size != 2) {
      break;
    }

    struct orbit_vm_instruction decoded_instruction = {
      .d_opcode = ORBIT_VM_D_OPCODE_MASK(instruction),
      .s_opcode = ORBIT_VM_S_OPCODE_MASK(instruction),
      .s_cmpz = ORBIT_VM_S_CMPZ_MASK(instruction),
      .offset = jit_value_create_nint_constant(vm->cycle_function,
          jit_type_uint, read_offset),
      .d_r1 = jit_value_create_nint_constant(vm->cycle_function,
          jit_type_uint, ORBIT_VM_D_R1_MASK(instruction)),
      .d_r2 = jit_value_create_nint_constant(vm->cycle_function,
          jit_type_uint, ORBIT_VM_D_R2_MASK(instruction)),
      .s_imm = jit_value_create_nint_constant(vm->cycle_function,
          jit_type_uint, ORBIT_VM_S_IMM_MASK(instruction)),
      .s_r1 = jit_value_create_nint_constant(vm->cycle_function,
          jit_type_uint, ORBIT_VM_S_R1_MASK(instruction)),
    };
    orbit_vm_emit_instruction(vm, &decoded_instruction);
  }

  jit_function_compile(vm->cycle_function);
  jit_context_build_end(vm->context);

  fclose(program);
}

void orbit_vm_run_iteration(struct orbit_vm *vm) {
  jit_function_apply(vm->cycle_function, vm->call_parameters, NULL);     
}

double orbit_vm_read_port(struct orbit_vm *vm, int port) {
  return vm->output_port[port];
}

void orbit_vm_write_port(struct orbit_vm *vm, int port, double value) {
  vm->input_port[port] = value;
}

void orbit_vm_print_status(struct orbit_vm *vm) {
  // Not imeplemented.
}

void orbit_vm_set_scenario(struct orbit_vm *vm, uint32_t scenario) {
  orbit_vm_write_port(vm, ORBIT_VM_PORT_CONFIG, scenario);
}
