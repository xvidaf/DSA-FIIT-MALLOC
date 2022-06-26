#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct header {
    unsigned int size;
    struct header *nextFree;
};

struct header *first;

struct headerAllocated {
    unsigned int size;
    char allocated;
};

void *memory_alloc(unsigned int size) {
    struct header *current;
    current = first;
    int difference = -1;
    struct header *currentBestFit;
    //Hladame best fit
    if (first->nextFree == NULL) {
        if (first->size >= size + sizeof(struct headerAllocated)) {
            difference = 0;
            currentBestFit = first;
        }
    } else {
        while (current->nextFree != NULL) {
            if ((difference == -1 && current->size >= size + sizeof(struct headerAllocated))
                || (current->size >= size + sizeof(struct headerAllocated) && current->size - size < difference)) {
                difference = current->size - size;
                currentBestFit = current;
            }
            current = current->nextFree;
        }
        if ((difference == -1 && current->size >= size + sizeof(struct headerAllocated))
            || (current->size >= size + sizeof(struct headerAllocated) && current->size - size < difference)) {
            difference = current->size - size;
            //printf("Novy difference je %d\n", difference);
            currentBestFit = current;
        }
    }
    current = currentBestFit;
    if (difference != -1) {
        //Premiestnenie free
        unsigned int newSize;
        struct header *newNextFree;
        newSize = current->size;
        newNextFree = current->nextFree;
        //Struktura pre alokovane miesto
        struct headerAllocated *newAllocated;
        newAllocated = current;
        //newAllocated -> size = size;
        //newAllocated -> allocated = 1;
        int fragmentation = 0;
        //Vonkajsia fragmentacia
        if (newSize - size - sizeof(struct headerAllocated) < 9/*sizeof(struct header) + 1*/) {
            fragmentation = newSize - size - sizeof(struct headerAllocated);
            newAllocated->size += fragmentation;
        }
        //Ak po alokovani ostane volne miesto
        if (newSize != size + sizeof(struct headerAllocated) && fragmentation == 0) {
            if (first == currentBestFit) {
                printf("Po alokovani ostalo miesto a je to first\n");
                void *oldNext = first->nextFree;
                void *pointer = (void *) first;
                first = pointer + sizeof(struct headerAllocated) +
                        size;
                first->size = newSize - size - sizeof(struct headerAllocated);
                first->nextFree = oldNext;
            } else {
                printf("Po alokovani ostalo volne miesto a prvok nebol first");
                struct header *helper = first;
                while (helper->nextFree != current) {
                    helper = helper->nextFree;
                }
                void *novyNext = current->nextFree;
                void *pointer = (void *) current;
                current = pointer + sizeof(struct headerAllocated) + size;
                current->size = newSize - size - sizeof(struct headerAllocated);
                current->nextFree = novyNext;
                helper->nextFree = current;
            }
        } else {
            //Ak vyplnime celu dostupnu pamet, nieje ziadna volna, vytvorime novy first aby sa nestratil ale dame ho na novu random adresu
            if (current->nextFree == NULL && current == first) {
                struct header novyFirst;
                novyFirst.size = 0;
                //novyFirst.allocated = 0;
                novyFirst.nextFree = NULL;
                first = &novyFirst;
            }
                //Ak je vyplnene miesto na konci linked listu ale nieje jeho zaciatok
            else if (current->nextFree == NULL && current != first) {
                printf("Vyplnene miesto je na konci linked listu ale nieje jeho zaciatok");
                struct header *helper;
                helper = first;
                while (helper != NULL) {
                    if (helper->nextFree == currentBestFit) {
                        helper->nextFree = NULL;
                    }
                    helper = helper->nextFree;
                }
            }
                //Ak je vyplnene miesto na zaciatku linked listu a existuju este ine
            else if (current->nextFree != NULL && current == first) {
                printf("vyplnene miesto je na zaciatku linked listu a existuju este ine");
                first = current->nextFree;
            }
                //Ak je vyplnene miesto v strede linked listu
            else if (current->nextFree != NULL && current != first) {
                printf("Vyplnene miesto je medzi 2 prvkami");
                struct header *helper;
                helper = first;
                while (helper != NULL) {
                    if (helper->nextFree == current) {
                        helper->nextFree = helper->nextFree->nextFree;
                        break;
                    }
                }
            }
            printf("Dostupna velkost je presne rovnako velka\n");
            //current->size = 0;
        }
        newAllocated->size = size + fragmentation;
        newAllocated->allocated = 1;
        //Pre jednoduchost citania
        void *pointer1 = (void *) newAllocated;
        if (newAllocated)
            memset(pointer1 + sizeof(struct headerAllocated), 34, size);
        void *pointer11 = (void *) newAllocated;
        printf("Alokovalo sa\n");
        return pointer11 + sizeof(struct headerAllocated);
    } else {
        printf("Nenasla sa dost velka pamet\n");
        return NULL;
    }
}

