#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

unsigned short header_size;
unsigned short version;
unsigned short no_of_sections;
typedef struct
{
    unsigned short section_type;
    unsigned int section_offset;
    unsigned int section_size;
    char section_name[8];
} SECTIONS;

void modes(char a[10],mode_t mode)
{
    strcpy(a,"---------");
   if(mode & S_IRUSR)
   {
    a[0]='r';
   }
   if(mode & S_IWUSR)
   {a[1]='w';}
   if(mode & S_IXUSR)
   {
    a[2]='x';
   }

   if(mode & S_IRGRP)
   {
    a[3]='r';
   }
   if(mode & S_IWGRP)
   {a[4]='w';}
   if(mode & S_IXGRP)
   {
    a[5]='x';
   }

   if(mode & S_IROTH)
   {
    a[6]='r';
   }
   if(mode & S_IWOTH)
   {a[7]='w';}
   if(mode & S_IXOTH)
   {
    a[8]='x';
   }

}

void listDirectory(const char *path, const char *filterName, const char *filterPerm, int recursive)
{
    
    DIR *directory;
    struct dirent *entry;
    static int depth = 0;
    char fullPath[1024];
   struct stat info;  
 
    if (!(directory = opendir(path)))
    {
        printf("ERROR\ninvalid directory path\n");
        return;
    }
    if (depth == 0)
    {
        printf("SUCCESS\n");
    }
    depth++;
    while ((entry = readdir(directory)) != NULL)
    {  char permissions[11];
        
        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
        if (filterName && strncmp(entry->d_name, filterName, strlen(filterName)) != 0)
            continue;
       
          if(stat(fullPath,&info)!=0)
          continue;
          modes(permissions,info.st_mode);
        
                    if(filterPerm && strcmp(permissions,filterPerm)!=0)
                    continue;

        printf("%s\n", fullPath);
        
                if (recursive)
                {
               
                    if(S_ISDIR(info.st_mode))
                    {listDirectory(fullPath, filterName, filterPerm, recursive);
                                    }                
                
        }
    }
        closedir(directory);
        depth--;
    
}

int parseSimple(const char *filePath, SECTIONS *sections, unsigned short *no_of_sections)
{
    char MAGIC;
    unsigned short header_size, version;
    int file = open(filePath, O_RDONLY);
    if (file == -1)
    {
     
        close(file);
        return -1; 
    }

   
    lseek(file, -1, SEEK_END);
    read(file, &MAGIC, 1);
    if (MAGIC != 'K')
    {
        
        close(file);
        return -1;
    }
    lseek(file, -3, SEEK_END);
    read(file, &header_size, 2);

    lseek(file, -header_size, SEEK_END);
    read(file, &version, 2);
    read(file, no_of_sections, 1);

    if (version < 37 || version > 96)
    {
        
        close(file);
        return -1;
    }

    if (*no_of_sections < 2 || *no_of_sections > 14)
    {
        
        close(file);
        return -1;
    }

    
    for (int i = 0; i < *no_of_sections; i++)
    {
        read(file, sections[i].section_name, 8);
        read(file, &sections[i].section_type, 2);
        read(file, &sections[i].section_offset, 4);
        read(file, &sections[i].section_size, 4);
    }
     unsigned short type_valid[] = {60, 25, 53, 46, 59};
    for (int i = 0; i < *no_of_sections; i++)
    {
        int valid = 0;
        for (int j = 0; j < 5; j++)
        {
            if (sections[i].section_type == type_valid[j])
            {
                valid = 1;
                break;
            }
        }
        if (!valid)
        {
            
            close(file);
            return -1;
        }
    }
    close(file);
    return 1; 
}

