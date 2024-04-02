#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

// ELF magic numbers
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

#define DEFAULT_STACK_SIZE_KB 80

#define STACK_ALIGNMENT 16

extern char **environ;

Elf64_Addr e_entry;

// Size of the ELF magic number
#define SELFMAG 4

// Had to add GNU property (elf.h did not have it on lab machine)
#define PT_GNU_PROPERTY 0x6474e553


int load_elf_binary(int argc, char *argv[], Elf64_Ehdr *header) {
  // for command line argument!
  if (argc < 2) {
    printf("Usage: %s <executable>\n", argv[0]);
    return 1;
  }

  // Open the ELF file
  int fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    perror("Failed to open file");
    return 1;
  } else {
    printf("Successfully opened file. \n");
  }

  // Read the ELF header
  Elf64_Ehdr elf_header;
  if (read(fd, &elf_header, sizeof(Elf64_Ehdr)) != sizeof(Elf64_Ehdr)) {
    perror("Failed to read ELF header");
    close(fd);
    return 1;
  } else {
    printf("Successfully read ELF header. \n");
  }
  e_entry = elf_header.e_entry;
  // check to see if we are dealing with an elf file!
  unsigned char elf_magic[SELFMAG] = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3};
  if (memcmp(elf_header.e_ident, elf_magic, SELFMAG) == 0) {
    printf("We are dealing with an ELF file. \n");
  } else {
    return 0;
  }

  Elf64_Phdr pheaders[elf_header.e_phnum];

  // seeks to the start of the program headers
  if (lseek(fd, elf_header.e_phoff, SEEK_SET) == (off_t)-1) {
    perror("Failed to seek to program headers.\n");
  } else {
    printf("Successfully seeked program headers. \n");
  }

  int i = 0;
  // read and load all program headers into memory.
  for (int i = 0; i < elf_header.e_phnum; ++i) {
    if (read(fd, &pheaders[i], sizeof(Elf64_Phdr)) != sizeof(Elf64_Phdr)) {
      perror("Failed to read a program header.\n");
      i += 1;
    }
  }

  if (i == 0) {
    printf("Successfully read program header. \n");
  }

  // iterate over the program headers of the ELF file
  for (int i = 0; i < elf_header.e_phnum; ++i) {
    // Access the i-th program header
    Elf64_Phdr phdr = pheaders[i];


    // If the program header is a loadable segment, map it into memory
    if (phdr.p_type == PT_LOAD) {
      int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
      int flags = MAP_PRIVATE | MAP_ANON;
      size_t offset = phdr.p_vaddr % sysconf(_SC_PAGE_SIZE);
      size_t start_addr = phdr.p_vaddr - offset;
      size_t map_size = offset + phdr.p_memsz;

      void *segment = mmap((void *)start_addr, map_size, prot, flags, -1, 0);
      if (segment == MAP_FAILED) {
        perror("Failed to mmap segment");
        return -1;
      }
      printf(
          "mmap result: %p, requested start addr: %p, map size: %zu, "
          "protection flags: %d, file descriptor: %d, file offset: %d\n",
          segment, start_addr, map_size, prot, -1, 0);

      // seek to segment in the file and do some deallocation from that last
      // portion.
      if (lseek(fd, phdr.p_offset, SEEK_SET) == (off_t)-1) {
        perror("Failed to seek to segment in file");
        munmap(segment, map_size);
        return -1;
      }

      // cpy segment data from file to memory
      if (read(fd, (char *)segment + (phdr.p_vaddr - start_addr),
               phdr.p_filesz) != phdr.p_filesz) {
        perror("Failed to read segment from file");
        munmap(segment, map_size);
        return -1;
      }
    }
  }
  header = &elf_header;
  printf("addr of elf_header %p \n", &header);
  printf("Elf loading complete. \n");
}

/**
 * Routine for checking stack made for child program.
 * top_of_stack: stack pointer that will given to child program as %rsp
 * argc: Expected number of arguments
 * argv: Expected argument strings
 */
