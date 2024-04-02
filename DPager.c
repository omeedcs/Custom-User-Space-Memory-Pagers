#include <assert.h>
#include <elf.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <ucontext.h>

// ELF magic numbers
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'
#define PAGE_SIZE 4096
#define STACK_ALIGNMENT 16

extern char **environ;

// Size of the ELF magic number
#define SELFMAG 4

Elf64_Addr e_entry;
int global_fd;
Elf64_Ehdr elf_header;
Elf64_Phdr *ph;

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

    // Allocate memory for program headers
    ph = (Elf64_Phdr *)malloc(elf_header.e_phnum * sizeof(Elf64_Phdr));
    if (ph == NULL) {
        perror("Failed to allocate memory for program headers");
        close(fd);
        return 1;
    }

    // seeks to the start of the program headers
    if (lseek(fd, elf_header.e_phoff, SEEK_SET) == (off_t)-1) {
        perror("Failed to seek to program headers.\n");
    } else {
        printf("Successfully seeked program headers. \n");
    }

    // Read all program headers into memory
    if (read(fd, ph, elf_header.e_phnum * sizeof(Elf64_Phdr)) != elf_header.e_phnum * sizeof(Elf64_Phdr)) {
        perror("Failed to read program headers");
        free(ph);
        close(fd);
        return 1;
    }
    
   printf("Successfully read program headers.\n");
    header = &elf_header;
    global_fd = fd;
    printf("addr of elf_header %p\n", header);
    printf("Elf loading complete.\n");
    return 0;
}


/**
 * Routine for checking stack made for child program.
 * top_of_stack: stack pointer that will given to child program as %rsp
 * argc: Expected number of arguments
 * argv: Expected argument strings
 */
void stack_check(void* top_of_stack, uint64_t argc, char** argv) {
	printf("----- stack check -----\n");

	assert(((uint64_t)top_of_stack) % 8 == 0);
	printf("top of stack is 8-byte aligned\n");

	uint64_t* stack = top_of_stack;
	uint64_t actual_argc = *(stack++);
	printf("argc: %lu\n", actual_argc);
	assert(actual_argc == argc);

	for (int i = 0; i < argc; i++) {
		char* argp = (char*)*(stack++);
		assert(strcmp(argp, argv[i]) == 0);
		printf("arg %d: %s\n", i, argp);
	}
	// Argument list ends with null pointer
	assert(*(stack++) == 0);

	int envp_count = 0;
	while (*(stack++) != 0)
		envp_count++;

	printf("env count: %d\n", envp_count);

	Elf64_auxv_t* auxv_start = (Elf64_auxv_t*)stack;
	Elf64_auxv_t* auxv_null = auxv_start;
	while (auxv_null->a_type != AT_NULL) {
		auxv_null++;
	}
	printf("aux count: %lu\n", auxv_null - auxv_start);
	printf("----- end stack check -----\n");
}

#define DEFAULT_STACK_SIZE_KB 80

typedef struct {
  void *base;
  size_t size;
} stack_info_t;

