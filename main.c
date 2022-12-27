/* ========================================================================== */
/*                                                                            */
/*   File: main.c                                                         */
/*   Author: saveld                                                           */
/*                                                                            */
/*   Created: 23.10.2022                                                      */
/*                                                                            */
/* ========================================================================== */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <ostream>
#include <iostream>
#include <cstring>

using namespace std;

const int32_t FAT_UNUSED = INT32_MAX - 1;
const int32_t FAT_FILE_END = INT32_MAX - 2;
const int32_t FAT_BAD_CLUSTER = INT32_MAX - 3;
const int32_t MAX_DIR_IMMERSION = 5;
const int32_t FAT_MAX_SIZE = 65536;

struct description {
    char signature[20];              //login autora FS
    int32_t disk_size;              //celkova velikost VFS
    int32_t cluster_size;           //velikost clusteru
    int32_t cluster_count;          //pocet clusteru
    int32_t fat_count;        	    //pocet polozek kazde FAT tabulce
    int32_t fat1_start_address;	    //adresa pocatku FAT1 tabulky
    int32_t data_start_address;     //adresa pocatku datovych bloku (hl. adresar)
    int32_t dir_entry_count;
    bool formated;
};

struct directory_item {
    char item_name[13];              //8+3 + /0 C/C++ ukoncovaci string znak
    bool isFile;		     //identifikace zda je soubor (TRUE), nebo adres?? (FALSE)
    int32_t size;                    //velikost souboru, u adres??e 0 (bude zabirat jeden blok)
    int32_t start_cluster;           //po??te?n? cluster položky
};

struct description desc;

char path_actual[200];

int getPrikaz(char* vstup);

int getEmptyCl(int32_t* fat_tab) {
    for (int i = 0; i < desc.cluster_count; i++) {
        if (fat_tab[i] == FAT_UNUSED) {
            return i;
        }
    }
    return -1;
}

void printFile(directory_item dir, int32_t *fat_tab, FILE* file) {
    int ptr = dir.start_cluster;
    int start_adress = desc.data_start_address + desc.cluster_size;
    int adress;
    char out[desc.cluster_size];   
    char* output;
    do {
        memset(out, '\0', sizeof(out));
        adress = start_adress + ptr * desc.cluster_size;
        fseek(file, adress, SEEK_SET);
        fread(&out, 1, sizeof(out), file);        
        printf("%s", out);
        ptr = fat_tab[ptr];      
    } while (ptr != FAT_FILE_END);
    printf("\n");
}


void get_fat(FILE* file, int32_t *fat_tab) {
    int offset = desc.fat1_start_address;
    fseek(file, offset, SEEK_SET);
    fread(fat_tab, sizeof(int32_t) * desc.cluster_count, 1, file);
}

directory_item get_directory_item(FILE* file, char* name) {
    struct directory_item dir;
    for (int i = 0; i < desc.dir_entry_count; i++) {
        int offset = desc.data_start_address + sizeof(directory_item) * i;
        fseek(file, offset, SEEK_SET);
        fread(&dir, sizeof(struct directory_item), 1, file);     
        if (strcmp(name, dir.item_name) == 0) {     
            return dir;
        }
    }
    memset(dir.item_name, '\0', sizeof(dir.item_name));
    return dir;
}

int isValidPath(FILE* file, char* path[], int length) {
    struct directory_item dir;
    int adress = desc.data_start_address + desc.cluster_size;
    char content[desc.cluster_size];
    char* subdirs[desc.cluster_size];
    char* token;
    int index;
    int ok;
    for (int i = 0; i < length; i++) {
        ok = 0;
        dir = get_directory_item(file, path[i]);
        adress = desc.data_start_address + desc.cluster_size;
        adress += dir.start_cluster * desc.cluster_size;
        fseek(file, adress, SEEK_SET);
        fread(&content, sizeof(content), 1, file);
        token = strtok(content, ",");
        index = 0;
        while (token != NULL) {
            subdirs[index] = token;
            index++;
            token = strtok(NULL, ",");
        }
        if (index == 0) {
            if (strcmp(content, path[i + 1]) == 0) {
                ok = 1;
            }
        }    
        
        for (int j = 0; j < index; j++) {
             if (strcmp(subdirs[j], path[i+1]) == 0) {
                    ok = 1;
             }
        }      
        
        if (ok == 0) {
            return 0;
        }
    }
    return 1;
}

void getFileName(FILE* file, char* param1) {
    char* path[MAX_DIR_IMMERSION];
    char* token = strtok(param1, "/");
    int index = 0;
    char null[] = { '\0' };
    while (token != NULL) {
        path[index] = token;
        index++;
        token = strtok(NULL, "/");
    }
    if (isValidPath(file, path, index - 1)) {
        strcpy(param1, null);
        strcat(param1, path[index - 1]);
    }
    else {
        memset(param1, '\0', sizeof(param1));
    }
}