int memory_check(void *ptr) {
    if (ptr != NULL && *(int *) (ptr - 8) > 0 && *(char *) (ptr - 4) == 1) {
        return 1;
    } else {
        //printf("Nepresiel memory check");
        return 0;
    }
}

int memory_free(void *valid_ptr) {
    //MOZNOSTI
    if (memory_check(valid_ptr) == 1) {
        void *prvokPred;
        void *prvokPo;
        //Ak neexistuje volne miesto
        if (first->size == 0) {
            void *pointer = (void *) valid_ptr;
            int size = *(int *) (valid_ptr - 8);
            first = (void *) pointer - 8;
            if (first)
                memset(first, 17, size + 8);
            first->size = size + 8;
            first->nextFree = NULL;
            printf("Neexistovalo volne miesto, uvolnil sa novy first\n");
            return 0;
        } else {
            //printf("%d\n",first->size);
            int nasloPred = 0;
            int nasloZa = 0;
            struct header *current;
            current = first;
            int size = *(int *) (valid_ptr - 8);
            void *pointer = (void *) valid_ptr;
            void *pointer1 = (void *) current;
            //Hladame oblasti presne pred a za
            while (current != NULL) {
                pointer1 = current;
                if (current == pointer + size) {
                    //printf("Naslo sa pred pointrom");
                    nasloPred = 1;
                    prvokPred = current;
                } else if (pointer1 + current->size == pointer - 8) {
                    //printf("Naslo sa za pointrom");
                    prvokPo = current;
                    nasloZa = 1;
                }
                current = current->nextFree;
            }
            //Pripady pre najdene moznosti
            //Alokovana pamet je medzi 2 prvkami linked listu
            if (nasloPred == 1 && nasloZa == 1) {
                printf("Alokovana pamet bolo medzi 2 prvkami\n");
                struct header *currentHelper = prvokPred;
                struct header *currentHelper2 = first;
                //struct header *beforeAllocated = prvokPred;
                //struct header *afterAllocated = prvokPo;
                int size1 = *(int *) (prvokPred);
                int size2 = *(int *) (prvokPo);
                //Vymazeme prvokPred
                //Ak je prvokPred first
                if (prvokPred == first) {
                    //printf("Sme tu");
                    first = first->nextFree;
                }
                    //Ak je prvokPred na konci, hladame prvok pred prvokPred
                else if (currentHelper->nextFree == NULL) {
                    //printf("Sme tu 1");
                    while (currentHelper2->nextFree != prvokPred) {
                        currentHelper2 = currentHelper2->nextFree;
                    }
                    currentHelper2->nextFree = NULL;
                }
                    //Ak je prvok medzi 2mi prvkami, hladame prvok pred a nastavime ho na prvokPred -> next;
                else if (currentHelper->nextFree != NULL && prvokPred != first) {
                    //printf("Sme tu 2");
                    while (currentHelper2->nextFree != prvokPred) {
                        currentHelper2 = currentHelper2->nextFree;
                    }
                    currentHelper2->nextFree = currentHelper->nextFree;
                } else {
                    printf("ERROR pri prvkuPred");
                    return 1;
                }
                //Vymazeme prvokPo
                currentHelper = prvokPo;
                currentHelper2 = first;
                //Ak je prvokZa first
                if (prvokPo == first) {
                    first = first->nextFree;
                }
                    //Ak je prvokZa na konci, hladame prvok pred prvokPred
                else if (currentHelper->nextFree == NULL) {
                    while (currentHelper2->nextFree != prvokPo) {
                        currentHelper2 = currentHelper2->nextFree;
                    }
                    currentHelper2->nextFree = NULL;
                }
                    //Ak je prvok medzi 2mi prvkami, hladame prvok pred a nastavime ho na prvokPred -> next;
                else if (currentHelper->nextFree != NULL && prvokPo != first) {
                    while (currentHelper2->nextFree != prvokPo) {
                        currentHelper2 = currentHelper2->nextFree;
                    }
                    currentHelper2->nextFree = currentHelper->nextFree;
                } else {
                    printf("ERROR pri prvkuPo");
                    return 1;
                }
                //Vytvorime novy velky blok
                if (prvokPred) {
                    memset(prvokPo, 17, size + size1 + size2 + 8);
                }
                if (first == NULL) {
                    first = prvokPo;
                    first->size = size + size1 + size2 + 8;
                    //first -> allocated = 0;
                    first->nextFree = NULL;
                } else {
                    struct header *newJoinedBlocks;
                    newJoinedBlocks = prvokPo;
                    newJoinedBlocks->size = size + size1 + size2 + 8;
                    //newJoinedBlocks -> allocated = 0;
                    newJoinedBlocks->nextFree = NULL;
                    current = first;
                    while (current->nextFree != NULL) {
                        current = current->nextFree;
                    }
                    current->nextFree = newJoinedBlocks;

                }
                return 0;
            }
                //Alokovana pamet je pred prvkom v linked listu
            else if (nasloPred == 1 && nasloZa == 0) {
                printf("Alokovana pamet bola pred linked listom\n");
                if (valid_ptr)
                    memset(valid_ptr - 8, 17, size + 8);
                struct header *currentHelper;
                currentHelper = first;
                while (currentHelper->nextFree != NULL) {
                    if (currentHelper->nextFree == prvokPred) {
                        //printf("Nasiel sa prvok\n");
                        break;
                    }
                    currentHelper = currentHelper->nextFree;
                }
                if (prvokPred != first) {
                    current = prvokPred;
                    int sizeOriginal = current->size;
                    void *nextOriginal = current->nextFree;
                    current = valid_ptr - 8;
                    currentHelper->nextFree = current;
                    current->size = sizeOriginal + size + 8;
                    current->nextFree = nextOriginal;
                    //current->allocated = 0;
                    pointer = current;
                    if (current)
                        memset(pointer + 12, 17, current->size - 12);
                } else if (prvokPred == first) {
                    current = prvokPred;
                    int sizeOriginal = current->size;
                    void *nextOriginal = current->nextFree;
                    current = valid_ptr - 8;
                    current->nextFree = current;
                    current->size = sizeOriginal + size + 8;
                    current->nextFree = nextOriginal;
                    //current->allocated = 0;
                    first = current;
                    pointer = current;
                    if (current)
                        memset(pointer + 12, 17, current->size - 12);
                }
                return 0;
            }
                //Alokovana pamet je presne za prvkom v linked liste
            else if (nasloPred == 0 && nasloZa == 1) {
                printf("Alokovana pamet bolo za linked listom\n");
                if (valid_ptr)
                    memset(valid_ptr - 8, 17, size + 8);
                current = prvokPo;
                current->size = current->size + size + 8;
                return 0;
            }
                //Alokovana pamet nieje spojena s volnou pametou
            else if (nasloPred == 0 && nasloZa == 0) {
                printf("Alokovane miesto nieje spojene s linked listom\n");
                struct header *pointerFreeNew = valid_ptr - 8;
                if (pointerFreeNew)
                    memset(pointerFreeNew, 17, size + 8);
                pointerFreeNew->size = size + 8;
                //pointerFreeNew -> allocated = 0;
                pointerFreeNew->nextFree = NULL;
                current = first;
                while (current->nextFree != NULL) {
                    current = current->nextFree;
                }
                current->nextFree = pointerFreeNew;
                return 0;
            }
        }
    } else {
        printf("Nepresiel memory check\n");
        return 1;
    }
}