int parseFile(char *filePath)
{
    char MAGIC;
    int file = open(filePath, O_RDONLY);
    if (file == -1)
    {
        printf("Error opening file");
        close(file);
        return -1;
    }

    off_t sizefile = lseek(file, 0, SEEK_END); 
    lseek(file, -1, SEEK_END);
    read(file, &MAGIC, 1);
    if (MAGIC != 'K')
    {
        printf("ERROR\nwrong magic\n");
        close(file);
        return -1;
    }

    lseek(file, -3, SEEK_END);
    read(file, &header_size, 2);
    if ((off_t)header_size > sizefile - 1)
    {
        printf("ERROR\nInvalid header_size\n");
        close(file);
        return -1;
    }

    lseek(file, -header_size, SEEK_END);
    read(file, &version, 2);
    read(file, &no_of_sections, 1);
    if (version < 37 || version > 96)
    {
        printf("ERROR\nwrong version\n");
        close(file);
        return -1;
    }

    if (no_of_sections < 2 || (no_of_sections > 14))
    {
        printf("ERROR\nwrong sect_nr\n");
        close(file);
        return -1;
    }

    SECTIONS sections[no_of_sections];
    for (int i = 0; i < no_of_sections; i++)
    {
        read(file, sections[i].section_name, 8);
        read(file, &sections[i].section_type, 2);
        read(file, &sections[i].section_offset, 4);
        read(file, &sections[i].section_size, 4);
    }

    unsigned short type_valid[] = {60, 25, 53, 46, 59};
    for (int i = 0; i < no_of_sections; i++)
    {
        int valid = 0;
        for (int j = 0; j < 5; j++)
        {
            if (sections[i].section_type == type_valid[j])
            {
                valid = 1;
                break;
            }
        }
        if (!valid)
        {
            printf("ERROR\nwrong sect_types\n");
            close(file);
            return -1;
        }
    }

    printf("SUCCESS\n");
    printf("version=%hu\n", version);
    printf("nr_sections=%hu\n", no_of_sections);
    for (int i = 0; i < no_of_sections; i++)
    {
        printf("section%d: %.*s %hu %u\n", i + 1, 8, sections[i].section_name, sections[i].section_type, sections[i].section_size);
    }

    close(file);
    return 0;
}

void extract(const char *filePath, unsigned short sect_nr, unsigned short line_nr)
{

    SECTIONS sections[15];
    unsigned short no_of_sections = 0;
    if (!parseSimple(filePath, sections, &no_of_sections) || sect_nr == 0 || sect_nr > no_of_sections)
    {
        printf("ERROR\nParsing file failed or section number out of range.\n");
        return;
    }
    int fd = open(filePath, O_RDONLY);
    if (fd < 0)
    {
        perror("ERROR\nnopening file\n");
        return;
    }

    SECTIONS section = sections[sect_nr - 1];
    if (lseek(fd, section.section_offset, SEEK_SET) == (off_t)-1)
    {
        printf("ERROR\nSeeking to section failed.\n");
        close(fd);
        return;
    }
    char ch;
    unsigned short currentLine = 0;
    char *lineBuffer = (char *)malloc(1000000 * sizeof(char)); 
    int bufferIndex = 0;
    int foundLine = 0;
    printf("SUCCESS\n");

    while (read(fd, &ch, 1) == 1 && currentLine <= line_nr)
    {
      
        
        if (ch == '\n' || bufferIndex == section.section_size - 1 || ch=='\0')
        {
            currentLine++;
            if (currentLine == line_nr)
            {
                for (int i = bufferIndex - 1; i >= 0; i--)
                {
                    printf("%c", lineBuffer[i]);
                }
                printf("\n");
                foundLine = 1;
                break;
            }
            bufferIndex = 0; 
        }
        else
        {
            if (ch != '\r')
            {
                lineBuffer[bufferIndex] = ch;
                bufferIndex++;
            }
        }
    }
    if (!foundLine)
    {
        printf("ERROR: Line not found.\n");
            free(lineBuffer);

    }

    free(lineBuffer);
    close(fd);
}
void FindAll(const char *path, int IsTop) {
    DIR *directory;
    struct dirent *entry;
    char fullPath[100000];
    struct stat entry_stat;

    if (!(directory = opendir(path))) {
        if (IsTop) {
            printf("ERROR\ninvalid directory path");
        }
        return;
    }

    if (IsTop) {
        printf("SUCCESS\n");
    }

    
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; 
        }

        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        if (stat(fullPath, &entry_stat) == -1) {
            continue; 
        }

       
        if (S_ISREG(entry_stat.st_mode)) {
            

            SECTIONS sections[15]; 
            unsigned short nr_sections = 0;
            if (parseSimple(fullPath, sections, &nr_sections) == 1) {
                for (int i = 0; i < nr_sections; i++) {
                    if (sections[i].section_type == 60) {
                        printf("%s\n", fullPath); 
                        break; 
                    }
                }
            }
        }
    }

    
    rewinddir(directory);

    
    while ((entry = readdir(directory)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue; 
        }

        snprintf(fullPath, sizeof(fullPath), "%s/%s", path, entry->d_name);
        if (stat(fullPath, &entry_stat) == -1) {
            continue; 
        }

      
        if (S_ISDIR(entry_stat.st_mode)) {
            FindAll(fullPath, 0); 
        }
    }

    closedir(directory);
}