void copyFile(directory_item dir, int32_t* fat_tab, directory_item dest, FILE* file, char* name) {
    char content[desc.cluster_size];
    char new_content[desc.cluster_size];
    struct directory_item new_file;
    memset(new_file.item_name, '\0', sizeof(new_file.item_name));
    new_file.isFile = 1;
    strcpy(new_file.item_name, name);
    new_file.size = dir.size;
    new_file.start_cluster = getEmptyCl(fat_tab);

    int ptr = dir.start_cluster;
    int new_ptr = new_file.start_cluster;
    int prev_ptr = 0;
    int start_adress = desc.data_start_address + desc.cluster_size;
    int adress;
    char out[desc.cluster_size];
    int tmp;

    do {
        adress = start_adress + ptr * desc.cluster_size;
        fseek(file, adress, SEEK_SET);
        fread(&out, sizeof(out), 1, file);

        adress = start_adress + new_ptr * desc.cluster_size;
        fseek(file, adress, SEEK_SET);
        fwrite(&out, sizeof(out), 1, file);

        ptr = fat_tab[ptr];
        if (prev_ptr != 0) {
            fat_tab[prev_ptr] = new_ptr;
        }
        fat_tab[new_ptr] = FAT_FILE_END;
        prev_ptr = new_ptr;
        new_ptr = getEmptyCl(fat_tab);
    } while (ptr != FAT_FILE_END);

    adress = start_adress + dest.start_cluster * desc.cluster_size;
    fseek(file, adress, SEEK_SET);
    fread(&content, sizeof(content), 1, file);    
    strcpy(new_content, new_file.item_name);
    strcat(new_content, ",");
    strcat(content, new_content);
    fseek(file, adress, SEEK_SET);
    fwrite(&content, sizeof(content), 1, file);

    adress = desc.data_start_address + desc.dir_entry_count * sizeof(directory_item);
    desc.dir_entry_count++;
    fseek(file, adress, SEEK_SET);
    fwrite(&new_file, sizeof(new_file), 1, file);  

    for (int i = 0; i < desc.cluster_count; i++) {
        fseek(file, desc.fat1_start_address + (i * sizeof(int32_t)), SEEK_SET);
        fwrite(&fat_tab[i], sizeof(int32_t), 1, file);     
    }

}

void deleteFile(directory_item dir, int32_t* fat_tab, FILE* file) {
    int offset;
    struct directory_item item;
    for (int i = 0; i < desc.dir_entry_count; i++) {
        offset = desc.data_start_address + sizeof(directory_item) * i;
        fseek(file, offset, SEEK_SET);
        fread(&item, sizeof(struct directory_item), 1, file);        
        if (strcmp(item.item_name, dir.item_name) == 0) {
            memset(item.item_name, '\0', sizeof(item.item_name));
            item.isFile = 0;
            item.size = 0;
            item.start_cluster = 0;
            fseek(file, offset, SEEK_SET);
            fwrite(&item, sizeof(item), 1, file);
            i = desc.dir_entry_count;           
        }
    }

    int ptr = dir.start_cluster;
    int tmp;
    int start_adress = desc.data_start_address + desc.cluster_size;
    int adress;
    char cluster_empty[desc.cluster_size];
    memset(cluster_empty, '\0', sizeof(cluster_empty));
    do {
        adress = start_adress + ptr * desc.cluster_size;
        fseek(file, adress, SEEK_SET);
        fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);
        tmp = fat_tab[ptr];
        fat_tab[ptr] = FAT_UNUSED;             
        ptr = tmp;
    } while (ptr != FAT_FILE_END);

    for (int i = 0; i < desc.cluster_count; i++) {
        fseek(file, desc.fat1_start_address + (i * sizeof(int32_t)), SEEK_SET);
        fwrite(&fat_tab[i], sizeof(int32_t), 1, file);
    }
}

void deleteFromDir(char* name, int32_t* fat_tab, FILE* file) {
    struct directory_item dir;
    char content[desc.cluster_size];
    char new_content[desc.cluster_size];
    char* token;
    for (int i = 0; i < desc.dir_entry_count; i++) {
        int offset = desc.data_start_address + sizeof(directory_item) * i;
        fseek(file, offset, SEEK_SET);
        fread(&dir, sizeof(struct directory_item), 1, file);
        if (!dir.isFile) {
            offset = desc.data_start_address + desc.cluster_size + dir.start_cluster * desc.cluster_size;
            fseek(file, offset, SEEK_SET);
            fread(&content, sizeof(content), 1, file);      
            if (strstr(content, name) != NULL) {

                token = strtok(content, ",");
                if (token != NULL) {
                    if (strcmp(token, name) != 0) {
                        strcpy(new_content, token);
                        strcat(new_content, ",");
                    }
                    else {
                        strcpy(new_content, ",");
                    }                   
                    token = strtok(NULL, ",");
                    while (token != NULL) {
                        if (strcmp(token, name) != 0) {
                            strcat(new_content, token);
                            strcat(new_content, ",");
                        }
                        token = strtok(NULL, ",");
                    }
                }
          
                
                new_content[strlen(new_content) - 1] = '\0';               
                fseek(file, offset, SEEK_SET);
                fwrite(&new_content, sizeof(new_content), 1, file);
                return;
            }
        }
    }
}


