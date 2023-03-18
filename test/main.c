
#define SDSF_IMPL
#include "..\simple_data_storage_format.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct
{
    const void* data;
    size_t size;
} FileContent;

FileContent read_whole_file(const char* path)
{
    FILE* const file = fopen(path, "rb");
    fseek(file, 0, SEEK_END);
    const long fsize = ftell(file);
    fseek(file, 0, SEEK_SET);
    void* const result = malloc(fsize + 1);
    fread(result, fsize, 1, file);
    fclose(file);
    return (FileContent){ result, fsize };
}

void* alloc(size_t size, void* _)
{
    return malloc(size);
}

void dealloc(void* ptr, size_t size, void* _)
{
    free(ptr);
}

void print_value_rec(const SdsfValue* value, size_t depth)
{
    for (size_t it = 0; it < depth; it++) printf("   ");
    printf("%s %s ", SDSF_VALUE_TYPE_TO_STR[value->type], value->name ? value->name : "unnamed");

    switch (value->type)
    {
        case SDSF_VALUE_BOOL:   { printf("%s", value->asBool ? "true" : "false"); } break;
        case SDSF_VALUE_INT:    { printf("%d", value->asInt); } break;
        case SDSF_VALUE_FLOAT:  { printf("%f", value->asFloat); } break;
        case SDSF_VALUE_STRING: { printf("%s", value->asString); } break;
        case SDSF_VALUE_BINARY: { printf("From %zu, size %zu", value->asBinary.dataOffset, value->asBinary.dataSize); } break;
    }
    printf("\n");

    if (value->type == SDSF_VALUE_COMPOSITE)
    {
        for (size_t it = 0; it < value->asComposite.childs.size; it++)
        {
            print_value_rec(value->asComposite.childs.ptr[it], depth + 1);
        }
    }
    else if (value->type == SDSF_VALUE_ARRAY)
    {
        for (size_t it = 0; it < value->asArray.childs.size; it++)
        {
            print_value_rec(value->asArray.childs.ptr[it], depth + 1);
        }
    }
}

void print_stuff(const SdsfDeserialized* stuff)
{
    for (size_t it = 0; it < stuff->topLevelValues.size; it++)
    {
        const SdsfValue* const value = stuff->topLevelValues.ptr[it];
        print_value_rec(value, 0);
    }
}

void main()
{
    const FileContent file = read_whole_file("test\\document.sdsf");
    const SdsfAllocator allocator = { NULL, alloc, dealloc };
    SdsfDeserialized result = {0};
    const SdsfError error = sdsf_deserialize(&result, file.data, file.size, allocator);

    if (error)
    {
        printf("Deserealization error : %s\n", SDSF_ERROR_TO_STR[error]);
    }
    else
    {
        print_stuff(&result);
    }

    sdsf_free(&result);
}
