/* ========================================================================== */
/*                                                                            */
/*   File: main.c                                                         */
/*   Author: saveld                                                           */
/*                                                                            */
/*   Created: 23.10.2022                                                      */
/*                                                                            */
/* ========================================================================== */

#include <stdio.h>
#include <unistd.h>
#include <ostream>
#include <iostream>
#include <cstring>

using namespace std;

const int32_t FAT_UNUSED = INT32_MAX - 1;
const int32_t FAT_FILE_END = INT32_MAX - 2;
const int32_t FAT_BAD_CLUSTER = INT32_MAX - 3;
const int32_t MAX_DIR_IMMERSION = 5;

struct description {
    char signature[9];              //login autora FS
    int32_t disk_size;              //celkova velikost VFS
    int32_t cluster_size;           //velikost clusteru
    int32_t cluster_count;          //pocet clusteru
    int32_t fat_count;        	    //pocet polozek kazde FAT tabulce
    int32_t fat1_start_address;	    //adresa pocatku FAT1 tabulky
    int32_t data_start_address;     //adresa pocatku datovych bloku (hl. adresar)
    int32_t dir_entry_count;
};

struct directory_item {
    char item_name[13];              //8+3 + /0 C/C++ ukoncovaci string znak
    bool isFile;		     //identifikace zda je soubor (TRUE), nebo adres?? (FALSE)
    int32_t size;                    //velikost souboru, u adres??e 0 (bude zabirat jeden blok)
    int32_t start_cluster;           //po??te?n? cluster polo?ky
};

struct description desc;

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
    do {
        adress = start_adress + ptr * desc.cluster_size;
        fseek(file, adress, SEEK_SET);
        fread(&out, sizeof(out), 1, file);
        printf("%s",out);
        ptr = fat_tab[ptr];      
    } while (ptr != FAT_FILE_END);
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
    while (token != NULL) {
        path[index] = token;
        index++;
        token = strtok(NULL, "/");
    }  
    if (isValidPath(file, path, index - 1)) {
        strcpy(param1, path[index - 1]);
    }
    else {
        memset(param1, '\0', sizeof(param1));
    }
}