void getActDirectory(char *path_actual, char *dir_name) {
    char s[strlen(path_actual)];
    strcpy(s, path_actual);
    char* token = strtok(s, "/");
    while (token != NULL) {
        strcpy(dir_name, token);
        token = strtok(NULL, "/");
    }
}

void moveFile(directory_item dir, directory_item src, directory_item dest, int32_t* fat_tab, FILE* file, char* new_name) {
    directory_item item;
    char content[desc.cluster_size];
    char old_name[13];
    int adress = desc.data_start_address;

    fseek(file, adress, SEEK_SET);
    fread(&item, sizeof(item), 1, file);
    while (strcmp(item.item_name, dir.item_name) != 0) {
        adress += sizeof(directory_item);
        fseek(file, adress, SEEK_SET);
        fread(&item, sizeof(item), 1, file);
    }
    strcpy(old_name, dir.item_name);
    strcpy(dir.item_name, new_name);
    fseek(file, adress, SEEK_SET);
    fwrite(&dir, sizeof(dir), 1, file);

    adress = desc.data_start_address + desc.cluster_size + (src.start_cluster * desc.cluster_size);
    fseek(file, adress, SEEK_SET);
    fread(&content, sizeof(content), 1, file);

    char* pos = strstr(content, old_name);
    int position = pos - content;

    int len = strlen(old_name);
    char* p = content;
    while ((p = strstr(p, old_name)) != NULL) {
        memmove(p, p + len, strlen(p + len) + 1);
    }
    memmove(&content[position], &content[position + 1], strlen(content) - position);
    if (position == strlen(content)) {
        position--;
        memmove(&content[position], &content[position + 1], strlen(content) - position);
    }
    fseek(file, adress, SEEK_SET);
    fwrite(&content, sizeof(content), 1, file);

    adress = desc.data_start_address + desc.cluster_size + (dest.start_cluster * desc.cluster_size);
    fseek(file, adress, SEEK_SET);
    fread(&content, sizeof(content), 1, file);

    strcat(new_name, ",");
    strcat(new_name, content);
    strcpy(content, new_name);

    fseek(file, adress, SEEK_SET);
    fwrite(&content, sizeof(content), 1, file);
}

int makeDirectory(directory_item dir, char* name, FILE* file, int32_t* fat_tab) {
    char* token;
    char content[desc.cluster_size];
    char new_content[desc.cluster_size];
    struct directory_item new_dir;
    int adress = desc.data_start_address + desc.cluster_size + (dir.start_cluster * desc.cluster_size);
    fseek(file, adress, SEEK_SET);
    fread(&content, sizeof(content), 1, file);

    strcpy(new_content, content);
    token = strtok(new_content, ",");
    while (token != NULL) {
        if (strcmp(token, name) == 0) {
            return 0;
        }
        token = strtok(NULL, ",");
    }
    
        strcat(content, ",");
        strcat(content, name);
        fseek(file, adress, SEEK_SET);
        fwrite(&content, sizeof(content), 1, file);

        memset(new_dir.item_name, '\0', sizeof(new_dir.item_name));
        new_dir.isFile = 0;
        strcpy(new_dir.item_name, name);
        new_dir.size = 0;
        new_dir.start_cluster = getEmptyCl(fat_tab);

        adress = desc.data_start_address + desc.dir_entry_count * sizeof(directory_item);
        desc.dir_entry_count++;
        fseek(file, adress, SEEK_SET);
        fwrite(&new_dir, sizeof(new_dir), 1, file);

        fat_tab[new_dir.start_cluster] = FAT_FILE_END;
        for (int i = 0; i < desc.cluster_count; i++) {
            fseek(file, desc.fat1_start_address + (i * sizeof(int32_t)), SEEK_SET);
            fwrite(&fat_tab[i], sizeof(int32_t), 1, file);
        }
        return 1;
    
}

int isEmptyDir(directory_item dir, FILE* file) {
    char content[desc.cluster_size];
    char null[desc.cluster_size];
    int adress = desc.data_start_address + desc.cluster_size + (dir.start_cluster * desc.cluster_size);
    fseek(file, adress, SEEK_SET);
    fread(&content, sizeof(content), 1, file);
    if (strlen(content) > 0) {
        return 0;
    }
    else {
        return 1;
    }
}

void printInfo(directory_item dir, int32_t* fat_tab) {
    int ptr = dir.start_cluster;
    printf("%s %d", dir.item_name, ptr);
    while (fat_tab[ptr] != FAT_FILE_END) {
        ptr = fat_tab[ptr];
        printf(", %d", ptr);
    }
    printf("\n");
}

