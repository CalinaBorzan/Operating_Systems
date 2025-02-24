#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/mman.h>

#define COMMAND_BUFFER_SIZE 256

unsigned int header_size;
unsigned int version;
unsigned int no_of_sections;

typedef struct {
    unsigned int section_type;
    unsigned int section_offset;
    unsigned int section_size;
    char section_name[8];
} SECTIONS;

const char *resp_pipe_path = "RESP_PIPE_14180";
const char *req_pipe_path = "REQ_PIPE_14180";

int read_fd, write_fd;
unsigned int memory_space;
struct stat size;

int create_pipe(const char *path_name, mode_t acc_rights);
int open_for_reading(const char *path_name);
int open_for_writing(const char *path_name);
int write_in_pipe_string(int fd, const char *message);
int write_in_pipe_number(int fd, unsigned int num);
char *read_from_pipe_string(int fd);
unsigned int read_from_pipe_number(int fd);
void handleSharedMemoryCreation(unsigned int size, int write_fd);
void first();
int parseSimple(const char *filePath);
void writeInSharedMemory(unsigned int offset, unsigned int value, int write_fd, unsigned int memory_size, void *shared_memory);
void mapInMemory(char *name, int write_fd, unsigned int memory_space);
void readFromFile(unsigned int offset, unsigned int no_of_bytes, int write_fd);
void readFromFileSections(unsigned int offset_within_section, unsigned int num_bytes, unsigned int section_number, int write_fd);
void readFromLogicalSpace(unsigned int offset4,unsigned int no_of_bytes4, int write_fd);


SECTIONS sections[15];

unsigned int offset;
unsigned int offset2;
unsigned int offset3;
unsigned int offset4;
unsigned int no_of_bytes4;

unsigned int no_of_bytes;
unsigned int no_of_bytes3;
char *name;
unsigned int value;
void *shared_map_address;
void *shared_map_address2;
void *shared_memory;

int main() {
    char *command;

    if (create_pipe(resp_pipe_path, 0666) == -1) {
        perror("Error creating pipe");
        exit(1);
    }

    read_fd = open_for_reading(req_pipe_path);
    if (read_fd == -1) {
        perror("Error opening request pipe");
        exit(1);
    }

    write_fd = open_for_writing(resp_pipe_path);
    if (write_fd == -1) {
        perror("Error opening response pipe");
        exit(1);
    }

    write_in_pipe_string(write_fd, "HELLO!");

    while (1) {
        command = read_from_pipe_string(read_fd);
        if (command == NULL) {
            break;
        }

        printf("Debug: Received command: %s\n", command);

        if (strcmp(command, "PING") == 0) {
            first();
        } else if (strcmp(command, "CREATE_SHM") == 0) {
            memory_space = read_from_pipe_number(read_fd);
            printf("Debug: Received CREATE_SHM with size: %u\n", memory_space);
            handleSharedMemoryCreation(memory_space, write_fd);
        } 
        else if (strcmp(command, "WRITE_TO_SHM") == 0) {
            offset = read_from_pipe_number(read_fd);
            value = read_from_pipe_number(read_fd);
            printf("Debug: Received WRITE_TO_SHM with offset: %u and value: %u\n", offset, value);
            writeInSharedMemory(offset, value, write_fd, memory_space, shared_map_address);
        } 
        else if (strcmp(command, "MAP_FILE") == 0) {
            char *name = read_from_pipe_string(read_fd);
            printf("Debug: Received MAP_FILE with name: %s\n", name);
            mapInMemory(name, write_fd, memory_space);
            free(name); 
        } else if (strcmp(command, "READ_FROM_FILE_OFFSET") == 0) {
            offset2 = read_from_pipe_number(read_fd);
            no_of_bytes = read_from_pipe_number(read_fd);
            printf("Debug: Received READ_FROM_FILE_OFFSET with offset: %u and no_of_bytes: %u\n", offset2, no_of_bytes);
            readFromFile(offset2, no_of_bytes, write_fd);
        } else if (strcmp(command, "READ_FROM_FILE_SECTION") == 0) {
            unsigned int section_number = read_from_pipe_number(read_fd);
            offset3 = read_from_pipe_number(read_fd);
            no_of_bytes3 = read_from_pipe_number(read_fd);
            printf("Debug: Received READ_FROM_FILE_SECTION with section_number: %u, offset: %u, no_of_bytes: %u\n", section_number, offset3, no_of_bytes3);
            if (parseSimple(shared_map_address2) == 0) {
                readFromFileSections(offset3, no_of_bytes3, section_number, write_fd);
            } else {
                write_in_pipe_string(write_fd, "READ_FROM_FILE_SECTION!ERROR!");
            }
        } else if (strcmp(command, "READ_FROM_LOGICAL_SPACE_OFFSET") == 0) {
            offset4 = read_from_pipe_number(read_fd);
            no_of_bytes4 = read_from_pipe_number(read_fd);
            printf("Debug: Received READ_FROM_LOGICAL_SPACE_OFFSET with logical offset: %u and no_of_bytes: %u\n", offset4, no_of_bytes4);
            if (parseSimple(shared_map_address2) == 0) {
  readFromLogicalSpace(offset4, no_of_bytes4, write_fd);
              } else {
                write_in_pipe_string(write_fd, "READ_FROM_FILE_SECTION!ERROR!");
            }
          
        } else if (strcmp(command, "EXIT") == 0) {
            printf("Received EXIT!, closing...\n");
            break;
        }
        free(command);
    }

    if (shared_map_address) {
        munmap(shared_map_address, memory_space);
    }
    if (shared_map_address2) {
        munmap(shared_map_address2, size.st_size);
    }
    close(read_fd);
    close(write_fd);
    unlink(resp_pipe_path);

    return 0;
}

