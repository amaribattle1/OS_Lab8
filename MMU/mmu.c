#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "list.h"
#include "util.h"

// Convert string to uppercase
void convertToUpperCase(char *arr) {
    for (int i = 0; i < (int)strlen(arr); i++) {
        arr[i] = toupper((unsigned char)arr[i]);
    }
}

// Parse input and determine memory management policy
void parseInput(char *args[], int input[][2], int *n, int *partitionSize, int *policy) {
    FILE *inputFile = fopen(args[1], "r");
    if (!inputFile) {
        fprintf(stderr, "Error: Unable to open file\n");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    parse_file(inputFile, input, n, partitionSize);
    fclose(inputFile);

    convertToUpperCase(args[2]);

    if ((strcmp(args[2], "-F") == 0) || (strcmp(args[2], "-FIFO") == 0)) {
        *policy = 1;  // First Fit
    } else if ((strcmp(args[2], "-B") == 0) || (strcmp(args[2], "-BESTFIT") == 0)) {
        *policy = 2;  // Best Fit
    } else if ((strcmp(args[2], "-W") == 0) || (strcmp(args[2], "-WORSTFIT") == 0)) {
        *policy = 3;  // Worst Fit
    } else {
        printf("Usage: ./mmu <input file> -{F | B | W}\n(F = FIFO | B = BEST FIT | W = WORST FIT)\n");
        exit(EXIT_FAILURE);
    }
}

// Allocate memory based on policy
void allocateMemory(list_t *freeList, list_t *allocList, int pid, int blockSize, int policy) {
    int index = list_get_index_of_by_Size(freeList, blockSize);
    if (index == -1) {
        printf("Error: Insufficient memory available\n");
        return;
    }

    block_t *chosenBlock = list_remove_at_index(freeList, index);

    int originalEnd = chosenBlock->end;
    chosenBlock->pid = pid;
    chosenBlock->end = chosenBlock->start + blockSize - 1;

    list_add_ascending_by_address(allocList, chosenBlock);

    int allocatedEnd = chosenBlock->end;
    if (allocatedEnd < originalEnd) {
        block_t *fragment = (block_t *)malloc(sizeof(block_t));
        fragment->pid = 0;
        fragment->start = allocatedEnd + 1;
        fragment->end = originalEnd;

        if (policy == 1) {
            list_add_to_back(freeList, fragment);
        } else if (policy == 2) {
            list_add_ascending_by_blocksize(freeList, fragment);
        } else if (policy == 3) {
            list_add_descending_by_blocksize(freeList, fragment);
        }
    }
}

// Deallocate memory and return it to the free list
void deallocateMemory(list_t *allocList, list_t *freeList, int pid, int policy) {
    int index = list_get_index_of_by_Pid(allocList, pid);
    if (index == -1) {
        printf("Error: No memory found for PID: %d\n", pid);
        return;
    }

    block_t *block = list_remove_at_index(allocList, index);
    block->pid = 0;

    if (policy == 1) {
        list_add_to_back(freeList, block);
    } else if (policy == 2) {
        list_add_ascending_by_blocksize(freeList, block);
    } else if (policy == 3) {
        list_add_descending_by_blocksize(freeList, block);
    }
}

// Coalesce adjacent free blocks into larger blocks
list_t *coalesceMemory(list_t *list) {
    list_t *tempList = list_alloc();
    block_t *block;

    while ((block = list_remove_from_front(list)) != NULL) {
        list_add_ascending_by_address(tempList, block);
    }

    list_coalesce_nodes(tempList);
    return tempList;
}

// Print details of memory blocks
void displayMemoryList(list_t *list, const char *message) {
    node_t *current = list->head;
    block_t *block;
    int i = 0;

    printf("%s:\n", message);

    while (current != NULL) {
        block = current->blk;
        printf("Block %d: START: %d END: %d", i, block->start, block->end);

        if (block->pid != 0) {
            printf(" PID: %d\n", block->pid);
        } else {
            printf("\n");
        }

        current = current->next;
        i++;
    }
}

// Main function
int main(int argc, char *argv[]) {
    int partitionSize, inputData[200][2], n = 0, memoryPolicy;

    list_t *freeList = list_alloc();
    list_t *allocList = list_alloc();

    if (argc != 3) {
        printf("Usage: ./mmu <input file> -{F | B | W}\n(F = FIFO | B = BEST FIT | W = WORST FIT)\n");
        exit(EXIT_FAILURE);
    }

    parseInput(argv, inputData, &n, &partitionSize, &memoryPolicy);

    block_t *partition = (block_t *)malloc(sizeof(block_t));
    partition->pid = 0;
    partition->start = 0;
    partition->end = partitionSize + partition->start - 1;

    list_add_to_front(freeList, partition);

    for (int i = 0; i < n; i++) {
        printf("************************\n");
        if (inputData[i][0] != -99999 && inputData[i][0] > 0) {
            printf("ALLOCATE: %d FROM PID: %d\n", inputData[i][1], inputData[i][0]);
            allocateMemory(freeList, allocList, inputData[i][0], inputData[i][1], memoryPolicy);
        } else if (inputData[i][0] != -99999 && inputData[i][0] < 0) {
            printf("DEALLOCATE MEM: PID %d\n", abs(inputData[i][0]));
            deallocateMemory(allocList, freeList, abs(inputData[i][0]), memoryPolicy);
        } else {
            printf("COALESCE/COMPACT\n");
            freeList = coalesceMemory(freeList);
        }

        printf("************************\n");
        displayMemoryList(freeList, "Free Memory");
        displayMemoryList(allocList, "Allocated Memory");
        printf("\n");
    }

    list_free(freeList);
    list_free(allocList);

    return 0;
}
