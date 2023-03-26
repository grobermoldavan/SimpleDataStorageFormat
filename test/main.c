
#define SDSF_IMPL
#include "..\simple_data_storage_format.h"

#include <stdio.h>

typedef struct
{
    const void* data;
    size_t size;
} FileContent;

FileContent read_whole_file(const char* path)
{
    //
    // @NOTE : if you use fopen for file read make shure
    // to use "rb" mode. "r" mode can alter file size or
    // content leading to an incorrect parsing and deserialization
    //
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

void deserialize_and_print(const void* data, size_t dataSize, SdsfAllocator allocator)
{
    SdsfDeserializedResult dr = {0};
    const SdsfDeserializationError error = sdsf_deserialize(&dr, data, dataSize, allocator);

    if (error)
    {
        printf("Deserialization error : %s. Description : %s\n", SDSF_DESERIALIZATION_ERROR_TO_STR[error], dr.errorMsg);
    }
    else
    {
        for (size_t it = 0; it < dr.topLevelValues.size; it++)
        {
            const SdsfValue* const value = dr.topLevelValues.ptr[it];
            print_value_rec(value, 0);
        }
    }

    sdsf_deserialized_result_free(&dr);
}

void serialize_bunch_of_stuff(SdsfSerializer* sdsf)
{
    sdsf_serialize_bool(sdsf, "boolValue", true);
    sdsf_serialize_int(sdsf, "intValue", 228);
    sdsf_serialize_float(sdsf, "floatValue", 2.001f);
    sdsf_serialize_string(sdsf, "stringValue", "String string string!");
}

void main()
{
    const SdsfAllocator allocator = { alloc, dealloc, NULL };

    printf(" ===================================================================\n");
    printf(" TEST DESERIALIZATION FROM FILE\n");
    printf(" ===================================================================\n\n");

    const FileContent file = read_whole_file("test\\document.sdsf");
    deserialize_and_print(file.data, file.size, allocator);

    printf("\n ===================================================================\n");
    printf(" TEST SERIALIZATION\n");
    printf(" ===================================================================\n\n");

    SdsfSerializer sdsf = sdsf_serializer_begin(allocator);

    const char* binaryData = "This is stored in binary section";

    //
    // @NOTE : here we don't check SdsfSerializationError's because we know that all commands will succeed
    // But in a real application it's recommended to check SdsfSerializationError value
    //
    serialize_bunch_of_stuff(&sdsf);
    sdsf_serialize_array_start(&sdsf, "valuesInArray");
        serialize_bunch_of_stuff(&sdsf);
    sdsf_serialize_array_end(&sdsf);
    sdsf_serialize_composite_start(&sdsf, "valuesInComposite");
        serialize_bunch_of_stuff(&sdsf);
        sdsf_serialize_array_start(&sdsf, "valuesInArray");
            serialize_bunch_of_stuff(&sdsf);
            sdsf_serialize_binary(&sdsf, "binaryValue", binaryData, strlen(binaryData));
        sdsf_serialize_array_end(&sdsf);
        //
        // @NOTE : + 1 here is only for demonstration purpouses, so we can print
        // whole file after serialization
        //
        sdsf_serialize_binary(&sdsf, "binaryValue", binaryData, strlen(binaryData) + 1);
    sdsf_serialize_composite_end(&sdsf);

    SdsfSerializedResult sr = {0};
    SdsfSerializationError error = sdsf_serializer_end(&sdsf, &sr);

    //
    // @NOTE : In real application it's incorrect to print whole sdsf file because
    // binary section can (and most probably will) have non-text data
    //
    printf("Serialized file :\n%s\n\n", (const char*)sr.buffer);

    printf(" ===================================================================\n");
    printf(" TEST DESERIALIZATION FROM SERIALIZED DATA\n");
    printf(" ===================================================================\n\n");

    deserialize_and_print(sr.buffer, sr.bufferSize, allocator);

    sdsf_serialized_result_free(&sr);
}