void first() {
    write_in_pipe_string(write_fd, "PING!");
    write_in_pipe_number(write_fd, 14180);
    write_in_pipe_string(write_fd, "PONG!");
}

char *read_from_pipe_string(int fd) {
    char *buffer = malloc(COMMAND_BUFFER_SIZE);
    if (!buffer) return NULL;

    int index = 0;
    char temp;

    while (index < COMMAND_BUFFER_SIZE - 1 && read(fd, &temp, 1) > 0) {
        if (temp == '!') break;
        buffer[index++] = temp;
    }
    buffer[index] = '\0';  

    if (index == 0) {
        free(buffer);
        return NULL;
    }

    return buffer;
}

unsigned int read_from_pipe_number(int fd) {
    unsigned int number;
    if (read(fd, &number, sizeof(number)) != sizeof(number)) {
        printf("Failed to read the expected number of bytes for an unsigned int.\n");
        exit(1);  // Consider exiting or handling this error more gracefully
    }
    return number;  // Consider byte order if necessary
}

int create_pipe(const char *path_name, mode_t acc_rights) {
    if (mkfifo(path_name, acc_rights) < 0) {
        perror("Error creating pipe");
        return -1;
    }
    return 0;
}

int open_for_reading(const char *path_name) {
    int fd = open(path_name, O_RDONLY);
    if (fd < 0) {
        perror("Error opening for reading the pipe");
        return -1;
    }
    return fd;
}

int open_for_writing(const char *path_name) {
    int fd = open(path_name, O_WRONLY);
    if (fd < 0) {
        perror("Error opening for writing the pipe");
        return -1;
    }
    return fd;
}

int write_in_pipe_string(int fd, const char *message) {
    if (write(fd, message, strlen(message)) == -1) {
        return -1;
    } else {
        return 0;
    }
}

int write_in_pipe_number(int fd, unsigned int num) {
    unsigned int net_num = num;
    if (write(fd, &net_num, sizeof(net_num)) == -1) {
        return -1;
    } else {
        return 0;
    }
}

