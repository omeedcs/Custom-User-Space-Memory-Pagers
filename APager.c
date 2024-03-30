#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <elf.h>
#include <string.h>

// ELF magic numbers
#define EI_MAG0 0
#define EI_MAG1 1
#define EI_MAG2 2
#define EI_MAG3 3
#define ELFMAG0 0x7f
#define ELFMAG1 'E'
#define ELFMAG2 'L'
#define ELFMAG3 'F'

// Size of the ELF magic number
#define SELFMAG 4

// Had to add GNU property (elf.h did not have it on lab machine)
#define PT_GNU_PROPERTY 0x6474e553

int load_elf_binary(int argc, char *argv[]) {

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
    for (Elf64_Half i = 0; i < elf_header.e_phnum; ++i) {
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

        if (phdr.p_type == PT_LOAD) {

            int prot = PROT_READ | PROT_WRITE | PROT_EXEC;
            int flags = MAP_PRIVATE | MAP_ANON;
            size_t offset = phdr.p_vaddr % sysconf(_SC_PAGE_SIZE);
            size_t start_addr = phdr.p_vaddr - offset;
            size_t map_size =  offset + phdr.p_memsz;

            void* segment = mmap((void*) start_addr, map_size, prot, flags, -1, 0);
            if (segment == MAP_FAILED) {
                perror("Failed to mmap segment");
                return -1;
            }
            printf("mmap result: %p, requested start addr: %p, map size: %zu, protection flags: %d, file descriptor: %d, file offset: %d\n", segment, start_addr, map_size, prot, -1, 0);

            // seek to segment in the file and do some deallocation from that last portion.
            if (lseek(fd, phdr.p_offset, SEEK_SET) == (off_t)-1) {
                perror("Failed to seek to segment in file");
                munmap(segment, map_size);
                return -1;
            } 

            // cpy segment data from file to memory
            if (read(fd, (char*)segment + (phdr.p_vaddr - start_addr), phdr.p_filesz) != phdr.p_filesz) {
                perror("Failed to read segment from file");
                munmap(segment, map_size);
                return -1;
            } 
        } 
    }
}

int main(int argc, char *argv[]) {
    load_elf_binary(argc, argv);
    return 0;
}