void memory_init(void *ptr, unsigned int size) {
    //Pre zlahsenie viditelnosti
    if (ptr)
        memset(ptr, 17, size);
    first = ptr;
    first->size = size;
    //first -> allocated = 0;
    first->nextFree = NULL;
}

//Test 100 bytov, alokujeme po 8 bytoch
void test1() {
    printf("\nTEST 1\n");
    char *region;
    unsigned int initialSize = 100; //Velkost dostupnej pamete
    region = (char *) malloc(initialSize); //celkový blok pamäte o veľkosti 50 bytov
    memory_init(region, initialSize);
    char *pole1 = memory_alloc(8);
    char *pole2 = memory_alloc(8);
    char *pole3 = memory_alloc(8);
    char *pole4 = memory_alloc(8);
    char *pole5 = memory_alloc(8);
    char *pole6 = memory_alloc(8);
    char *pole7 = memory_alloc(8);
    char *pole8 = memory_alloc(8);
    memory_free(pole5);
    memory_free(pole1);
    memory_free(pole3);
    memory_free(pole2);
    memory_free(pole4);
    memory_free(pole6);
    memory_free(pole7);
    memory_free(pole8);
    printf("Alokovali sme 6 z 8 poli, pouzivatelia dostali na pracu 48 bytov, efektivne vyuzita pamat bola po pridani hlaviciek 96, 4 byti sa pridali do jedneho z blokov ako vnutorna fragmentacia");
}