void uploadFile(directory_item dir, char * name, int32_t* fat_tab, FILE* file, FILE* input) {
    long filelen;
    int index;
    fseek(input, 0, SEEK_END);
    filelen = ftell(input);
    rewind(input);
    char content[desc.cluster_size];
    char content_control[desc.cluster_size];
    struct directory_item new_input;
    new_input.size = filelen;
    strcpy(new_input.item_name, name);
    new_input.isFile = 1;
    int ptr = getEmptyCl(fat_tab);
    new_input.start_cluster = ptr;

    int adress = desc.data_start_address + desc.dir_entry_count * sizeof(directory_item);
    desc.dir_entry_count++;
    fseek(file, adress, SEEK_SET);
    fwrite(&new_input, sizeof(new_input), 1, file);

    index = 0; 
    int prev_ptr = ptr;
    int start_adress = desc.data_start_address + desc.cluster_size;
    do {
        memset(content, '\0', sizeof(content));
        fread(&content, sizeof(content), 1, input);           
        adress = start_adress + ptr * desc.cluster_size;
        fseek(file, adress, SEEK_SET);
        fwrite(&content, 1, sizeof(content), file);
        fat_tab[ptr] = FAT_FILE_END;
        prev_ptr = ptr;
        ptr = getEmptyCl(fat_tab);
        if (ptr != prev_ptr) {
            fat_tab[prev_ptr] = ptr;
        }
        index += sizeof(content);
    } while (index < filelen);
    fat_tab[prev_ptr] = FAT_FILE_END;

    for (int i = 0; i < desc.cluster_count; i++) {
        fseek(file, desc.fat1_start_address + (i * sizeof(int32_t)), SEEK_SET);
        fwrite(&fat_tab[i], sizeof(int32_t), 1, file);
    }

    adress = desc.data_start_address + desc.cluster_size + (dir.start_cluster * desc.cluster_size);
    fseek(file, adress, SEEK_SET);
    fread(&content, sizeof(content), 1, file);
    strcat(content, ",");
    strcat(content, name);
    fseek(file, adress, SEEK_SET);
    fwrite(&content, sizeof(content), 1, file);
}

void printOut(directory_item dir, FILE* output, FILE* file, int32_t* fat_tab) {
    int ptr = dir.start_cluster;
    int new_ptr = fat_tab[ptr];
    int start_adress = desc.data_start_address + desc.cluster_size;
    int adress;
    char out[desc.cluster_size];
    int i = 0;
    while (new_ptr != FAT_FILE_END) {
        adress = start_adress + ptr * desc.cluster_size;
        fseek(file, adress, SEEK_SET);
        fread(&out, sizeof(out), 1, file);
        fwrite(&out, sizeof(out), 1, output);
        i++;
        ptr = new_ptr;
        new_ptr = fat_tab[ptr];
    }
    int bytes_left = dir.size - i * sizeof(out);
    char rest[bytes_left];
    adress = start_adress + ptr * desc.cluster_size;
    fseek(file, adress, SEEK_SET);
    fread(&rest, sizeof(rest), 1, file);
    fwrite(&rest, sizeof(rest), 1, output);
}

bool notDerranged(directory_item dir, int32_t* fat_tab) {
    int ptr = dir.start_cluster;
    while (ptr != FAT_FILE_END) {
        if (fat_tab[ptr] != ptr + 1 && fat_tab[ptr] != FAT_FILE_END) {
            return true;
        }
        ptr = fat_tab[ptr];
    }
    return false;
}

void  defrag(directory_item dir, int32_t* fat_tab, FILE* file) {
    int cluster_count;
    int start_index;
    int ptr = dir.start_cluster;
    int new_ptr;
    int adress;
    int start_adress = desc.data_start_address + desc.cluster_size;
    char buffer[desc.cluster_size];
    char empty_buffer[desc.cluster_size];
    int index = 0;
    directory_item item;
    bool found; 
    if (dir.size > desc.cluster_size && notDerranged(dir, fat_tab)) {
        cluster_count = dir.size / desc.cluster_size;
        cluster_count++;
        memset(empty_buffer, '\0', sizeof(empty_buffer));
        for (int i = 0; i < desc.cluster_count; i++) {
            if (fat_tab[i] == FAT_UNUSED) {
                found = true;
                for (int j = 1; j < cluster_count; j++) {
                    if (fat_tab[i + j] != FAT_UNUSED) {
                        found = false;
                        j = cluster_count;
                    }
                }
                if (found) {
                    start_index = i;
                    i = desc.cluster_count;
                }
            }
        }

        adress = desc.data_start_address;
        fseek(file, adress, SEEK_SET);
        fread(&item, sizeof(item), 1, file);
        while (strcmp(item.item_name, dir.item_name) != 0) {
            adress += sizeof(directory_item);
            fseek(file, adress, SEEK_SET);
            fread(&item, sizeof(item), 1, file);
        }
        dir.start_cluster = start_index;
        fseek(file, adress, SEEK_SET);
        fwrite(&dir, sizeof(dir), 1, file);

        while (ptr != FAT_FILE_END) {
            new_ptr = fat_tab[ptr];
            fat_tab[ptr] = FAT_UNUSED;

            adress = start_adress + ptr * desc.cluster_size;
            fseek(file, adress, SEEK_SET);
            fread(&buffer, 1, sizeof(buffer), file);

            fseek(file, adress, SEEK_SET);
            fwrite(&empty_buffer, 1, sizeof(empty_buffer), file);

            ptr = new_ptr;

            adress = start_adress + (start_index + index) * desc.cluster_size;
            fseek(file, adress, SEEK_SET);
            fwrite(&buffer, 1, sizeof(buffer), file);
            fat_tab[start_index + index] = start_index + index + 1;
            index++;
        }
        fat_tab[start_index + index - 1] = FAT_FILE_END;

        for (int i = 0; i < desc.cluster_count; i++) {
            fseek(file, desc.fat1_start_address + (i * sizeof(int32_t)), SEEK_SET);
            fwrite(&fat_tab[i], sizeof(int32_t), 1, file);
        }
    }
}