void copyFile(directory_item dir, int32_t* fat_tab, FILE* file, char** cesta, int size) {
    printf("kopiruju soubor: %s, do: %s\n", dir.item_name, cesta[size - 2]);
    char content[desc.cluster_size];
    struct directory_item new_file;
    memset(new_file.item_name, '\0', sizeof(new_file.item_name));
    new_file.isFile = 1;
    strcpy(new_file.item_name, cesta[size - 1]);
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

    struct directory_item subdir = get_directory_item(file, cesta[size - 2]);
    adress = start_adress + subdir.start_cluster * desc.cluster_size;
    fseek(file, adress, SEEK_SET);
    fread(&content, sizeof(content), 1, file);
    strcpy(content, ",");
    strcpy(content, new_file.item_name);
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
    printf("odstranuji: %s\n", dir.item_name);
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
                    strcpy(new_content, token);
                    strcat(new_content, ",");
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


void getActDirectory(FILE* file, char *path_actual, char *dir_name) {
    char s[strlen(path_actual)];
    strcpy(s, path_actual);
    char* token = strtok(s, "/");
    while (token != NULL) {
        strcpy(dir_name, token);
        token = strtok(NULL, "/");
    }
}

void moveFile(directory_item dir, directory_item src, directory_item dest, int32_t* fat_tab, FILE* file, char* new_name) {
    printf("presunuji soubor - %s ze - %s do - %s se jmenem - %s\n", dir.item_name, src.item_name, dest.item_name, new_name);
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
    printf("delam dir: %s v adresari %s\n", name, dir.item_name);
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
    printf("content %s\n", content);
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
    char bytes[filelen];
    char content[desc.cluster_size];
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

    fread(&content, sizeof(content), 1, input);
    printf("%s\n", content);

}

int main(int argc, char** argv) {

    if (argc != 2) {
        printf("Wrong number of parameters!!");
        return -1;
    }

    const char* name = argv[1];

    if (strlen(name) > 9) {
        printf("Name must be maximum 9 characters long!!");
        return -1;
    }

    FILE* file;
    struct directory_item root_dir;
    struct directory_item root_a, root_b, root_c, root_d, root_e, root_f;
    //smazani /0 u stringu
    memset(desc.signature, '\0', sizeof(desc.signature));
    desc.cluster_size = 256;
    desc.cluster_count = 252;
    desc.dir_entry_count = 7;
    strcpy(desc.signature, name);
    printf("name: %s\n", desc.signature);

    /*
    * vytvo?en? root polo?ek
    */
    memset(root_dir.item_name, '\0', sizeof(root_dir.item_name));
    root_dir.isFile = 0;
    strcpy(root_dir.item_name, "root");
    root_dir.size = 0;
    root_dir.start_cluster = 0;

    memset(root_a.item_name, '\0', sizeof(root_a.item_name));
    root_a.isFile = 1;
    strcpy(root_a.item_name, "cisla.txt");
    root_a.size = 135;
    root_a.start_cluster = 25;

    memset(root_b.item_name, '\0', sizeof(root_b.item_name));
    root_b.isFile = 1;
    strcpy(root_b.item_name, "pohadka.txt");
    root_b.size = 5975;
    root_b.start_cluster = 1;

    /*
    * directory
    */
    memset(root_c.item_name, '\0', sizeof(root_c.item_name));
    root_c.isFile = 0;
    strcpy(root_c.item_name, "direct-1");
    root_c.size = 0;
    root_c.start_cluster = 29;

    memset(root_d.item_name, '\0', sizeof(root_d.item_name));
    root_d.isFile = 1;
    strcpy(root_d.item_name, "cisla1.txt");
    root_d.size = 135;
    root_d.start_cluster = 30;

    memset(root_e.item_name, '\0', sizeof(root_e.item_name));
    root_e.isFile = 0;
    strcpy(root_e.item_name, "subdir");
    root_e.size = 0;
    root_e.start_cluster = 31;

    memset(root_f.item_name, '\0', sizeof(root_f.item_name));
    root_f.isFile = 1;
    strcpy(root_f.item_name, "hoven.txt");
    root_f.size = 56;
    root_f.start_cluster = 32;

    char cluster_root[desc.cluster_size];
    char cluster_empty[desc.cluster_size];
    char cluster_dir1[desc.cluster_size];
    char cluster_dir2[desc.cluster_size];
    char cluster_a[desc.cluster_size];
    char cluster_b1[desc.cluster_size];
    char cluster_b2[desc.cluster_size];
    char cluster_b3[desc.cluster_size];
    char cluster_b4[desc.cluster_size];
    char cluster_b5[desc.cluster_size];
    char cluster_b6[desc.cluster_size];
    char cluster_b7[desc.cluster_size];
    char cluster_b8[desc.cluster_size];
    char cluster_b9[desc.cluster_size];
    char cluster_b10[desc.cluster_size];
    char cluster_b11[desc.cluster_size];
    char cluster_b12[desc.cluster_size];
    char cluster_b13[desc.cluster_size];
    char cluster_b14[desc.cluster_size];
    char cluster_b15[desc.cluster_size];
    char cluster_b16[desc.cluster_size];
    char cluster_b17[desc.cluster_size];
    char cluster_b18[desc.cluster_size];
    char cluster_b19[desc.cluster_size];
    char cluster_b20[desc.cluster_size];
    char cluster_b21[desc.cluster_size];
    char cluster_b22[desc.cluster_size];
    char cluster_b23[desc.cluster_size];
    char cluster_b24[desc.cluster_size];
    char cluster_c1[desc.cluster_size];
    char cluster_c2[desc.cluster_size];
    char cluster_hoven[desc.cluster_size];

    memset(cluster_root, '\0', sizeof(cluster_root));
    strcpy(cluster_root, "pohadka.txt,cisla.txt,direct-1");

    memset(cluster_hoven, '\0', sizeof(cluster_hoven));
    strcpy(cluster_hoven, "www.youtube.com/watch?v=qJTwTYgouQY&ab_channel=VNEUMICKY");

    char cluster_cccc[desc.cluster_size];
    memset(cluster_cccc, '\0', sizeof(cluster_cccc));
    strcpy(cluster_cccc, "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 - tohle je malicky soubor pro test");

    memset(cluster_empty, '\0', sizeof(cluster_empty));
    memset(cluster_a, '\0', sizeof(cluster_a));
    memset(cluster_b1, '\0', sizeof(cluster_b1));
    memset(cluster_b2, '\0', sizeof(cluster_b2));
    memset(cluster_b3, '\0', sizeof(cluster_b3));
    memset(cluster_b4, '\0', sizeof(cluster_b4));
    memset(cluster_b5, '\0', sizeof(cluster_b5));
    memset(cluster_b6, '\0', sizeof(cluster_b6));
    memset(cluster_b7, '\0', sizeof(cluster_b7));
    memset(cluster_b8, '\0', sizeof(cluster_b8));
    memset(cluster_b9, '\0', sizeof(cluster_b9));
    memset(cluster_b10, '\0', sizeof(cluster_b10));
    memset(cluster_b11, '\0', sizeof(cluster_b11));
    memset(cluster_b12, '\0', sizeof(cluster_b12));
    memset(cluster_b13, '\0', sizeof(cluster_b13));
    memset(cluster_b14, '\0', sizeof(cluster_b14));
    memset(cluster_b15, '\0', sizeof(cluster_b15));
    memset(cluster_b16, '\0', sizeof(cluster_b16));
    memset(cluster_b17, '\0', sizeof(cluster_b17));
    memset(cluster_b18, '\0', sizeof(cluster_b18));
    memset(cluster_b19, '\0', sizeof(cluster_b19));
    memset(cluster_b20, '\0', sizeof(cluster_b20));
    memset(cluster_b21, '\0', sizeof(cluster_b21));
    memset(cluster_b22, '\0', sizeof(cluster_b22));
    memset(cluster_b23, '\0', sizeof(cluster_b23));
    memset(cluster_b24, '\0', sizeof(cluster_b24));
    memset(cluster_c1, '\0', sizeof(cluster_c1));
    memset(cluster_c2, '\0', sizeof(cluster_c2));
    memset(cluster_dir1, '\0', sizeof(cluster_dir1));
    memset(cluster_dir2, '\0', sizeof(cluster_dir2));
    strcpy(cluster_a, "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 - tohle je malicky soubor pro test");
    strcpy(cluster_b1, "Byla jednou jedna sladka divenka, kterou musel milovat kazdy, jen ji uvidel, ale nejvice ji milovala jeji babicka, ktera by ji snesla i modre z nebe. Jednou ji darovala cepecek karkulku z cerveneho sametu a ten se vnucce tak libil, ze nic jineho nechtela ");
    strcpy(cluster_b2, "nosit, a tak ji zacali rikat Cervena Karkulka. Jednou matka Cervene Karkulce rekla: ?Podivej, Karkulko, tady mas kousek kolace a lahev vina, zanes to babicce, je nemocna a zeslabla, timhle se posilni. Vydej se na cestu drive nez bude horko, jdi hezky spor");
    strcpy(cluster_b3, "adane a neodbihej z cesty, kdyz upadnes, lahev rozbijes a babicka nebude mit nic. A jak vejdes do svetnice, nezapome? babicce poprat dobreho dne a ne abys smejdila po vsech koutech.? ?Ano, maminko, udelam, jak si prejete.? rekla Cerveni Karkulka, na stvrz");
    strcpy(cluster_b4, "eni toho slibu podala matce ruku a vydala se na cestu. Babicka bydlela v lese; celou p?lhodinu cesty od vesnice. Kdyz sla Cervena Karkulka lesem, potkala vlka. Tenkrat jeste nevedela, co je to za zaludne zvire a ani trochu se ho nebala. ?Dobry den, Cerven");
    strcpy(cluster_b5, "a Karkulko!? rekl vlk. ?Dekuji za prani, vlku.? ?Kampak tak casne, Cervena Karkulko?? ?K babicce!? ?A copak to neses v zasterce?? ?Kolac a vino; vcera jsme pekli, nemocne a zeslable babicce na posilnenou.? ?Kdepak bydli babicka, Cervena Karkulko?? ?Inu, j");
    strcpy(cluster_b6, "este tak ctvrthodiny cesty v lese, jeji chaloupka stoji mezi tremi velkymi duby, kolem je liskove oresi, urcite to tam musis znat.? odvetila Cervena Karkulka. Vlk si pomyslil: ?Tohle mla?oucke, jem?oucke masicko bude jiste chutnat lepe nez ta starena, mus");
    strcpy(cluster_b7, "im to navleci lstive, abych schlamstnul obe.? Chvili sel vedle Cervene Karkulky a pak pravil: ?Cervena Karkulko, koukej na ty krasne kvetiny, ktere tu rostou vsude kolem, procpak se trochu nerozhlednes? Myslim, ze jsi jeste neslysela ptacky, kteri by zpiv");
    strcpy(cluster_b8, "ali tak libezne. Ty jsi tu vykracujes, jako kdybys sla do skoly a pritom je tu v lese tak krasne!? Cervena Karkulka otevrela oci dokoran a kdyz videla, jak slunecni paprsky tancuji skrze stromy sem a tam a vsude roste tolik krasnych kvetin, pomyslila si: ");
    strcpy(cluster_b9, "?Kdyz prinesu babicce kytici cerstvych kvetin, bude mit jiste radost, casu mam dost, prijdu akorat.? A sebehla z cesty do lesa a trhala kvetiny. A kdyz jednu utrhla, zjistila, ze o kus dal roste jeste krasnejsi, bezela k ni, a tak se dostavala stale hloub");
    strcpy(cluster_b10, "eji do lesa. Ale vlk bezel rovnou k babiccine chaloupce a zaklepal na dvere. ?Kdo je tam?? ?Cervena Karkulka, co nese kolac a vino, otevri!? ?Jen zmackni kliku,? zavolala babicka: ?jsem prilis slaba a nemohu vstat.? Vlk vzal za kliku, otevrel dvere a beze");
    strcpy(cluster_b11, "slova sel rovnou k babicce a spolknul ji. Pak si obleknul jeji saty a nasadil jeji cepec, polozil se do postele a zatahnul zaves. Zatim Cervena Karkulka behala mezi kvetinami, a kdyz jich mela naruc tak plnou, ze jich vic nemohla pobrat, tu ji prisla na  ");
    strcpy(cluster_b12, "mysl babicka, a tak se vydala na cestu za ni. Podivila se, ze jsou dvere otevrene, a kdyz vesla do svetnice, prislo ji vse takove podivne, ze si pomyslila: ?Dobrotivy Boze, je mi dneska nejak ?zko a jindy jsem u babicky tak rada.? Zvolala: ?Dobre jitro!? ");
    strcpy(cluster_b13, "Ale nedostala zadnou odpove?. ?la tedy k posteli a odtahla zaves; lezela tam babicka a mela cepec narazeny hluboko do obliceje a vypadala nejak podivne. Ach, babicko, proc mas tak velke usi?? ?Abych te lepe slysela.? ?Ach, babicko, proc mas tak velke oci ");
    strcpy(cluster_b14, "?? ?Abych te lepe videla.? ?Ach, babicko, proc mas tak velke ruce?? ?Abych te lepe objala.? ?Ach, babicko, proc mas tak straslivou tlamu?? ?Abych te lepe sezrala!!? Sotva vlk ta slova vyrknul, vyskocil z postele a ubohou Cervenou Karkulku spolknul. Kdyz t");
    strcpy(cluster_b15, "e? uhasil svoji zadostivost, polozil se zpatky do postele a usnul a z toho spanku se jal mocne chrapat. Zrovna sel kolem chaloupky lovec a pomyslil si: ?Ta starenka ale chrape, musim se na ni podivat, zda neco nepotrebuje.? Vesel do svetnice, a kdyz prist");
    strcpy(cluster_b16, "oupil k posteli, uvidel, ze v ni lezi vlk. ?Tak prece jsem te nasel, ty stary hrisniku!? zvolal lovec: ?Uz mam na tebe dlouho policeno!? Strhnul z ramene pusku, ale pak mu prislo na mysl, ze vlk mohl sezrat babicku a mohl by ji jeste zachranit. Nestrelil ");
    strcpy(cluster_b17, "tedy, nybrz vzal n?zky a zacal spicimu vlkovi parat bricho. Sotva ucinil par rez?, uvidel se cervenat karkulku a po par dalsich rezech vyskocila divenka ven a volala: ?Ach, ja jsem se tolik bala, ve vlkovi je cernocerna tma.? A potom vylez la ven i ziva b");
    strcpy(cluster_b18, "abicka; sotva dechu popadala. Cervena Karkulka pak nanosila obrovske kameny, kterymi vlkovo bricho naplnili, a kdyz se ten probudil a chtel uteci, kameny ho tak desive tizily, ze klesnul k zemi nadobro mrtvy. Ti tri byli spokojeni. Lovec stahnul vlkovi ko");
    strcpy(cluster_b19, "zesinu a odnesl si ji dom?, babicka snedla kolac a vypila vino, ktere Cervena Karkulka prinesla, a opet se zotavila. A Cervena Karkulka? Ta si svatosvate prisahala: ?Uz nikdy v zivote nesejdu z cesty do lesa, kdyz mi to maminka zakaze!? O Cervene  Karkulc");
    strcpy(cluster_b20, "e se jeste vypravi, ze kdyz sla jednou zase k babicce s babovkou, potkala jineho vlka a ten se ji taky vemlouval a snazil se ji svest z cesty. Ale  ona se toho vystrihala a kracela rovnou k babicce, kde hned vypovedela, ze potkala vlka, ktery ji sice popr");
    strcpy(cluster_b21, "al dobry den, ale z oci mu koukala nekalota. ?Kdyby to nebylo na verejne ceste, jiste by mne sezral!? ?Poj?,?  rekla babicka: ?zavreme dobre dvere, aby nemohl dovnitr.? Brzy nato zaklepal vlk a zavolal: ?Otevri, babicko, ja jsem Cervena Karkulka a nesu ti");
    strcpy(cluster_b22, "pecivo!? Ty dve vsak z?staly jako peny a neotevrely. Tak se ten sedivak plizil kolem domu a naslouchal, pak vylezl na strechu, aby tam pockal, az Cervena Karkulka p?jde vecer dom?, pak ji v temnote popadne a sezere. Ale babicka zle vlkovy ?mysly odhalila ");
    strcpy(cluster_b23, ". Pred domem staly obrovske kamenne necky, tak Cervene  Karkulce rekla: ?Vezmi vedro, devenko, vcera jsem varila klobasy, tak tu vodu nanosime venku do necek.? Kdyz byly necky plne, stoupala v?ne klobas nahoru az k vlkovu cenichu. Zavetril a natahoval krk");
    strcpy(cluster_b24, "tak daleko, ze se na strese vice neudrzel a zacal klouzat dol?, kde spadnul primo do necek a bidne se utopil.");
    strcpy(cluster_c1, "Prodej aktiv SABMilleru v Ceske republice, Polsku, Ma?arsku, Rumunsku a na Slovensku je soucasti podminek pro prevzeti podniku ze strany americkeho pivovaru Anheuser-Busch InBev, ktere bylo dokonceno v rijnu. Krome Plze?skeho Prazdroje zahrnuji prodavana ");
    strcpy(cluster_c2, "aktiva polske znacky Tyskie a Lech, slovensky Topvar, ma?arsky Dreher a rumunsky Ursus. - Tento soubor je sice kratky, ale neni fragmentovany");
    strcpy(cluster_dir1, "cisla1.txt,subdir");
    strcpy(cluster_dir2, "hoven.txt");

    //FAT TABULKA
    int32_t fat[desc.cluster_count];

    fat[0] = FAT_FILE_END;
    fat[1] = 2;
    fat[2] = 3;
    fat[3] = 4;
    fat[4] = 5;
    fat[5] = 6;
    fat[6] = 7;
    fat[7] = 8;
    fat[8] = 9;
    fat[9] = 10;
    fat[10] = 11;
    fat[11] = 12;
    fat[12] = 13;
    fat[13] = 14;
    fat[14] = 15;
    fat[15] = 16;
    fat[16] = 17;
    fat[17] = 18;
    fat[18] = 19;
    fat[19] = 20;
    fat[20] = 21;
    fat[21] = 22;
    fat[22] = 23;
    fat[23] = 24;
    fat[24] = FAT_FILE_END;
    fat[25] = FAT_FILE_END;
    fat[26] = FAT_UNUSED;
    fat[27] = FAT_UNUSED;
    fat[28] = FAT_UNUSED;
    fat[29] = FAT_FILE_END;
    fat[30] = FAT_FILE_END;
    fat[31] = FAT_FILE_END;
    fat[32] = FAT_FILE_END;

    for (int32_t i = 33; i < desc.cluster_count; i++)
    {
        fat[i] = FAT_UNUSED;
    }

    file = fopen("empty.fat", "w+");
    fwrite(&desc, sizeof(desc), 1, file);
    fwrite(&fat, sizeof(fat), 1, file);

    desc.fat1_start_address = sizeof(desc);
    desc.data_start_address = desc.fat1_start_address + sizeof(fat);

    int16_t cl_size = desc.cluster_size;
    int16_t ac_size = 0;

    fwrite(&root_dir, sizeof(root_dir), 1, file);
    ac_size += sizeof(root_dir);
    fwrite(&root_a, sizeof(root_a), 1, file);
    ac_size += sizeof(root_a);
    fwrite(&root_b, sizeof(root_b), 1, file);
    ac_size += sizeof(root_b);
    fwrite(&root_c, sizeof(root_c), 1, file);
    ac_size += sizeof(root_c);
    fwrite(&root_d, sizeof(root_d), 1, file);
    ac_size += sizeof(root_d);
    fwrite(&root_e, sizeof(root_e), 1, file);
    ac_size += sizeof(root_e);
    fwrite(&root_f, sizeof(root_f), 1, file);
    ac_size += sizeof(root_f);

    char buffer[] = { '\0' };
    for (int16_t i = 0; i < (cl_size - ac_size); i++) {
        fwrite(buffer, sizeof(buffer), 1, file);
    }

    fwrite(&cluster_root, sizeof(cluster_root), 1, file);

    fwrite(&cluster_b1, sizeof(cluster_b1), 1, file);
    fwrite(&cluster_b2, sizeof(cluster_b2), 1, file);
    fwrite(&cluster_b3, sizeof(cluster_b3), 1, file);
    fwrite(&cluster_b4, sizeof(cluster_b4), 1, file);
    fwrite(&cluster_b5, sizeof(cluster_b5), 1, file);
    fwrite(&cluster_b6, sizeof(cluster_b6), 1, file);
    fwrite(&cluster_b7, sizeof(cluster_b7), 1, file);
    fwrite(&cluster_b8, sizeof(cluster_b8), 1, file);
    fwrite(&cluster_b9, sizeof(cluster_b9), 1, file);
    fwrite(&cluster_b10, sizeof(cluster_b10), 1, file);
    fwrite(&cluster_b11, sizeof(cluster_b11), 1, file);
    fwrite(&cluster_b12, sizeof(cluster_b12), 1, file);
    fwrite(&cluster_b13, sizeof(cluster_b13), 1, file);
    fwrite(&cluster_b14, sizeof(cluster_b14), 1, file);
    fwrite(&cluster_b15, sizeof(cluster_b15), 1, file);
    fwrite(&cluster_b16, sizeof(cluster_b16), 1, file);
    fwrite(&cluster_b17, sizeof(cluster_b17), 1, file);
    fwrite(&cluster_b18, sizeof(cluster_b18), 1, file);
    fwrite(&cluster_b19, sizeof(cluster_b19), 1, file);
    fwrite(&cluster_b20, sizeof(cluster_b20), 1, file);
    fwrite(&cluster_b21, sizeof(cluster_b21), 1, file);
    fwrite(&cluster_b22, sizeof(cluster_b22), 1, file);
    fwrite(&cluster_b23, sizeof(cluster_b23), 1, file);
    fwrite(&cluster_b24, sizeof(cluster_b24), 1, file);

    fwrite(&cluster_a, sizeof(cluster_a), 1, file);
    fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);
    fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);
    fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);


    fwrite(&cluster_dir1, sizeof(cluster_dir1), 1, file);
    fwrite(&cluster_cccc, sizeof(cluster_cccc), 1, file);
    fwrite(&cluster_dir2, sizeof(cluster_dir2), 1, file);
    fwrite(&cluster_hoven, sizeof(cluster_hoven), 1, file);

    for (long i = 33; i < desc.cluster_count; i++)
    {
        fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);
    }

    char vstup[50];
    char fce[10];
    char param1[20];
    char param2[50];
    char* token;
    char* rest = NULL;
    char* path[MAX_DIR_IMMERSION];
    int prikaz;
    int offset, index;
    directory_item dir;
    int32_t fat_tab[desc.cluster_count];
    char enter;
    char null[5];
    char path_actual[200]; 
    strcpy(path_actual, "/root");

    while (true) {
        memset(&vstup, '\0', sizeof(vstup));
        memset(&param1, '\0', sizeof(param1));
        memset(&param2, '\0', sizeof(param2));

        fgets(vstup, 50, stdin);
        vstup[strcspn(vstup, "\n")] = 0;
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

        prikaz = getPrikaz(fce);        

        switch (prikaz) {

        case 1:
            if (strchr(param1, '/') != NULL) {
                getFileName(file, param1);
            }

            token = strtok(param2, "/");
            index = 0;
            while (token != NULL) {
                path[index] = token;
                index++;
                token = strtok(NULL, "/");
            }

            if (isValidPath(file, path, index - 2) && index > 1) {
                                 
            dir = get_directory_item(file, param1);
                if (strcmp(dir.item_name, null) == 0) {
                    printf("FILE NOT FOUND\n");
                    break;
                }
            get_fat(file, fat_tab);
            copyFile(dir, fat_tab, file, path, index);
            }
            else {
                printf("PATH NOT FOUND \n");
            }
            break;

        case 2:
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
                  
                    char* s = strdup(path[index - 1]);
                    strcpy(param1, path[index - 2]);
                    dir = get_directory_item(file, s);                              
                    src = get_directory_item(file, param1);                  
                }
                else {    
                    printf("FILE NOT FOUND\n");
                    break;
                }
            }
            else {
                dir = get_directory_item(file, param1);
                getActDirectory(file, path_actual, param1);
                src = get_directory_item(file, param1);
            }

            if (strcmp(dir.item_name, null) == 0 || strcmp(src.item_name, null) == 0) {
                printf("2");
                printf("FILE NOT FOUND\n");
                break;
            }

            if (strchr(param2, '/') != NULL) {
                token = strtok(param2, "/");
                index = 0;
                while (token != NULL) {
                    path[index] = token;
                    index++;
                    token = strtok(NULL, "/");
                }
                if (isValidPath(file, path, index - 2) && index > 1) {             
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
                    printf("PATH NOT FOUND \n");
                    break;
                }               
            }
            else {                
                dest = get_directory_item(file, param2);
                if (strcmp(dest.item_name, null) == 0) {
                    getActDirectory(file, path_actual, param1);
                    dest = get_directory_item(file, param1);
                }
                else {
                    getActDirectory(file, path_actual, param1);
                    path[0] = param1;
                    path[1] = param2;
                    if (isValidPath(file, path, 1)) {
                        strcpy(param2, dir.item_name);
                    }
                }                                       
                
            }
            get_fat(file, fat_tab);
            moveFile(dir, src, dest, fat_tab, file, param2);
            break;

        case 3:
            if (strchr(param1, '/') != NULL) {
                getFileName(file, param1);
            }
            dir = get_directory_item(file, param1);
            if (strcmp(dir.item_name, null) == 0 || dir.isFile == 0) {
                printf("FILE NOT FOUND\n");
                break;
            }
            else {
                get_fat(file, fat_tab);               
                deleteFromDir(dir.item_name, fat_tab, file);
                deleteFile(dir, fat_tab, file);
            }
            break;

        case 4:           
            if (strchr(param1, '/') != NULL) {
                token = strtok(param1, "/");
                index = 0;
                while (token != NULL) {
                    path[index] = token;
                    index++;
                    token = strtok(NULL, "/");
                }
                if (isValidPath(file, path, index - 2)) {
                    dir = get_directory_item(file, path[index - 2]);
                    strcpy(param1, path[index - 1]);
                }
                else {
                    printf("PATH NOT FOUND \n");
                    break;
                }
            }
            else {
                getActDirectory(file, path_actual, param2);
                dir = get_directory_item(file, param2);
            }
            get_fat(file, fat_tab);
            if (makeDirectory(dir, param1, file, fat_tab)) {
                printf("OK\n");
                break;
            }
            else {
                printf("EXIST\n");
                break;
            }

        case 5:
            if (strchr(param1, '/') != NULL) {
                token = strtok(param1, "/");
                index = 0;
                while (token != NULL) {
                    path[index] = token;
                    index++;
                    token = strtok(NULL, "/");
                }
                if (isValidPath(file, path, index - 1)) {
                    dir = get_directory_item(file, path[index-1]);
                }
                else {
                    printf("FILE NOT FOUND\n");
                    break;
                }
            }
            else {
                dir = get_directory_item(file, param1);
            }
            if (strcmp(dir.item_name, null) == 0 || dir.isFile == 1) {
                printf("FILE NOT FOUND\n");
                break;
            }
            else {
                if (isEmptyDir(dir, file)) {
                    get_fat(file, fat_tab);
                    deleteFromDir(dir.item_name, fat_tab, file);
                    deleteFile(dir, fat_tab, file);
                }else{
                    printf("NOT EMPTY\n");
                    break;
                }
            }
            break;

        case 6:            
            if (strlen(param1) == 0) {
                getActDirectory(file, path_actual, param1);      
                printf("dirname %s\n", param1);
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
                    dir = get_directory_item(file, path[index-1]);                   
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
            break;
        case 7:
            if (strchr(param1, '/') != NULL) {
                getFileName(file, param1);                
            }
            dir = get_directory_item(file, param1);           
            if (strcmp(dir.item_name, null) == 0 || dir.isFile == 0) {
                printf("FILE NOT FOUND\n");
                break;
            }
            else {
                get_fat(file, fat_tab);
                printFile(dir, fat_tab, file);
                break;
            }

        case 8:    
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
                    getActDirectory(file, path_actual, param2);
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
                        break;
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
                    getActDirectory(file, path_actual, param2);
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
                        break;
                    }
                }
            }
            printf("%s> ", path_actual);
            break;            

        case 9:
            printf("%s> ", path_actual);
            break;

        case 10:
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
                    break;
                }               
            }
            else {
                dir = get_directory_item(file, param1);
            }
            if (strcmp(dir.item_name, null) == 0) {
                printf("FILE NOT FOUND\n");
                break;
            }else{
                get_fat(file, fat_tab);
                printInfo(dir, fat_tab);
                break;
            }
        case 11:
            FILE * input;
            input = fopen(param1, "r");
            if (NULL == input) {
                printf("FILE NOT FOUND\n");
                break;
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
                    strcpy(param2, path[index - 1]);
                    getActDirectory(file, path_actual, param1);
                    dir = get_directory_item(file, param1);
                }
            }
            else {
                getActDirectory(file, path_actual, param1);
                dir = get_directory_item(file, param1);
            }
            get_fat(file, fat_tab);
            uploadFile(dir, param2, fat_tab, file, input);
            break;
                       
        case 15:
            printf("Neplatny prikaz\n");
            break;

        case 16:
            printf("Konec\n");
            return 0;

        default:
            printf("ERROR\n");
            return -1;
        }
    }
        fclose(file);

        return 0;
    
}

    int getPrikaz(char* vstup) {
        char* token;
        token = strtok(vstup, " ");
        if (strcmp(vstup, "cp") == 0) {
            return 1;
        }
        if (strcmp(vstup, "mv") == 0) {
            return 2;
        }
        if (strcmp(vstup, "rm") == 0) {
            return 3;
        }
        if (strcmp(vstup, "mkdir") == 0) {
            return 4;
        }
        if (strcmp(vstup, "rmdir") == 0) {
            return 5;
        }
        if (strcmp(vstup, "ls") == 0) {
            return 6;
        }
        if (strcmp(vstup, "cat") == 0) {
            return 7;
        }
        if (strcmp(vstup, "cd") == 0) {            
            return 8;
        }
        if (strcmp(vstup, "pwd") == 0) {
            return 9;
        }
        if (strcmp(vstup, "info") == 0) {
            return 10;
        }
        if (strcmp(vstup, "incp") == 0) {
            return 11;
        }
        if (strcmp(vstup, "exit") == 0) {
            return 16;
        }
        else {
            return 15;
        }
    }