//Test 100 bytov, alokujeme po rozne velkosti a medzi tymaj freeujeme
void test2() {
    printf("\nTEST 2\n");
    char *region;
    unsigned int initialSize = 100; //Velkost dostupnej pamete
    region = (char *) malloc(initialSize); //celkový blok pamäte o veľkosti 50 bytov
    memory_init(region, initialSize);
    printf("\n%d\n", first->size);
    char *pole1 = memory_alloc(8);
    char *pole2 = memory_alloc(8);
    char *pole3 = memory_alloc(8);
    memory_free(pole1);
    pole1 = memory_alloc(8); // Otestovanie mallocu pokial allocneme cely prvok a je na konci zoznamu a nieje first
    char *pole4 = memory_alloc(8);
    char *pole5 = memory_alloc(8);
    char *pole6 = memory_alloc(8);
    char *pole7 = memory_alloc(8);
    memory_free(pole1);
    memory_free(pole6);
    pole1 = memory_alloc(12);// Test mallocu ked zoberieme celu velkost free prvku, ktory je zaroven free a nieje jediny v linked liste
    pole6 = memory_alloc(8);// Test mallocu ked sa neostane ziadny nealokovany blok
    memory_free(pole1);
    memory_free(pole2);
    memory_free(pole4);
    memory_free(pole6);
    pole1 = memory_alloc(10); // Test mallocu ked je alokujeme na first a ostane v nom miesto
    pole2 = memory_alloc(20); // Test mallocu ked alokujeme na miesto medzi 2 prvkami a ostane zvysok
    memory_free(pole1);
    memory_free(pole2);
    memory_free(pole5);
    memory_free(pole3);
    pole1 = memory_alloc(4); // Test mallocu ked alokujeme na miesto medzi 2 prvkami a neostane zvysok
    pole2 = memory_alloc(4);
    pole3 = memory_alloc(4);
    pole4 = memory_alloc(16);
    pole5 = memory_alloc(4);
    pole6 = memory_alloc(4);
    memory_free(pole1);
    memory_free(pole4);
    memory_free(pole6);
    pole1 = memory_alloc(2);
    memory_free(pole1);
    memory_free(pole2);
    memory_free(pole3);
    memory_free(pole5);

    //printf("Ahoj");
}

//Test pre vacsie hodnoty
void test3() {
    printf("\nTEST 3\n");
    char *region;
    unsigned int initialSize = 5000; //Velkost dostupnej pamete
    region = (char *) malloc(initialSize); //celkový blok pamäte o veľkosti 50 bytov
    memory_init(region, initialSize);
    //printf("\n%d\n",first->size);
    //memory_alloc(16);
    //memory_alloc(16);
    char *pole1 = memory_alloc(1000);
    char *pole2 = memory_alloc(1000);
    char *pole3 = memory_alloc(1000);
    char *pole4 = memory_alloc(750);
    char *pole5 = memory_alloc(500);
    char *pole6 = memory_alloc(600);
    memory_free(pole1);
    memory_free(pole3);
    memory_free(pole2);
    memory_free(pole4);
    memory_free(pole5);
    memory_free(pole6);
}

//Test pre vacsie hodnoty pri alokovani roznych hodnot
void test4() {
    printf("\nTEST 4\n");
    char *region;
    unsigned int initialSize = 5000; //Velkost dostupnej pamete
    region = (char *) malloc(initialSize); //celkový blok pamäte o veľkosti 50 bytov
    memory_init(region, initialSize);
    //printf("\n%d\n",first->size);
    //memory_alloc(16);
    //memory_alloc(16);
    char *pole1 = memory_alloc(1000);
    char *pole2 = memory_alloc(1000);
    char *pole3 = memory_alloc(1000);
    char *pole4 = memory_alloc(750);
    char *pole5 = memory_alloc(500);
    char *pole6 = memory_alloc(600);
    char *pole7 = memory_alloc(8);
    char *pole8 = memory_alloc(16);
    char *pole9 = memory_alloc(50);
    memory_free(pole1);
    memory_free(pole3);
    memory_free(pole2);
    memory_free(pole4);
    memory_free(pole5);
    memory_free(pole6);
}

int main() {

    test1();
    //test2();
    //test3();
    //test4();

    return 0;
}