void executeCommand(char* fce, char* param1, char* param2, FILE* file) {
    char* token;
    char* rest = NULL;
    char* path[MAX_DIR_IMMERSION];
    int prikaz;
    int offset, index;
    directory_item dir;
    int32_t fat_tab[desc.cluster_count];
    char enter;
    char null[5] = {'\0'};

    if (strcmp(fce, "cp") == 0) {
        directory_item dest;
        if (strchr(param1, '/') != NULL) {
            getFileName(file, param1);
        }
        dir = get_directory_item(file, param1);
        if (strcmp(dir.item_name, null) == 0) {
            printf("FILE NOT FOUND\n");
            return;;
        }

        token = strtok(param2, "/");
        index = 0;
        while (token != NULL) {
            path[index] = token;
            index++;
            token = strtok(NULL, "/");
        }

        if (isValidPath(file, path, index - 2)) {
            if (index > 1) {
                
                strcpy(param1, path[index - 2]);
                dest = get_directory_item(file, param1);
                strcpy(param2, path[index - 1]);
            }
            else {
                getActDirectory(path_actual, param1);
                dest = get_directory_item(file, param1);
                strcpy(param2, path[index - 1]);
            }
            if (strcmp(dest.item_name, null) == 0) {
                printf("PATH NOT FOUND \n");
                return;
            }
            get_fat(file, fat_tab);
            copyFile(dir, fat_tab, dest, file, param2);
            printf("OK\n");
        }
        else {
            printf("PATH NOT FOUND \n");
        }
    }

    else if (strcmp(fce, "mv") == 0) {
        directory_item src;
        directory_item dest;
        if (strchr(param1, '/') != NULL) {            
            token = strtok(param1, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }
            if (isValidPath(file, path, index - 1)) {
                if (index > 1) {
                    char* s = strdup(path[index - 1]);
                    strcpy(param1, path[index - 2]);
                    dir = get_directory_item(file, s);
                    src = get_directory_item(file, param1);
                }
                else {
                    char* s = strdup(path[index - 1]);
                    dir = get_directory_item(file, s);
                    getActDirectory(path_actual, param1);
                    src = get_directory_item(file, param1);
                }               
            }
            else {
                printf("FILE NOT FOUND\n");
                return;
            }
        }
        else {
            dir = get_directory_item(file, param1);
            getActDirectory(path_actual, param1);
            src = get_directory_item(file, param1);
        }

        if (strcmp(dir.item_name, null) == 0 || strcmp(src.item_name, null) == 0) {
            printf("FILE NOT FOUND\n");
            return;
        }

        if (strchr(param2, '/') != NULL) {
            token = strtok(param2, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }
            if (isValidPath(file, path, index - 2)) {
                if (index > 1) {
                    strcpy(param1, path[index - 2]);
                    strcpy(param2, path[index - 1]);
                    dest = get_directory_item(file, param2);
                    if (dest.isFile == 0) {
                        strcpy(param2, dir.item_name);
                    }
                    else {
                        dest = get_directory_item(file, param1);
                    }
                }
                else {
                    strcpy(param2, path[index - 1]);
                    dest = get_directory_item(file, param2);
                    if (dest.isFile == 0) {
                        strcpy(param2, dir.item_name);
                    }
                    else {
                        getActDirectory(path_actual, param1);
                        dest = get_directory_item(file, param1);
                    }
                }
                if (strcmp(dest.item_name, null) == 0) {
                    printf("PATH NOT FOUND \n");
                    return;
                }
               
            }
            else {
                printf("PATH NOT FOUND \n");
                return;
            }
        }
        else {
            dest = get_directory_item(file, param2);
            if (strcmp(dest.item_name, null) == 0) {
                getActDirectory(path_actual, param1);
                dest = get_directory_item(file, param1);
            }
            else {
                getActDirectory(path_actual, param1);
                path[0] = param1;
                path[1] = param2;
                if (isValidPath(file, path, 1)) {
                    strcpy(param2, dir.item_name);
                }
            }

        }
        get_fat(file, fat_tab);
        moveFile(dir, src, dest, fat_tab, file, param2);
        printf("OK\n");
    }

    else if (strcmp(fce, "rm") == 0) {
        if (strchr(param1, '/') != NULL) {
            getFileName(file, param1);
        }
        dir = get_directory_item(file, param1);
        if (strcmp(dir.item_name, null) == 0 || dir.isFile == 0) {
            printf("FILE NOT FOUND\n");
            return;
        }
        else {
            get_fat(file, fat_tab);
            deleteFromDir(dir.item_name, fat_tab, file);
            deleteFile(dir, fat_tab, file);
            printf("OK\n");
        }
    }

    else if (strcmp(fce, "mkdir") == 0) {
        if (strchr(param1, '/') != NULL) {
            token = strtok(param1, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }
            if (isValidPath(file, path, index - 2)) {
                if (index > 1) {
                    dir = get_directory_item(file, path[index - 2]);
                }
                else {
                    getActDirectory(path_actual, param2);
                    dir = get_directory_item(file, param2);
                }
                char empty[20] = { '\0' };
                strcpy(param1, empty);
                strcat(param1, path[index - 1]);
                
            }
            else {
                printf("PATH NOT FOUND \n");
                return;
            }
        }
        else {
            getActDirectory(path_actual, param2);
            dir = get_directory_item(file, param2);
        }
        get_fat(file, fat_tab);
        if (makeDirectory(dir, param1, file, fat_tab)) {
            printf("OK\n");
        }
        else {
            printf("EXIST\n");
        }
    }

    else if (strcmp(fce, "rmdir") == 0) {
        if (strchr(param1, '/') != NULL) {
            token = strtok(param1, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }
            if (isValidPath(file, path, index - 1)) {
                dir = get_directory_item(file, path[index - 1]);
            }
            else {
                printf("FILE NOT FOUND\n");
                return;
            }
        }
        else {
            dir = get_directory_item(file, param1);
        }
        if (strcmp(dir.item_name, null) == 0 || dir.isFile == 1) {
            printf("FILE NOT FOUND\n");
        }
        else {
            if (isEmptyDir(dir, file)) {
                get_fat(file, fat_tab);
                deleteFromDir(dir.item_name, fat_tab, file);
                deleteFile(dir, fat_tab, file);
                printf("OK\n");
            }
            else {
                printf("NOT EMPTY\n");
            }
        }
    }

    else if (strcmp(fce, "ls") == 0) {
        if (strlen(param1) == 0) {
            getActDirectory(path_actual, param1);
            dir = get_directory_item(file, param1);
            get_fat(file, fat_tab);
            printFile(dir, fat_tab, file);
            printf("\n");
        }
        else {
            token = strtok(param1, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }
            if (isValidPath(file, path, index - 1)) {
                dir = get_directory_item(file, path[index - 1]);
                if (dir.isFile) {
                    printf("PATH NOT FOUND\n");
                }
                else {
                    get_fat(file, fat_tab);
                    printFile(dir, fat_tab, file);
                }
            }
            else {
                printf("PATH NOT FOUND\n");
            }
        }
    }

    else if (strcmp(fce, "cat") == 0) {
        if (strchr(param1, '/') != NULL) {
            getFileName(file, param1);
        }
        printf("param1: %s\n", param1);
        dir = get_directory_item(file, param1);
        printf("dirname: %s\n", dir.item_name);
        if (strcmp(dir.item_name, null) == 0 || dir.isFile == 0) {
            printf("FILE NOT FOUND\n");
        }
        else {
            get_fat(file, fat_tab);
            printFile(dir, fat_tab, file);
        }
    }

    else if (strcmp(fce, "cd") == 0) {
        if (strchr(param1, '/') != NULL) {
            token = strtok(param1, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }
            dir = get_directory_item(file, path[index - 1]);
            if (isValidPath(file, path, index - 1) && dir.isFile == 0) {
                strcpy(path_actual, "root");
                for (int i = 0; i < index; i++) {
                    strcat(path_actual, "/");
                    strcat(path_actual, path[i]);
                }
            }
            else {
                getActDirectory(path_actual, param2);
                for (int i = index; i > 0; i--) {
                    path[i] = path[i - 1];
                }
                path[0] = param2;
                if (isValidPath(file, path, index - 1) && dir.isFile == 0) {
                    for (int i = 0; i < index - 1; i++) {
                        strcat(path_actual, path[i]);
                        strcat(path_actual, "/");
                    }
                }
                else {
                    printf("PATH NOT FOUND\n");
                    return;
                }
            }

        }
        else {
            if (strcmp(param1, "..") == 0) {
                char* s;
                s = &path_actual[strlen(path_actual) - 1];
                while (strcmp(s, "/") != 0) {
                    path_actual[strlen(path_actual) - 1] = '\0';
                    s = &path_actual[strlen(path_actual) - 1];
                }
                path_actual[strlen(path_actual) - 1] = '\0';
            }
            else {
                index = 0;
                getActDirectory(path_actual, param2);
                path[index] = param2;
                path[index + 1] = param1;
                dir = get_directory_item(file, path[1]);
                if (isValidPath(file, path, 1) && dir.isFile == 0) {
                    char tmp[2] = "/";
                    strcat(path_actual, tmp);
                    strcat(path_actual, param1);
                }
                else {
                    printf("PATH NOT FOUND\n");
                    return;
                }
            }
        }
        printf("%s> ", path_actual);
    }

    else if (strcmp(fce, "pwd") == 0) {
        printf("%s> ", path_actual);
    }

    else if (strcmp(fce, "info") == 0) {
        if (strchr(param1, '/') != NULL) {
            token = strtok(param1, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }
            if (isValidPath(file, path, index - 1)) {
                dir = get_directory_item(file, path[index - 1]);
            }
            else {
                printf("FILE NOT FOUND\n");
                return;
            }
        }
        else {
            dir = get_directory_item(file, param1);
        }
        if (strcmp(dir.item_name, null) == 0) {
            printf("FILE NOT FOUND\n");
            return;
        }
        else {
            get_fat(file, fat_tab);
            printInfo(dir, fat_tab);
        }
    }

    else if (strcmp(fce, "incp") == 0) {
        FILE* input;
        input = fopen(param1, "r");
        if (NULL == input) {
            printf("FILE NOT FOUND\n");
            return;
        }
        if (strchr(param2, '/') != NULL) {
            token = strtok(param2, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }
            if (isValidPath(file, path, index - 2)) {
                if (index > 1) {
                    strcpy(param1, path[index - 2]);                    
                    dir = get_directory_item(file, param1);                    
                }
                else {
                    getActDirectory(path_actual, param1);
                    dir = get_directory_item(file, param1);
                } 
                char empty[20] = { '\0' };
                strcpy(param2, empty);
                strcat(param2, path[index - 1]);             
            }
        }
        else {
            getActDirectory(path_actual, param1);
            dir = get_directory_item(file, param1);
        }
        get_fat(file, fat_tab);
        uploadFile(dir, param2, fat_tab, file, input);
        fclose(input);
        printf("OK\n");
    }

    else if (strcmp(fce, "outcp") == 0) {
        FILE* output;
        output = fopen(param2, "w");
        if (NULL == output) {
            printf("PATH NOT FOUND\n");
            return;
        }
        if (strchr(param1, '/') != NULL) {
            token = strtok(param1, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }
            if (isValidPath(file, path, index - 1)) {
                dir = get_directory_item(file, path[index - 1]);
            }
            else {
                printf("FILE NOT FOUND\n");
                return;
            }
        }
        else {
            dir = get_directory_item(file, param1);
        }
        if (strlen(dir.item_name) < 1) {
            printf("FILE NOT FOUND\n");
            return;
        }
        get_fat(file, fat_tab);
        printOut(dir, output, file, fat_tab);
        fclose(output);
        printf("OK\n");
    }

    else if (strcmp(fce, "defrag") == 0) {
        if (strchr(param1, '/') != NULL) {
            getFileName(file, param1);
        }
        dir = get_directory_item(file, param1);
        if (strcmp(dir.item_name, null) == 0 || dir.isFile == 0) {
            printf("FILE NOT FOUND\n");
        }
        else {
            get_fat(file, fat_tab);          
            defrag(dir, fat_tab, file);          
            printf("OK\n");
        }
    }

    else {
        printf("Neplatny prikaz\n");
    }
}