void stack_check(void *top_of_stack, uint64_t argc, char **argv) {
  printf("----- stack check -----\n");

  assert(((uint64_t)top_of_stack) % 8 == 0);
  printf("top of stack is 8-byte aligned\n");

  uint64_t *stack = top_of_stack;
  uint64_t actual_argc = *(stack++);
  printf("argc: %lu\n", actual_argc);
  assert(actual_argc == argc);

  for (int i = 0; i < argc; i++) {
    char *argp = (char *)*(stack++);
    assert(strcmp(argp, argv[i]) == 0);
    printf("arg %d: %s\n", i, argp);
  }
  // Argument list ends with null pointer
  assert(*(stack++) == 0);

  int envp_count = 0;
  while (*(stack++) != 0) envp_count++;

  printf("env count: %d\n", envp_count);

  Elf64_auxv_t *auxv_start = (Elf64_auxv_t *)stack;
  Elf64_auxv_t *auxv_null = auxv_start;
  while (auxv_null->a_type != AT_NULL) {
    auxv_null++;
  }
  printf("aux count: %lu\n", auxv_null - auxv_start);
  printf("----- end stack check -----\n");
}


typedef struct {
  void *base;
  size_t size;
} stack_t;

int allocate_stack(stack_t *stack, size_t size_kb, void *desired_addr) {
  long page_size = sysconf(_SC_PAGESIZE);
  if (page_size == -1) {
    perror("Failed to get page size");
    return -1;
  }


  size_t stack_size = size_kb * 1024;
  if (stack_size % page_size != 0) {
    stack_size = ((stack_size / page_size) + 1) * page_size;
  }

  void *stack_base = mmap(desired_addr, stack_size, PROT_READ | PROT_WRITE,
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (stack_base == MAP_FAILED) {
    perror("Failed to allocate stack");
    return -1;
  }

  stack->base = stack_base;
  stack->size = stack_size;
  return 0;
}

void free_stack(stack_t *stack) {
  if (stack->base != NULL) {
    if (munmap(stack->base, stack->size) == -1) {
      perror("Failed to free stack");
    }
    stack->base = NULL;
    stack->size = 0;
  }
}

int setup_the_stack(int argc, char *argv[], char *envp[],
                    Elf64_Ehdr *elf_header) {
  stack_t stack;
  void *desired_addr = (void *)0x7000000;
  size_t stack_size_kb = DEFAULT_STACK_SIZE_KB;

  // allocate the stack
  if (allocate_stack(&stack, stack_size_kb, desired_addr) == -1) {
    fprintf(stderr, "Failed to allocate stack\n");
    return 1;
  }

  printf("Stack allocated successfully:\n");
  printf("  Base address: %p\n", stack.base);
  printf("  Size: %zu bytes\n", stack.size);

  void *stack_top = (void *)(stack.base + stack.size);
  int num_env_vars = count_env_vars();
  printf("Number of environment variables: %d\n", num_env_vars);

  // Allocate memory for env_vars strings
  size_t env_vars_total_len = 0;
  for (int i = 0; envp[i] != NULL; i++) {
    env_vars_total_len += strlen(envp[i]) + 1;
  }
  char *env_vars_buffer = (char *)malloc(env_vars_total_len);
  char *env_vars_ptr = env_vars_buffer;
  for (int i = 0; envp[i] != NULL; i++) {
    size_t len = strlen(envp[i]);
    memcpy(env_vars_ptr, envp[i], len + 1);
    envp[i] = env_vars_ptr;
    env_vars_ptr += len + 1;
  }

  // Allocate memory for cmd_args strings
  size_t cmd_args_total_len = 0;
  for (int i = 0; i < argc; i++) {
    cmd_args_total_len += strlen(argv[i]) + 1;
  }
  char *cmd_args_buffer = (char *)malloc(cmd_args_total_len);
  char *cmd_args_ptr = cmd_args_buffer;
  for (int i = 0; i < argc; i++) {
    size_t len = strlen(argv[i]);
    memcpy(cmd_args_ptr, argv[i], len + 1);
    argv[i] = cmd_args_ptr;
    cmd_args_ptr += len + 1;
  }

  // calculate the address of auxv using pointer arithmetic
  Elf64_auxv_t *auxv = (Elf64_auxv_t *)(envp + num_env_vars + 1);

  // count the number of auxiliary vector entries
  int aux_entries = count_auxv_entries(auxv);

  printf("Number of auxiliary vector entries: %d\n", aux_entries);

  Elf64_auxv_t *vectors =
      (Elf64_auxv_t *)malloc(aux_entries * sizeof(Elf64_auxv_t));
  if (vectors == NULL) {
    perror("Failed to allocate auxiliary vector");
    return -1;
  }
  memset(vectors, 0, aux_entries * sizeof(Elf64_auxv_t));

  Elf64_auxv_t *auxv_ptr = (Elf64_auxv_t *)auxv;
  memcpy(vectors, auxv_ptr, aux_entries * sizeof(Elf64_auxv_t));
  size_t stack_ptr = (size_t)stack_top;
  stack_ptr -= aux_entries * sizeof(Elf64_auxv_t);
  stack_ptr -= (argc + num_env_vars + 2) * sizeof(char *);

  stack_top = (char **)((stack_ptr & ~(STACK_ALIGNMENT - 1)) & ~(STACK_ALIGNMENT - 1));

  Elf64_Addr stack_top_addr = (Elf64_Addr)stack_top;
  *(long *)stack_top = (long)argc;
  stack_top += sizeof(long);

  char **argv_ptr = (char **)stack_top;
  stack_top += sizeof(char *) * argc;

  argv_ptr[0] = cmd_args_buffer;
  cmd_args_buffer +=
      strlen(cmd_args_buffer) + 1;  
  *(char **)stack_top = NULL;
  stack_top += sizeof(char *);

  char **envp_ptr = (char **)stack_top;
  stack_top += sizeof(char *) * num_env_vars;

  for (int i = 0; i < num_env_vars - 1; ++i) {
    envp_ptr[i] = env_vars_buffer;
    env_vars_buffer +=
        strlen(env_vars_buffer) + 1; 
  }

  envp_ptr[num_env_vars - 1] = NULL;

  memcpy(stack_top, vectors, sizeof(Elf64_auxv_t) * aux_entries);
  stack_top += sizeof(Elf64_auxv_t) * aux_entries;

  stack_check((void *)stack_top_addr, argc, (char **)argv_ptr);
  // printf("Stack setup completed\n");
  // print_stack_image((void *)stack_top_addr, argc, argv, envp, vectors);

  // printf("Beginning to execute assembly code\n");

  // using the neat trick described in handout:
  // will use the ret instruction to perform a return, which will pop the
  // address from the top of the stack and jump to that address.
  asm("xor %rax, %rax");
  asm("xor %rbx, %rbx");
  asm("xor %rcx, %rcx");
  asm("xor %rdx, %rdx");
  asm("mov %0, %%rsp" : : "r"(stack_top_addr));
  asm("push %0" : : "r"(e_entry));
  asm("ret");

  free_stack(&stack);

  return 0;
}

void print_stack_image(void *top_of_stack, uint64_t argc, char **argv,
                       char **envp, Elf64_auxv_t *auxv) {
  printf(
      "position            content                     size (bytes) + "
      "comment\n");
  printf(
      "------------------------------------------------------------------------"
      "\n");
  printf("stack pointer ->  [ argc = %lu args ]     8\n", argc);

  for (uint64_t i = 0; i < argc; ++i) {
    printf("                  [ argv[%lu] (pointer) ]         8\n", i);
  }
  printf("                  [ argv[n] (pointer) ]         8   (= NULL)\n");

  for (int i = 0; envp[i] != NULL; ++i) {
    printf("                  [ envp[%d] (pointer) ]         8\n", i);
  }
  printf("                  [ envp[term] (pointer) ]      8   (= NULL)\n");

  for (int i = 0; auxv[i].a_type != 0; ++i) {
    printf("                  [ auxv[%d] (Elf64_auxv_t) ]    16\n", i);
  }
  printf(
      "                  [ auxv[term] (Elf64_auxv_t) ] 16   (= AT_NULL "
      "vector)\n");

  // Padding and actual string data not easily represented without specific
  // stack details
  printf("                  [ padding ]                   0 - 16\n");
  printf("                  [ argument ASCIIZ strings ]   >= 0\n");
  printf("                  [ environment ASCIIZ str. ]   >= 0\n");
  printf("(0xbffffffc)      [ end marker ]                8   (= NULL)\n");
  printf("(0xc0000000)      < bottom of stack >           0   (virtual)\n");
}

int count_auxv_entries(Elf64_auxv_t *auxv) {
  int count = 0;
  while (auxv->a_type != AT_NULL) {
    count++;
    auxv++;
  }
  return count;
}

int count_env_vars_recursive(char **env) {
  if (*env == NULL) {
    return 0;
  }

  return 1 + count_env_vars_recursive(env + 1);
}

int count_env_vars() { return count_env_vars_recursive(environ); }

int main(int argc, char *argv[], char *envp[]) {
  Elf64_Ehdr header;
  load_elf_binary(argc, argv, &header);
  setup_the_stack(argc - 1, &argv[1], envp, &header);
  return 0;
}