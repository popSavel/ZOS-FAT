/* ========================================================================== */
/*                                                                            */
/*   File: main.cpp                                                               */
/*   Author: saveld                                                         */
/*                                                                            */
/*   Created: 23.10.2022                                                           */
/*                                                                            */
/* ========================================================================== */

#include <cstdio>
#include <cstring>
#include <cstdint>

const int32_t FAT_UNUSED = INT32_MAX - 1;
const int32_t FAT_FILE_END = INT32_MAX - 2;
const int32_t FAT_BAD_CLUSTER = INT32_MAX - 3;

struct description {
    char signature[9];              //login autora FS
    int32_t disk_size;              //celkova velikost VFS
    int32_t cluster_size;           //velikost clusteru
    int32_t cluster_count;          //pocet clusteru
    int32_t fat_count;        	    //pocet polozek kazde FAT tabulce
    int32_t fat1_start_address;	    //adresa pocatku FAT1 tabulky
    int32_t data_start_address;     //adresa pocatku datovych bloku (hl. adresar)  
};

struct directory_item {
    char item_name[13];              //8+3 + /0 C/C++ ukoncovaci string znak
    bool isFile;		     //identifikace zda je soubor (TRUE), nebo adresáø (FALSE)
    int32_t size;                    //velikost souboru, u adresáøe 0 (bude zabirat jeden blok)
    int32_t start_cluster;           //poèáteèní cluster položky
};

int getPrikaz(char* vstup);