void format(FILE* file, const char* name, char* size) {   
    char* token;
    int value;
    if (strstr(size, "GB") != NULL) {
        token = strtok(size, "GB");   
        value = atoi(token);
        value = value * 1000000000;
    }
    else if (strstr(size, "MB") != NULL) {   
        token = strtok(size, "MB");                                 
        value = atoi(token);
        value = value * 1000000;
    }
    else if (strstr(size, "kB") != NULL) {        
        token = strtok(size, "kB");        
        value = atoi(token);
        value = value * 1000;
    }
    else if (strstr(size, "B") != NULL) {     
        token = strtok(size, "B");           
        value = atoi(token);
    }
    else {
        printf("Wrong format parameter!\n");
        return;
    }   
    
    int cluster_count = sqrt(value);
    if (cluster_count * sizeof(int) > FAT_MAX_SIZE) {
        cluster_count = (FAT_MAX_SIZE / sizeof(int)) - 1;
    }
    int cluster_size = value / cluster_count;
    cluster_size++;

    memset(desc.signature, '\0', sizeof(desc.signature));
    desc.cluster_count = cluster_count;
    desc.cluster_size = cluster_size;
    desc.dir_entry_count = 1;
    desc.formated = true;
    strcpy(desc.signature, name);
    struct directory_item root_dir;
    memset(root_dir.item_name, '\0', sizeof(root_dir.item_name));
    root_dir.isFile = 0;
    strcpy(root_dir.item_name, "root");
    root_dir.size = 0;
    root_dir.start_cluster = 0;
    char cluster_empty[desc.cluster_size];
    memset(cluster_empty, '\0', sizeof(cluster_empty));
    
    int fat[desc.cluster_count];
    fat[0] = FAT_FILE_END;
    for (int32_t i = 1; i < desc.cluster_count; i++)
    {
        fat[i] = FAT_UNUSED;
    }
    fseek(file, 0, SEEK_SET);
    fwrite(&desc, sizeof(desc), 1, file);
    fwrite(&fat, sizeof(fat), 1, file);
    desc.fat1_start_address = sizeof(desc);
    desc.data_start_address = desc.fat1_start_address + sizeof(fat);
    fwrite(&root_dir, sizeof(root_dir), 1, file);
    char buffer[] = { '\0' };
    for (int i = 0; i < (desc.cluster_size - sizeof(root_dir)); i++) {
        fwrite(buffer, sizeof(buffer), 1, file);
    }
    for (int i = 1; i < desc.cluster_count; i++)
    {
        fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);
    }
    printf("OK\n");
}