void handleSharedMemoryCreation(unsigned int size, int write_fd) {
    int shm_fd = shm_open("/frWYQar", O_CREAT | O_RDWR, 0644);
    if (shm_fd < 0 || ftruncate(shm_fd, size) != 0) {
        perror("Failed to create or resize shared memory");
        write_in_pipe_string(write_fd, "CREATE_SHM!ERROR!");
    } else {
        shared_map_address = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
        if (shared_map_address == MAP_FAILED) {
            perror("Failed to map shared memory");
            write_in_pipe_string(write_fd, "CREATE_SHM!ERROR!");
        } else {
            write_in_pipe_string(write_fd, "CREATE_SHM!SUCCESS!");
        }
        close(shm_fd);
    }
}

void writeInSharedMemory(unsigned int offset, unsigned int value, int write_fd, unsigned int memory_size, void *shared_memory) {
    if (offset + sizeof(unsigned int) > memory_size) {
        write_in_pipe_string(write_fd, "WRITE_TO_SHM!ERROR!");
        return;
    }
    *(unsigned int *)(shared_map_address + offset) = value;
    write_in_pipe_string(write_fd, "WRITE_TO_SHM!SUCCESS!");
}

void mapInMemory(char *name, int write_fd, unsigned int memory_space) {
    int filename = open(name, O_RDONLY);
    if (filename == -1) {
        write_in_pipe_string(write_fd, "MAP_FILE!ERROR!");
        return;
    }
    if (fstat(filename, &size) == -1) {
        close(filename);
        write_in_pipe_string(write_fd, "MAP_FILE!ERROR!");
        return;
    }
    shared_map_address2 = mmap(NULL, size.st_size, PROT_READ, MAP_SHARED, filename, 0);
    if (shared_map_address2 == MAP_FAILED) {
        write_in_pipe_string(write_fd, "MAP_FILE!ERROR!");
    } else {
        write_in_pipe_string(write_fd, "MAP_FILE!SUCCESS!");
    }
    close(filename);
}

void readFromFile(unsigned int offset, unsigned int no_of_bytes, int write_fd) {
    if (!shared_map_address2) {
        write_in_pipe_string(write_fd, "READ_FROM_FILE_OFFSET!ERROR!");
        return;
    }
    if (offset + no_of_bytes > size.st_size) {
        write_in_pipe_string(write_fd, "READ_FROM_FILE_OFFSET!ERROR!");
        return;
    }
    memcpy(shared_map_address, (char *)shared_map_address2 + offset, no_of_bytes);
    write_in_pipe_string(write_fd, "READ_FROM_FILE_OFFSET!SUCCESS!");
    printf("Debug: Successfully read %u bytes from file offset %u to shared memory\n", no_of_bytes, offset);
}

int parseSimple(const char *mappedData) {
    const char *current = mappedData + size.st_size - 1; 
    if (*current != 'K') {
        printf("Debug: Magic byte 'K' not found, found '%c' instead\n", *current);
        return -1;
    }

    current -= 2;
    header_size = *(unsigned short *)current;
    printf("Debug: Header size is %u\n", header_size);

    if (header_size > size.st_size) {
        printf("Debug: Header size %u exceeds file size %lu\n", header_size, size.st_size);
        return -1;
    }

    current = mappedData + size.st_size - header_size;

    version = *(unsigned short *)current;
    printf("Debug: Version number is %u\n", version);
    current += 2;

    no_of_sections = *(unsigned char *)current;
    printf("Debug: Number of sections is %u\n", no_of_sections);
    current += 1;

    if (version < 37 || version > 96 || no_of_sections < 2 || no_of_sections > 14) {
        printf("Debug: Version or number of sections out of valid range\n");
        return -1;
    }

    for (int i = 0; i < no_of_sections; i++) {
        memcpy(sections[i].section_name, current, 8);
        sections[i].section_name[7] = '\0'; 
        current += 8;
        sections[i].section_type = *(unsigned short *)current; 
        current += 2;
        sections[i].section_offset = *(unsigned int *)current;
        current += 4;
        sections[i].section_size = *(unsigned int *)current;
        current += 4;

        if (sections[i].section_offset + sections[i].section_size > size.st_size) {
            printf("Debug: Section %d - Offset and size exceed file size (Offset: %u, Size: %u, File size: %lu)\n",
                   i, sections[i].section_offset, sections[i].section_size, size.st_size);
            return -1;
        }

        printf("Debug: Section %d - Name: %s, Type: %u, Offset: %u, Size: %u\n",
               i, sections[i].section_name, sections[i].section_type, sections[i].section_offset, sections[i].section_size);
    }

    return 0;
}