int main(int argc, char** argv){

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
    struct description desc;
    struct directory_item root_a, root_b, root_c;
    //smazani /0 u stringu
    memset(desc.signature, '\0', sizeof(desc.signature));
    desc.cluster_size = 256;
    desc.cluster_count = 252;
    strcpy(desc.signature, name);
    printf("name: %s\n", desc.signature);

    /*
    * vytvoøení root položek
    */
    memset(root_a.item_name, '\0', sizeof(root_a.item_name));
    root_a.isFile = 1;
    strcpy(root_a.item_name, "cisla.txt");
    root_a.size = 135;
    root_a.start_cluster = 1;

    memset(root_b.item_name, '\0', sizeof(root_b.item_name));
    root_b.isFile = 1;
    strcpy(root_b.item_name, "pohadka.txt");
    root_a.size = 5975;
    root_a.start_cluster = 2;

    /*
    * directory
    */
    memset(root_c.item_name, '\0', sizeof(root_c.item_name));
    root_c.isFile = 0;
    strcpy(root_c.item_name, "direct-1");
    root_c.size = 0;
    root_c.start_cluster = 29;

    char cluster_empty[desc.cluster_size];
    char cluster_dir1[desc.cluster_size];
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
    strcpy(cluster_a, "0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789 - tohle je malicky soubor pro test");
    strcpy(cluster_b1, "Byla jednou jedna sladka divenka, kterou musel milovat kazdy, jen ji uvidel, ale nejvice ji milovala jeji babicka, ktera by ji snesla i modre z nebe. Jednou ji darovala cepecek karkulku z cerveneho sametu a ten se vnucce tak libil, ze nic jineho nechtela ");
    strcpy(cluster_b2, "nosit, a tak ji zacali rikat Cervena Karkulka. Jednou matka Cervene Karkulce rekla: „Podivej, Karkulko, tady mas kousek kolace a lahev vina, zanes to babicce, je nemocna a zeslabla, timhle se posilni. Vydej se na cestu drive nez bude horko, jdi hezky spor");
    strcpy(cluster_b3, "adane a neodbihej z cesty, kdyz upadnes, lahev rozbijes a babicka nebude mit nic. A jak vejdes do svetnice, nezapomeò babicce poprat dobreho dne a ne abys smejdila po vsech koutech.“ „Ano, maminko, udelam, jak si prejete.“ rekla Cerveni Karkulka, na stvrz");
    strcpy(cluster_b4, "eni toho slibu podala matce ruku a vydala se na cestu. Babicka bydlela v lese; celou pùlhodinu cesty od vesnice. Kdyz sla Cervena Karkulka lesem, potkala vlka. Tenkrat jeste nevedela, co je to za zaludne zvire a ani trochu se ho nebala. „Dobry den, Cerven");
    strcpy(cluster_b5, "a Karkulko!“ rekl vlk. „Dekuji za prani, vlku.“ „Kampak tak casne, Cervena Karkulko?“ „K babicce!“ „A copak to neses v zasterce?“ „Kolac a vino; vcera jsme pekli, nemocne a zeslable babicce na posilnenou.“ „Kdepak bydli babicka, Cervena Karkulko?“ „Inu, j");
    strcpy(cluster_b6, "este tak ctvrthodiny cesty v lese, jeji chaloupka stoji mezi tremi velkymi duby, kolem je liskove oresi, urcite to tam musis znat.“ odvetila Cervena Karkulka. Vlk si pomyslil: „Tohle mlaïoucke, jemòoucke masicko bude jiste chutnat lepe nez ta starena, mus");
    strcpy(cluster_b7, "im to navleci lstive, abych schlamstnul obe.“ Chvili sel vedle Cervene Karkulky a pak pravil: „Cervena Karkulko, koukej na ty krasne kvetiny, ktere tu rostou vsude kolem, procpak se trochu nerozhlednes? Myslim, ze jsi jeste neslysela ptacky, kteri by zpiv");
    strcpy(cluster_b8, "ali tak libezne. Ty jsi tu vykracujes, jako kdybys sla do skoly a pritom je tu v lese tak krasne!“ Cervena Karkulka otevrela oci dokoran a kdyz videla, jak slunecni paprsky tancuji skrze stromy sem a tam a vsude roste tolik krasnych kvetin, pomyslila si: ");
    strcpy(cluster_b9, "„Kdyz prinesu babicce kytici cerstvych kvetin, bude mit jiste radost, casu mam dost, prijdu akorat.“ A sebehla z cesty do lesa a trhala kvetiny. A kdyz jednu utrhla, zjistila, ze o kus dal roste jeste krasnejsi, bezela k ni, a tak se dostavala stale hloub");
    strcpy(cluster_b10, "eji do lesa. Ale vlk bezel rovnou k babiccine chaloupce a zaklepal na dvere. „Kdo je tam?“ „Cervena Karkulka, co nese kolac a vino, otevri!“ „Jen zmackni kliku,“ zavolala babicka: „jsem prilis slaba a nemohu vstat.“ Vlk vzal za kliku, otevrel dvere a beze");
    strcpy(cluster_b11, "slova sel rovnou k babicce a spolknul ji. Pak si obleknul jeji saty a nasadil jeji cepec, polozil se do postele a zatahnul zaves. Zatim Cervena Karkulka behala mezi kvetinami, a kdyz jich mela naruc tak plnou, ze jich vic nemohla pobrat, tu ji prisla na  ");
    strcpy(cluster_b12, "mysl babicka, a tak se vydala na cestu za ni. Podivila se, ze jsou dvere otevrene, a kdyz vesla do svetnice, prislo ji vse takove podivne, ze si pomyslila: „Dobrotivy Boze, je mi dneska nejak úzko a jindy jsem u babicky tak rada.“ Zvolala: „Dobre jitro!“ ");
    strcpy(cluster_b13, "Ale nedostala zadnou odpoveï. Šla tedy k posteli a odtahla zaves; lezela tam babicka a mela cepec narazeny hluboko do obliceje a vypadala nejak podivne. Ach, babicko, proc mas tak velke usi?“ „Abych te lepe slysela.“ „Ach, babicko, proc mas tak velke oci ");
    strcpy(cluster_b14, "?“ „Abych te lepe videla.“ „Ach, babicko, proc mas tak velke ruce?“ „Abych te lepe objala.“ „Ach, babicko, proc mas tak straslivou tlamu?“ „Abych te lepe sezrala!!“ Sotva vlk ta slova vyrknul, vyskocil z postele a ubohou Cervenou Karkulku spolknul. Kdyz t");
    strcpy(cluster_b15, "eï uhasil svoji zadostivost, polozil se zpatky do postele a usnul a z toho spanku se jal mocne chrapat. Zrovna sel kolem chaloupky lovec a pomyslil si: „Ta starenka ale chrape, musim se na ni podivat, zda neco nepotrebuje.“ Vesel do svetnice, a kdyz prist");
    strcpy(cluster_b16, "oupil k posteli, uvidel, ze v ni lezi vlk. „Tak prece jsem te nasel, ty stary hrisniku!“ zvolal lovec: „Uz mam na tebe dlouho policeno!“ Strhnul z ramene pusku, ale pak mu prislo na mysl, ze vlk mohl sezrat babicku a mohl by ji jeste zachranit. Nestrelil ");
    strcpy(cluster_b17, "tedy, nybrz vzal nùzky a zacal spicimu vlkovi parat bricho. Sotva ucinil par rezù, uvidel se cervenat karkulku a po par dalsich rezech vyskocila divenka ven a volala: „Ach, ja jsem se tolik bala, ve vlkovi je cernocerna tma.“ A potom vylez la ven i ziva b");
    strcpy(cluster_b18, "abicka; sotva dechu popadala. Cervena Karkulka pak nanosila obrovske kameny, kterymi vlkovo bricho naplnili, a kdyz se ten probudil a chtel uteci, kameny ho tak desive tizily, ze klesnul k zemi nadobro mrtvy. Ti tri byli spokojeni. Lovec stahnul vlkovi ko");
    strcpy(cluster_b19, "zesinu a odnesl si ji domù, babicka snedla kolac a vypila vino, ktere Cervena Karkulka prinesla, a opet se zotavila. A Cervena Karkulka? Ta si svatosvate prisahala: „Uz nikdy v zivote nesejdu z cesty do lesa, kdyz mi to maminka zakaze!“ O Cervene  Karkulc");
    strcpy(cluster_b20, "e se jeste vypravi, ze kdyz sla jednou zase k babicce s babovkou, potkala jineho vlka a ten se ji taky vemlouval a snazil se ji svest z cesty. Ale  ona se toho vystrihala a kracela rovnou k babicce, kde hned vypovedela, ze potkala vlka, ktery ji sice popr");
    strcpy(cluster_b21, "al dobry den, ale z oci mu koukala nekalota. „Kdyby to nebylo na verejne ceste, jiste by mne sezral!“ „Pojï,“  rekla babicka: „zavreme dobre dvere, aby nemohl dovnitr.“ Brzy nato zaklepal vlk a zavolal: „Otevri, babicko, ja jsem Cervena Karkulka a nesu ti");
    strcpy(cluster_b22, "pecivo!“ Ty dve vsak zùstaly jako peny a neotevrely. Tak se ten sedivak plizil kolem domu a naslouchal, pak vylezl na strechu, aby tam pockal, az Cervena Karkulka pùjde vecer domù, pak ji v temnote popadne a sezere. Ale babicka zle vlkovy úmysly odhalila ");
    strcpy(cluster_b23, ". Pred domem staly obrovske kamenne necky, tak Cervene  Karkulce rekla: „Vezmi vedro, devenko, vcera jsem varila klobasy, tak tu vodu nanosime venku do necek.“ Kdyz byly necky plne, stoupala vùne klobas nahoru az k vlkovu cenichu. Zavetril a natahoval krk");
    strcpy(cluster_b24, "tak daleko, ze se na strese vice neudrzel a zacal klouzat dolù, kde spadnul primo do necek a bidne se utopil.");
    strcpy(cluster_c1, "Prodej aktiv SABMilleru v Ceske republice, Polsku, Maïarsku, Rumunsku a na Slovensku je soucasti podminek pro prevzeti podniku ze strany americkeho pivovaru Anheuser-Busch InBev, ktere bylo dokonceno v rijnu. Krome Plzeòskeho Prazdroje zahrnuji prodavana ");
    strcpy(cluster_c2, "aktiva polske znacky Tyskie a Lech, slovensky Topvar, maïarsky Dreher a rumunsky Ursus. - Tento soubor je sice kratky, ale neni fragmentovany");

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
    fat[25] = FAT_UNUSED;
    fat[26] = FAT_UNUSED;
    fat[27] = FAT_UNUSED;
    fat[28] = FAT_UNUSED;
    fat[29] = FAT_FILE_END;

    for (int32_t i = 30; i < desc.cluster_count; i++)
    {
        fat[i] = FAT_UNUSED;
    }

    file = fopen("empty.fat", "w");
    fwrite(&desc, sizeof(desc), 1, file);
    fwrite(&fat, sizeof(fat), 1, file);

    desc.fat1_start_address = sizeof(desc);
    desc.data_start_address = desc.fat1_start_address + sizeof(fat);

    int16_t cl_size = desc.cluster_size;
    int16_t ac_size = 0;

    fwrite(&root_a, sizeof(root_a), 1, file);
    ac_size += sizeof(root_a);
    fwrite(&root_b, sizeof(root_b), 1, file);
    ac_size += sizeof(root_b);
    fwrite(&root_c, sizeof(root_c), 1, file);
    ac_size += sizeof(root_c);

    char buffer[] = { '\0' };
    for (int16_t i = 0; i < (cl_size - ac_size); i++) {
        fwrite(buffer, sizeof(buffer), 1, file);
    }

    fwrite(&cluster_a, sizeof(cluster_a), 1, file);

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

    fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);
    fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);
    fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);
    fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);

    fwrite(&cluster_dir1, sizeof(cluster_dir1), 1, file);

    for (long i = 30; i < desc.cluster_count; i++)
    {
        fwrite(&cluster_empty, sizeof(cluster_empty), 1, file);
    }
    fclose(file);

    char vstup [10];
    char param1[10];
   // memset(vstup, '\0', sizeof(vstup));    
    int prikaz;

    while (true) {
        memset(&vstup, '\0', sizeof(vstup));
       scanf("%s", &vstup);
       scanf("%s", &param1);
        scanf(vstup);
        printf("vstup: %s\n", vstup);
        prikaz = getPrikaz(vstup);

        switch (prikaz){
        
        case 1:
            //scanf("%s", &vstup);
            printf("file: %s\n", param1);
            break;
        
        case 15:
            printf("Neplatny prikaz\n");
            break;

        case 16:
            printf("KOnec\n");
            return 0;

        default:
            printf("ERROR\n");
            return -1;
        }
    }

	return 0;
}

int getPrikaz(char * vstup) {
    char* token;
    token = strtok(vstup, " ");
    /*
    if (strcmp(token, "cat") == 0) {
        return 1;
    }*/
    
    if (strcmp(vstup, "cat") == 0) {
        return 1;
    }
    if (strcmp(vstup, "exit") == 0) {
        return 16;
    }
    else {
        return 15;
    }
}