int manualStringToUnsignedShort(const char *str)
{
    int result = 0;
    while (*str)
    {
        if (*str < '0' || *str > '9')
            return -1;
        result = result * 10 + (*str - '0');
        str++;
    }
    return result;
}

int main(int argc, char *argv[])
{
    if (argc > 1)
    {
        if (strcmp(argv[1], "variant") == 0)
        {
            printf("14180\n");
            return 0;
        }
        else if (argc > 2 && strcmp(argv[1], "parse") == 0)
        {
            char *pathPrefix = "path=";
            char *filePath = NULL;

            if (strncmp(argv[2], pathPrefix, strlen(pathPrefix)) == 0)
            {
               
                filePath = argv[2] + strlen(pathPrefix);
            }

            if (filePath)
            {
                
                return parseFile(filePath);
            }
            else
            {
                printf("Invalid or missing path.\n");
                return -1;
            }
        }
        else if (argc > 2 && strcmp(argv[1], "extract") == 0)
        {
            char *filePath = NULL;
            int sect_nr = 0, line_nr = 0;

            for (int i = 2; i < argc; i++)
            {
                if (strncmp(argv[i], "path=", 5) == 0)
                {
                    filePath = argv[i] + 5; 
                }
                else if (strncmp(argv[i], "section=", 8) == 0)
                {
                    sect_nr = manualStringToUnsignedShort(argv[i] + 8); 
                }
                else if (strncmp(argv[i], "line=", 5) == 0)
                {
                    line_nr = manualStringToUnsignedShort(argv[i] + 5); 
                }
            }

            if (!filePath || sect_nr <= 0 || line_nr <= 0)
            {
                printf("ERROR\nMissing or invalid arguments for extract command.\n");
                return -1;
            }

            extract(filePath, (unsigned short)sect_nr, (unsigned short)line_nr);
        }
     else if (argc > 2 && strcmp(argv[1], "findall") == 0)
        {
            char *pathPrefix = "path=";
            char *filePath = NULL;

            if (strncmp(argv[2], pathPrefix, strlen(pathPrefix)) == 0)
            {
               
                filePath = argv[2] + strlen(pathPrefix);
            }

            if (filePath)
            {
                
                 FindAll(filePath,1);
            }
            else
            {
                printf("Invalid or missing path.\n");
                return -1;
            }
        }
        else if (strcmp(argv[1], "list") == 0)
        {
            char *path = NULL;
            char *filterName=NULL;
            char *filterPerm = NULL;
            int recursive = 0;

            for (int i = 2; i < argc; i++)
            {
                if (strncmp(argv[i], "name_starts_with=", 17) == 0)
                {
                    filterName = argv[i] + 17;
                }
                else if (strncmp(argv[i], "path=", 5) == 0)
                {
                    path = argv[i] + 5;
                }
               
                else if (strncmp(argv[i], "permissions=", 12) == 0)
                {
                    filterPerm = argv[i] + 12;
                }
                else if (strcmp(argv[i], "recursive") == 0)
                {
                    recursive = 1;
                }
            }

            if (path == NULL)
            {
                printf("Error: Path is required.\n");
                return 1;
            }

            listDirectory(path, filterName, filterPerm, recursive);
            return 0;
        }
    }
    if (argc < 1)
    {
        printf("Usage: %s [variant|parse path=<file_path>]\n", argv[0]);

        printf("Usage: %s variant\n", argv[0]);
        printf("       %s parse path=<file_path>\n", argv[0]);
        printf("       %s list [recursive] [name starts with=<name>] [permissions=<perms>] path=<path>\n", argv[0]);
        return -1;
    }
}