void readFromFileSections(unsigned int offset_within_section, unsigned int num_bytes, unsigned int section_number, int write_fd) {
    if (section_number == 0 || section_number > no_of_sections) {
        printf("Debug: Invalid section number %u\n", section_number);
        write_in_pipe_string(write_fd, "READ_FROM_FILE_SECTION!ERROR!");
        return;
    }

    SECTIONS section = sections[section_number - 1];
    if (offset_within_section + num_bytes > section.section_size) {
        printf("Debug: Requested bytes exceed section size (Offset: %u, Bytes: %u, Section size: %u)\n",
               offset_within_section, num_bytes, section.section_size);
        write_in_pipe_string(write_fd, "READ_FROM_FILE_SECTION!ERROR!");
        return;
    }

    unsigned int actual_offset = section.section_offset + offset_within_section;
    if (actual_offset + num_bytes > size.st_size) {
        printf("Debug: Actual offset and bytes exceed file size (Actual offset: %u, Bytes: %u, File size: %lu)\n",
               actual_offset, num_bytes, size.st_size);
        write_in_pipe_string(write_fd, "READ_FROM_FILE_SECTION!ERROR!");
        return;
    }

    memcpy(shared_map_address, (char *)shared_map_address2 + actual_offset, num_bytes);
    write_in_pipe_string(write_fd, "READ_FROM_FILE_SECTION!SUCCESS!");
    printf("Debug: Successfully read %u bytes from section %u, offset %u to shared memory\n",
           num_bytes, section_number, offset_within_section);
}
            

void readFromLogicalSpace(unsigned int logical_offset, unsigned int num_bytes, int write_fd) {
    unsigned int current_logical_offset = 0;
    unsigned int next_logical_offset = 0;
    int section_found = -1;

    for (int i = 0; i < no_of_sections; i++) {
        // Calculate the next logical offset (aligned to 3072 bytes)
        next_logical_offset = (current_logical_offset + sections[i].section_size + 3071) / 3072 * 3072;

        if (logical_offset >= current_logical_offset && logical_offset < next_logical_offset) {
            section_found = i;
            break;
        }
        
        current_logical_offset = next_logical_offset;
    }

    if (section_found == -1) {
        printf("Debug: Logical offset %u is out of bounds\n", logical_offset);
        write_in_pipe_string(write_fd, "READ_FROM_LOGICAL_SPACE_OFFSET!ERROR!");
        return;
    }

    SECTIONS section = sections[section_found];
    unsigned int offset_within_section = logical_offset - current_logical_offset;
    unsigned int actual_offset = section.section_offset + offset_within_section;

    if (offset_within_section + num_bytes > section.section_size) {
        printf("Debug: Requested bytes exceed section size (Offset within section: %u, Bytes: %u, Section size: %u)\n",
               offset_within_section, num_bytes, section.section_size);
        write_in_pipe_string(write_fd, "READ_FROM_LOGICAL_SPACE_OFFSET!ERROR!");
        return;
    }

    if (actual_offset + num_bytes > size.st_size) {
        printf("Debug: Actual offset and bytes exceed file size (Actual offset: %u, Bytes: %u, File size: %lu)\n",
               actual_offset, num_bytes, size.st_size);
        write_in_pipe_string(write_fd, "READ_FROM_LOGICAL_SPACE_OFFSET!ERROR!");
        return;
    }

    memcpy(shared_map_address, (char *)shared_map_address2 + actual_offset, num_bytes);
    write_in_pipe_string(write_fd, "READ_FROM_LOGICAL_SPACE_OFFSET!SUCCESS!");
    printf("Debug: Successfully read %u bytes from logical offset %u to shared memory\n", num_bytes, logical_offset);
}