int allocate_stack(stack_info_t *stack, size_t size_kb, void *desired_addr) {
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

void free_stack(stack_info_t *stack) {
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
  stack_info_t stack;
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

  stack_top =
      (char **)((stack_ptr & ~(STACK_ALIGNMENT - 1)) & ~(STACK_ALIGNMENT - 1));

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

  // Direct manipulation of env_vars_buffer within the loop
  for (int i = 0; i < num_env_vars - 1; ++i) {
    envp_ptr[i] = env_vars_buffer;
    env_vars_buffer +=
        strlen(env_vars_buffer) + 1;  
  }

  envp_ptr[num_env_vars - 1] = NULL;

  memcpy(stack_top, vectors, sizeof(Elf64_auxv_t) * aux_entries);
  stack_top += sizeof(Elf64_auxv_t) * aux_entries;

  stack_check((void *)stack_top_addr, argc, (char **)argv_ptr);
  printf("Stack setup completed\n");

  printf("Beginning to execute assembly code\n");

  // // using the neat trick described in handout:
  // // will use the ret instruction to perform a return, which will pop the
  // // address from the top of the stack and jump to that address.
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


// void segv_handler(int sig, siginfo_t *info, void *ucontext) {
//     void *fault_addr = info->si_addr;
//     printf("Handling SIGSEGV at address: %p\n", fault_addr);
//     printf("Global fd: %d\n", global_fd);

//     // Determine the system's page size
//     size_t page_size = sysconf(_SC_PAGE_SIZE);

//     // Iterate over program headers to find the relevant segment
//     for (int i = 0; i < elf_header.e_phnum; ++i) {
//         Elf64_Phdr phdr = ph[i];
//         if (phdr.p_type == PT_LOAD) {
//             uintptr_t start_addr = phdr.p_vaddr;
//             uintptr_t end_addr = start_addr + phdr.p_memsz;
//             if (fault_addr >= (void *)start_addr && fault_addr < (void *)end_addr) {
//                 // Fault address is within the current loadable segment
//                 int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
//                 int flags = MAP_PRIVATE | MAP_ANON;

//                 // Calculate the page-aligned start address and adjusted map size
//                 uintptr_t page_aligned_start_addr = start_addr & ~(page_size - 1);
//                 size_t offset = start_addr - page_aligned_start_addr;
//                 size_t adjusted_map_size = phdr.p_memsz + offset;

//                 // Use page_aligned_start_addr for mmap
//                 void *segment = mmap((void *)page_aligned_start_addr, adjusted_map_size, prot, flags, -1, 0);
//                 if (segment == MAP_FAILED) {
//                     perror("Failed to mmap segment");
//                     exit(1);
//                 }

//                 // Copy segment data from the ELF file into the mapped memory
//                 if (pread(global_fd, segment + offset, phdr.p_filesz, phdr.p_offset) != phdr.p_filesz) {
//                     perror("Failed to read segment data");
//                     exit(1);
//                 }

//                 printf("mmap result: %p, page-aligned start addr: %p, map size: %zu, protection flags: %d\n",
//                        segment, (void *)page_aligned_start_addr, adjusted_map_size, prot);
//                 return; // Successfully handled the segmentation fault
//             }
//         }
//     }

//     // If we reach here, the fault address was not within a loadable segment
//     fprintf(stderr, "Invalid memory access at address: %p\n", fault_addr);
//     exit(1);
// }



void segv_handler(int sig, siginfo_t *info, void *ucontext) {
    // Print basic information about the signal received
    printf("Received signal: %d\n", sig);
    
    // Access the faulting address from the siginfo_t structure
    void *fault_addr = info->si_addr;
    // if (fault_addr == NULL) {
    //     force_seg_fault(); 
    // }
    printf("Handling SIGSEGV at address: %p\n", fault_addr);
    printf("Global fd: %d\n", global_fd);

    // Determine the system's page size for memory mapping
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    printf("System page size: %zu bytes\n", page_size);

    // Iterate over program headers to find if the fault address falls within a segment
    int segment_found = 0;
    for (int i = 0; i < elf_header.e_phnum; i++) {
        Elf64_Phdr phdr = ph[i]; 

        // Check if the program header is for a loadable segment
        if (phdr.p_type == PT_LOAD) {
            uintptr_t start_addr = phdr.p_vaddr;
            uintptr_t end_addr = start_addr + phdr.p_memsz;

            // Check if the fault address is within the segment
            if (fault_addr >= (void *)start_addr && fault_addr < (void *)end_addr) {
                segment_found = 1;
                printf("Fault address is within segment [%d]: %p - %p\n", i, (void *)start_addr, (void *)end_addr);

                int prot = PROT_READ | PROT_WRITE | PROT_EXEC; 
                int flags = MAP_PRIVATE | MAP_ANON;  

                // Calculate the page-aligned address of the faulting page
                uintptr_t page_aligned_fault_addr = (uintptr_t)fault_addr & ~(page_size - 1);

                // Calculate the file offset for mapping, ensuring it's page-aligned
                off_t file_offset = phdr.p_offset + (page_aligned_fault_addr - start_addr);

                // Map only the page that caused the fault
                void *segment = mmap((void *)page_aligned_fault_addr, page_size, prot, flags, -1, 0);
                if (segment == MAP_FAILED) {
                    perror("Failed to mmap segment");
                    exit(1);
                }
                
                // Check if the segment data is large enough to read
                if (phdr.p_filesz + phdr.p_vaddr >= page_aligned_fault_addr) {

                    // Calculate the size of data to read based on file and memory sizes
                    size_t read_size = phdr.p_filesz - (page_aligned_fault_addr - start_addr);
                    if (read_size > page_size) {
                        read_size = page_size;
                    }

                    // Read the segment data from the file into the mapped area
                    if (pread(global_fd, page_aligned_fault_addr, read_size, file_offset) != read_size) {
                        perror("Failed to read segment data");
                        exit(1);
                    }
    
                    printf("Mapped and read segment successfully. Address: %p, Size: %zu bytes\n", segment, read_size);
                    printf("Offset: %ld\n", file_offset);
                    return;
                } else {
                    printf("Mapped segment successfully. Address: %p, Size: %zu bytes\n", segment, 0);
                    return;
                }

            }
        }
    }

    if (!segment_found) {
        fprintf(stderr, "Invalid memory access at address: %p\n", fault_addr);
        fprintf(stderr, "Fault address does not fall within any loadable segment\n");
        exit(1);
    }
}

// void force_seg_fault() {
//   int *zero = NULL;
//   return *zero;
// }

void setup_signal_handler() {
  struct sigaction sa;
  memset(&sa, 0, sizeof(struct sigaction));
  sa.sa_sigaction = segv_handler;
  sa.sa_flags = SA_SIGINFO;
  if (sigaction(SIGSEGV, &sa, NULL) == -1) {
    perror("Failed to set up signal handler");
    exit(1);
  }
}

int main(int argc, char *argv[], char *envp[]) {
  Elf64_Ehdr header;
  load_elf_binary(argc, argv, &header);
  setup_signal_handler();
  setup_the_stack(argc - 1, &argv[1], envp, &header);
  return 0;
}