int main(int argc, char** argv) {

    if (argc != 2) {
        printf("Wrong number of parameters!!");
        return -1;
    }

    const char* name = argv[1];

    if (strlen(name) > 12) {
        printf("Name must be maximum 12 characters long!!");
        return -1;
    }

    FILE* file;
    char vstup[50];
    char fce[10];
    char param1[20];
    char param2[50];
    char* token;
    char* rest = NULL;
    int prikaz;

    /* aktualni cesta - pri spusteni v adresari root * /
    strcpy(path_actual, "/root");
    /* po spusteni neni formatovany vstupni soubor */
    bool formated = false;

    file = fopen(name, "w+");
    char null[] = { '\0' };

    while (true) {
        memset(&vstup, '\0', sizeof(vstup));
        memset(&param1, '\0', sizeof(param1));
        memset(&param2, '\0', sizeof(param2));

        fgets(vstup, 50, stdin);

        vstup[strcspn(vstup, "\n")] = 0;
        if (strcmp(vstup, null) != 0) {

            /* ziskani parametru ze vstupu */
            token = strtok_r(vstup, " ", &rest);
            strcpy(fce, token);
            token = strtok_r(NULL, " ", &rest);
            if (token != NULL) {
                strcpy(param1, token);
                token = strtok_r(NULL, " ", &rest);
                if (token != NULL) {
                    strcpy(param2, token);
                    token = strtok_r(NULL, " ", &rest);
                    if (token != NULL) {
                        printf("prilis mnoho parametru\n");
                        return -1;
                    }
                }
            }

            if (strcmp(fce, "format") == 0) {
                format(file, name, param1);
                formated = true;
            }
            else {
                if (formated) {
                    if (strcmp(fce, "load") == 0) {
                        int bufferLength = 50;
                        char buffer[bufferLength];
                        bool txt = false;

                        /* u souboru s priponou .txt je treba odstranovat esc znaky */
                        if (strstr(param1, ".txt") != NULL) {
                            txt = true;
                        }
                        FILE* commands = fopen(param1, "r");
                        if (NULL == commands) {
                            printf("FILE NOT FOUND\n");
                        }
                        else {
                            while (fgets(buffer, bufferLength, commands)) {
                                memset(fce, '\0', sizeof(fce));
                                memset(param1, '\0', sizeof(param1));
                                memset(param2, '\0', sizeof(param2));
                                char last_char = buffer[strlen(buffer) - 1];
                                char nl = '\n';
                                buffer[strcspn(buffer, "\n")] = 0;

                                /* odstrani se posledni znak pokud je \n a vstupni soubor je s priponou txt*/
                                if (last_char == nl && txt) {
                                    buffer[strlen(buffer) - 1] = '\0';
                                }

                                printf("%s\n", buffer);

                                /* ziskani parametru */
                                if (strstr(buffer, " ") != NULL) {
                                    token = strtok_r(buffer, " ", &rest);

                                    strcpy(fce, token);

                                    token = strtok_r(NULL, " ", &rest);
                                    if (token != NULL) {
                                        strcpy(param1, token);
                                        token = strtok_r(NULL, " ", &rest);
                                        if (token != NULL) {
                                            strcpy(param2, token);
                                        }
                                    }
                                }
                                else {
                                    strcpy(fce, buffer);
                                }                            
                                executeCommand(fce, param1, param2, file);
                                memset(buffer, '\0', sizeof(buffer));
                            }
                            fclose(commands);
                            printf("OK\n");
                        }
                    }
                    else {
                        executeCommand(fce, param1, param2, file);
                    }
                }
                else {
                    printf("Input file is not formated, please format first\n");
                }
            }
        }
    }
    fclose(file);

    return 0;
    
}



    
    