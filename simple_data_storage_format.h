#ifndef _SIMPLE_DATA_STORAGE_FORMAT_H_
#define _SIMPLE_DATA_STORAGE_FORMAT_H_

/*
    Simple data storage format (sdsf)

    This library provides functions for serializing and deserializing sdsf files




     === SIMPLE DATA STORAGE FORMAT DESCRIPTION ===

    Simple data storage format (sdsf) is a text-based format.
    Data is stored using prebuit types. Those types are:
    1) boolean
    2) integer
    3) floating point
    4) string
    5) binary
    6) array
    7) composite

     - Boolean values are represented using 't' or 'f' characters
     - Integer values are represented using decimals. Example : 0, -1, 1, 999999, -999999, etc.
     - Floating point values are represented using decimals. Example : 0.0, 0.1, -0.1, 999.999, -999.999, etc.
     - String values are represented using double quotation marks. Example : "string of text", etc.
     - Binary values are special kind of values, which are used to store binary data in file. Binary value points to a binary blob at the end of file.
       Binary values start with 'b' character and are followed by two integer values separated by '-' character. Example : b0-100, b99-1024, etc.
     - Arrays can store multiple member (child) values. Array members must have no name. Arrays start with '[' character, each member is separated with ','
       character. Arrays end with ']' character. Example : [0, t, "string", b0-123, [1, 2, 3]]
     - Composite values can also store multiple childs. But, unlike an arrays, composite childs must have names and must not be separated by ',' character.
       Example :
        {
            int 1
            float 0.1
            array [0, 1, 2, { text "composite value inside of an array" anotherValue 42 }]
        }
    
    All values, except array members, must be paired with indentifiers. Identifier is a string which:
     - can have any letter
     - can have numbers
     - can't begin with number
     - can't have ',' '[' ']' '{' '}' '"' '@' '.' '-' characters

    Values and identifiers are separated using:
     - skip-characters - ' ' '\n' '\t' '\r'
     - reserved characters - ',' '[' ']' '{' '}' '"' '@'

    Values and identifiers can be separated by *any* number of skip-characters

    Binary data blob is stored at the end of file. Binary data blob starts with '@' character
    Example of sdsf file with binary blob:
        some_value 10
        another_value 123
        binary_value b0-71
        @this is binary data blob and it can store anything, not only plain text

    Sdsf files are expected to use utf-8 encoding




     === LIBRARY DESCRIPTION ===

    To deserialize file user must:
        1) read file into a memory buffer
        2) provide SdsfAllocator for library to use
        3) preallocate SdsfDeserializedResult value (can be on stack)
        4) call sdsf_deserialize function with all required args
        5) check for SdsfDeserializationError value
        6) process result
        7) call sdsf_deserialized_result_free

        Important - if c file api is used to read file (fopen, fread, etc.), "rb" mode must be used because "r" mode can alter file size and stuff
        Important - even if deserialization fails sdsf_deserialized_result_free must be called

    To serialize file user must:
        1) provide SdsfAllocator for library to use
        2) call sdsf_serializer_begin
        3) call sdsf_serialize_* functions to store data to sdsf
        3.1) check for SdsfSerializationError if neccessary
        4) call sdsf_serializer_end
        5) use SdsfSerializedResult data as needed
        6) free SdsfSerializedResult using sdsf_serialized_result_free

        Important - if SdsfSerializationError occurs, user can continue serialization process. Serialization error invalidates only a single command
        For example, if following sequence of commands was executed:
            sdsf_serialize_string(&sdsf, "a", "first string");
            sdsf_serialize_string(&sdsf, NULL, "this command will fail because name is null");
            sdsf_serialize_string(&sdsf, "c", "third string");
        Resulting sdsf file will look like this:
            a "first string"
            c "third string"

    User can alter library behaviour using preprocessor definitions:
        SDSF_VALUES_ARRAY_DEFAULT_CAPACITY                  - defines default size for SdsfValueArray
        SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY              - defines default size for SdsfValuePtrArray
        SDSF_STRING_ARRAY_DEFAULT_CAPACITY                  - defines default size for SdsfStringArray
        SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY             - defines size for serializer's staging buffer (used for converting integers and floats to string)
        SDSF_SERIALIZER_MAIN_BUFFER_DEFAULT_CAPACITY        - defines default size for serializer's main (aka result) buffer
        SDSF_SERIALIZER_STACK_DEFAULT_CAPACITY              - defines default size for serializer's _SdsfSerializerStackEntry stack
        SDSF_SERIALIZER_BINARY_DATA_BUFFER_DEFAULT_CAPACITY - defines default size for serializer's binary data buffer

    Library does not check SdsfAllocator::alloc result. Valid pointer is always expected

    Both serialization and deserialization operations return error codes (SdsfDeserializationError / SdsfSerializationError)
    On success error code is 0, so user can do error check using if statement:
    SdsfDeserializationError err = sdsf_deserialize(&dr, data, dataSize, allocator);
    if (err)
    {
        // Handle error
    }

    If error occurs, SdsfDeserializedResult and SdsfSerializer will have error description in errorMsg member
    To access error message in unified manner sdsf_get_error_message macro can be used
*/

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#ifndef SDSF_VALUES_ARRAY_DEFAULT_CAPACITY
#   define SDSF_VALUES_ARRAY_DEFAULT_CAPACITY 1024
#endif

#ifndef SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY
#   define SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY 8
#endif

#ifndef SDSF_STRING_ARRAY_DEFAULT_CAPACITY
#   define SDSF_STRING_ARRAY_DEFAULT_CAPACITY 2048
#endif

#ifndef SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY
#   define SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY 128
#endif

#ifndef SDSF_SERIALIZER_STACK_DEFAULT_CAPACITY
#   define SDSF_SERIALIZER_STACK_DEFAULT_CAPACITY 32
#endif

#ifndef SDSF_SERIALIZER_BINARY_DATA_BUFFER_DEFAULT_CAPACITY
#   define SDSF_SERIALIZER_BINARY_DATA_BUFFER_DEFAULT_CAPACITY 128
#endif

#ifndef SDSF_SERIALIZER_MAIN_BUFFER_DEFAULT_CAPACITY
#   define SDSF_SERIALIZER_MAIN_BUFFER_DEFAULT_CAPACITY 2048
#endif

typedef enum
{
    SDSF_VALUE_UNDEFINED,
    SDSF_VALUE_BOOL,
    SDSF_VALUE_INT,
    SDSF_VALUE_FLOAT,
    SDSF_VALUE_STRING,
    SDSF_VALUE_BINARY,
    SDSF_VALUE_ARRAY,
    SDSF_VALUE_COMPOSITE,
} SdsfValueType;

const char* SDSF_VALUE_TYPE_TO_STR[] =
{
    "SDSF_VALUE_UNDEFINED",
    "SDSF_VALUE_BOOL",
    "SDSF_VALUE_INT",
    "SDSF_VALUE_FLOAT",
    "SDSF_VALUE_STRING",
    "SDSF_VALUE_BINARY",
    "SDSF_VALUE_ARRAY",
    "SDSF_VALUE_COMPOSITE",
};

typedef enum
{
    SDSF_DESERIALIZATION_ERROR_ALL_FINE = 0,
    SDSF_DESERIALIZATION_ERROR_TOKENIZER_FAILED,
    SDSF_DESERIALIZATION_ERROR_EXPECTED_IDENTIFIER,
    SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL,
    SDSF_DESERIALIZATION_ERROR_UNEXPECTED_BINARY_DATA_BLOB,
    SDSF_DESERIALIZATION_ERROR_UNEXPECTED_IDENTIFIER,
    SDSF_DESERIALIZATION_ERROR_INVALID_BINARY_LITERAL,
} SdsfDeserializationError;

const char* SDSF_DESERIALIZATION_ERROR_TO_STR[] =
{
    "SDSF_DESERIALIZATION_ERROR_ALL_FINE",
    "SDSF_DESERIALIZATION_ERROR_TOKENIZER_FAILED",
    "SDSF_DESERIALIZATION_ERROR_EXPECTED_IDENTIFIER",
    "SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL",
    "SDSF_DESERIALIZATION_ERROR_UNEXPECTED_BINARY_DATA_BLOB",
    "SDSF_DESERIALIZATION_ERROR_UNEXPECTED_IDENTIFIER",
    "SDSF_DESERIALIZATION_ERROR_INVALID_BINARY_LITERAL",
};

typedef struct
{
    void* (*alloc)(size_t size, void* userData);
    void (*dealloc)(void* ptr, size_t size, void* userData);
    void* userData;
} SdsfAllocator;

typedef struct
{
    struct SdsfValue* ptr;
    size_t size;
    size_t capacity;
} SdsfValueArray;

typedef struct
{
    struct SdsfValue** ptr;
    size_t size;
    size_t capacity;
} SdsfValuePtrArray;

typedef struct
{
    char* ptr;
    size_t size;
    size_t capacity;
} SdsfStringArray;

typedef struct SdsfValue
{
    struct SdsfValue* parent;
    const char* name;
    SdsfValueType type;
    union
    {
        bool asBool;
        int32_t asInt;
        float asFloat;
        const char* asString;
        struct
        {
            size_t dataOffset;
            size_t dataSize;
        } asBinary;
        struct
        {
            SdsfValuePtrArray childs;
        } asArray;
        struct
        {
            SdsfValuePtrArray childs;
        } asComposite;
    };
} SdsfValue;

typedef struct
{
    SdsfAllocator       allocator;
    SdsfValuePtrArray   topLevelValues;
    SdsfValueArray      values;
    SdsfStringArray     strings;
    void*               binaryData;
    size_t              binaryDataSize;
    const char*         errorMsg;
} SdsfDeserializedResult;

typedef enum
{
    SDSF_SERIALIZATION_ERROR_ALL_FINE = 0,
    SDSF_SERIALIZATION_ERROR_NO_NAME_PROVIDED,
    SDSF_SERIALIZATION_ERROR_INVALID_NAME,
    SDSF_SERIALIZATION_ERROR_NO_VALUE_PROVIDED,
    SDSF_SERIALIZATION_ERROR_UNABLE_TO_CONVERT_VALUE_TO_STRING,
    SDSF_SERIALIZATION_ERROR_UNABLE_TO_END_ARRAY,
    SDSF_SERIALIZATION_ERROR_UNABLE_TO_END_COMPOSITE,
    SDSF_SERIALIZATION_ERROR_UNFINISHED_ARRAY_OR_COMPOSITE_VALUES,
} SdsfSerializationError;

const char* SDSF_SERIALIZATION_ERROR_TO_STR[] =
{
    "SDSF_SERIALIZATION_ERROR_ALL_FINE",
    "SDSF_SERIALIZATION_ERROR_NO_NAME_PROVIDED",
    "SDSF_SERIALIZATION_ERROR_INVALID_NAME",
    "SDSF_SERIALIZATION_ERROR_NO_VALUE_PROVIDED",
    "SDSF_SERIALIZATION_ERROR_UNABLE_TO_CONVERT_VALUE_TO_STRING",
    "SDSF_SERIALIZATION_ERROR_UNABLE_TO_END_ARRAY",
    "SDSF_SERIALIZATION_ERROR_UNABLE_TO_END_COMPOSITE",
    "SDSF_SERIALIZATION_ERROR_UNFINISHED_ARRAY_OR_COMPOSITE_VALUES",
};

typedef enum 
{
    _SDSF_SERIALIZER_IN_NOTHING,
    _SDSF_SERIALIZER_IN_ARRAY,
    _SDSF_SERIALIZER_IN_COMPOSITE,
} _SdsfSerializerStackEntry;

typedef struct
{
    SdsfAllocator               allocator;
    char*                       stagingBuffer1;
    char*                       stagingBuffer2;
    _SdsfSerializerStackEntry*  stack;
    size_t                      stackSize;
    size_t                      stackCapacity;
    void*                       binaryDataBuffer;
    size_t                      binaryDataBufferSize;
    size_t                      binaryDataBufferCapacity;
    void*                       mainBuffer;
    size_t                      mainBufferSize;
    size_t                      mainBufferCapacity;
    const char*                 errorMsg;
} SdsfSerializer;

typedef struct
{
    SdsfAllocator   allocator;
    void*           buffer;
    size_t          bufferSize;
    size_t          bufferCapacity;
} SdsfSerializedResult;

SdsfDeserializationError sdsf_deserialize(SdsfDeserializedResult* result, const void* data, size_t dataSize, SdsfAllocator allocator);
void sdsf_deserialized_result_free(SdsfDeserializedResult* sdsf);

SdsfSerializer sdsf_serializer_begin(SdsfAllocator allocator);
SdsfSerializationError sdsf_serialize_bool(SdsfSerializer* sdsf, const char* name, bool value);
SdsfSerializationError sdsf_serialize_int(SdsfSerializer* sdsf, const char* name, int32_t value);
SdsfSerializationError sdsf_serialize_float(SdsfSerializer* sdsf, const char* name, float value);
SdsfSerializationError sdsf_serialize_string(SdsfSerializer* sdsf, const char* name, const char* value);
SdsfSerializationError sdsf_serialize_binary(SdsfSerializer* sdsf, const char* name, const void* value, size_t size);
SdsfSerializationError sdsf_serialize_array_start(SdsfSerializer* sdsf, const char* name);
SdsfSerializationError sdsf_serialize_array_end(SdsfSerializer* sdsf);
SdsfSerializationError sdsf_serialize_composite_start(SdsfSerializer* sdsf, const char* name);
SdsfSerializationError sdsf_serialize_composite_end(SdsfSerializer* sdsf);
SdsfSerializationError sdsf_serializer_end(SdsfSerializer* sdsf, SdsfSerializedResult* result);
void sdsf_serialized_result_free(SdsfSerializedResult* sdsf);

#define sdsf_get_error_message(obj) (obj).errorMsg

#ifdef __cplusplus
}
#endif

#endif //_SIMPLE_DATA_STORAGE_FORMAT_H_

#define SDSF_IMPL
#ifdef SDSF_IMPL
#ifndef _SDSF_IMPL_INNER_
#define _SDSF_IMPL_INNER_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include <stdlib.h>

#ifdef _SDSF_INDENT_SIZE
#   error User should not redefine _SDSF_INDENT_SIZE value
#endif
#define _SDSF_INDENT_SIZE 4

#ifdef _SDSF_INDENT
#   error User should not redefine _SDSF_INDENT value
#endif
#define _SDSF_INDENT "    "

// ==============================================================================================================
//
//
// Deserializer
//
//
// ==============================================================================================================

typedef struct
{
    const char* ptr;
    size_t size;
} _SdsfComsumedString;

typedef enum
{
    _SDSF_TOKEN_TYPE_INVALID,
    _SDSF_TOKEN_TYPE_IDENTIFIER,
    _SDSF_TOKEN_TYPE_RESERVED_SYMBOL,
    _SDSF_TOKEN_TYPE_BOOL_LITERAL,
    _SDSF_TOKEN_TYPE_INT_LITERAL,
    _SDSF_TOKEN_TYPE_FLOAT_LITERAL,
    _SDSF_TOKEN_TYPE_BINARY_LITERAL,
    _SDSF_TOKEN_TYPE_STRING_LITERAL,
} _SdsfTokenType;

typedef enum
{
    _SDSF_STRING_LITERAL_NONE,
    _SDSF_STRING_LITERAL_BEGIN,
    _SDSF_STRING_LITERAL_END,
} _SdsfStringLiteralState;

typedef struct
{
    const char* data;
    size_t dataSize;
    _SdsfStringLiteralState stringLiteralState;
    size_t stringConsumePtr;
} _SdsfTokenizerData;

typedef struct
{
    const char* stringPtr;
    size_t stringSize;
    _SdsfTokenType tokenType;
} _SdsfConsumedToken;

inline bool _sdsf_is_skipped_char(char c)
{
    return (c == ' ') || (c == '\n') || (c == '\r') || (c == '\t');
}

inline bool _sdsf_is_reserved_symbol(char c)
{
    return
        (c == ',') ||
        (c == '[') ||
        (c == ']') ||
        (c == '{') ||
        (c == '}') ||
        (c == '\"') ||
        (c == '@');
}

inline bool _sdsf_is_number(char c)
{
    return c >= '0' && c <= '9';
}

bool _sdsf_consume_string(const char* sourceBuffer, size_t sourceBufferSize, size_t* consumePtr, _SdsfComsumedString* result, bool isStringLiteral)
{
    if (*consumePtr >= sourceBufferSize)
    {
        return false;
    }

    size_t beginning;
    if (!isStringLiteral)
    {
        // Skip spaces and stuff
        for (; *consumePtr < sourceBufferSize; *consumePtr += 1)
        {
            if (!_sdsf_is_skipped_char(sourceBuffer[*consumePtr])) break;
        }

        // Check reserved symbol
        if (_sdsf_is_reserved_symbol(sourceBuffer[*consumePtr]))
        {
            result->ptr = sourceBuffer + *consumePtr;
            result->size = 1;
            *consumePtr += 1;
            return true;
        }

        // Consume string until reserved or skip symbol
        beginning = *consumePtr;
        for (; *consumePtr < sourceBufferSize; *consumePtr += 1)
        {
            const char c = sourceBuffer[*consumePtr];
            const bool isSkipped = _sdsf_is_skipped_char(c);
            const bool isReserved = _sdsf_is_reserved_symbol(c);
            if (isSkipped || isReserved) break;
        }

        // Zero size means we finished
        const size_t size = *consumePtr - beginning;
        if (!size)
        {
            return false;
        }
    }
    else
    {
        beginning = *consumePtr;
        for (; *consumePtr < sourceBufferSize; *consumePtr += 1)
        {
            const char c = sourceBuffer[*consumePtr];
            if (c == '\"') break;
        }
    }

    result->ptr = sourceBuffer + beginning;
    result->size = *consumePtr - beginning;
    return true;
}

_SdsfTokenType _sdsf_match_string(SdsfDeserializedResult* sdsf, const _SdsfComsumedString* str)
{
    if (!str->ptr || !str->size)
    {
        sdsf->errorMsg = "Tokenzer error - invalid string provided";
        return _SDSF_TOKEN_TYPE_INVALID;
    }
    
    if (str->size == 1)
    {
        const char c = *str->ptr;
        const bool isReserved = _sdsf_is_reserved_symbol(c);
        if (isReserved)
        {
            return _SDSF_TOKEN_TYPE_RESERVED_SYMBOL;
        }
        else
        {
            if (_sdsf_is_number(c)) return _SDSF_TOKEN_TYPE_INT_LITERAL;
            switch (c)
            {
                case 't':
                case 'f': return _SDSF_TOKEN_TYPE_BOOL_LITERAL;
                sdsf->errorMsg = "Tokenzer error - invalid bool literal";
                default: return _SDSF_TOKEN_TYPE_INVALID;
            }
        }
    }
    else
    {
        typedef enum
        {
            _POSSIBLE_IDENTIFIER     = 1 << 0,
            _POSSIBLE_INT_LITERAL    = 1 << 1,
            _POSSIBLE_FLOAT_LITERAL  = 1 << 2,
            _POSSIBLE_BINARY_LITERAL = 1 << 3,
        } _PossibilitySpaceValue;
        int32_t possibilitySpace = 0;
        bool dotFound = false;
        bool dashFound = false;

        const char firstChar = *str->ptr;
        if (firstChar == 'b')
        {
            possibilitySpace = _POSSIBLE_BINARY_LITERAL | _POSSIBLE_IDENTIFIER;
        }
        else if (_sdsf_is_number(firstChar) || firstChar == '-')
        {
            possibilitySpace = _POSSIBLE_INT_LITERAL | _POSSIBLE_FLOAT_LITERAL;
            if (firstChar == '-') dashFound = true;
        }
        else if (firstChar == '.' )
        {
            possibilitySpace = _POSSIBLE_FLOAT_LITERAL;
            dotFound = true;
        }
        else if (_sdsf_is_skipped_char(firstChar) || _sdsf_is_reserved_symbol(firstChar))
        {
            sdsf->errorMsg = "Tokenzer error - unexpected character";
            return _SDSF_TOKEN_TYPE_INVALID;
        }
        else
        {
            possibilitySpace = _POSSIBLE_IDENTIFIER;
        }

        for (size_t it = 1; it < str->size; it++)
        {
            const char c = str->ptr[it];
            if (_sdsf_is_number(c))
            {
                continue; // can be anything
            }

            if (_sdsf_is_skipped_char(c) || _sdsf_is_reserved_symbol(c))
            {
                sdsf->errorMsg = "Tokenzer error - unexpected character";
                return _SDSF_TOKEN_TYPE_INVALID;
            }

            switch(c)
            {
                case '.':
                {
                    if (dotFound)
                    {
                        sdsf->errorMsg = "Tokenzer error - two '.' characters in single literal";
                        return _SDSF_TOKEN_TYPE_INVALID;
                    }
                    // only floats can have '.'
                    possibilitySpace &= _POSSIBLE_FLOAT_LITERAL;
                    dotFound = true;
                } break;
                case '-':
                {
                    if (dashFound)
                    {
                        sdsf->errorMsg = "Tokenzer error - two '-' characters in single literal";
                        return _SDSF_TOKEN_TYPE_INVALID;
                    }
                    // everything, but identifier can have '-'
                    possibilitySpace &= ~_POSSIBLE_IDENTIFIER;
                    dashFound = true;
                } break;
                default:
                {
                    // only identifier can have something other than number, dot or dash (binary literal 'b' prefix is checked earlier)
                    possibilitySpace &= _POSSIBLE_IDENTIFIER;
                } break;
            }
        }

        if (possibilitySpace & _POSSIBLE_IDENTIFIER)
        {
            // It is possible to have both _POSSIBLE_IDENTIFIER and _POSSIBLE_BINARY_LITERAL in possibility space,
            // but if we have _POSSIBLE_IDENTIFIER that means binary literal wasn't fully completed (it must have '-' symbol)
            return _SDSF_TOKEN_TYPE_IDENTIFIER;
        }
        if (possibilitySpace & _POSSIBLE_INT_LITERAL)
        {
            // It is possible to have both _POSSIBLE_INT_LITERAL and _POSSIBLE_FLOAT_LITERAL in possibility space,
            // but if we have _POSSIBLE_INT_LITERAL that means float literal wasn't fully completed (it must have '.' symbol)
            return _SDSF_TOKEN_TYPE_INT_LITERAL;
        }
        if (possibilitySpace & _POSSIBLE_FLOAT_LITERAL)
        {
            return _SDSF_TOKEN_TYPE_FLOAT_LITERAL;
        }
        if (possibilitySpace & _POSSIBLE_BINARY_LITERAL)
        {
            return _SDSF_TOKEN_TYPE_BINARY_LITERAL;
        }
    }

    sdsf->errorMsg = "Tokenzer error - failed to match token";
    return _SDSF_TOKEN_TYPE_INVALID;
}

bool _sdsf_consume_token(SdsfDeserializedResult* sdsf, _SdsfConsumedToken* result, _SdsfTokenizerData* data)
{
    _SdsfComsumedString consumedString;
    if (!_sdsf_consume_string(data->data, data->dataSize, &data->stringConsumePtr, &consumedString, data->stringLiteralState == _SDSF_STRING_LITERAL_BEGIN))
    {
        return false;
    }

    result->stringPtr = consumedString.ptr;
    result->stringSize = consumedString.size;

    if (data->stringLiteralState == _SDSF_STRING_LITERAL_BEGIN)
    {
        data->stringLiteralState = _SDSF_STRING_LITERAL_END;
        result->tokenType = _SDSF_TOKEN_TYPE_STRING_LITERAL;
    }
    else
    {
        result->tokenType = _sdsf_match_string(sdsf, &consumedString);
        if (result->tokenType == _SDSF_TOKEN_TYPE_RESERVED_SYMBOL && consumedString.ptr[0] == '\"')
        {
            if (data->stringLiteralState == _SDSF_STRING_LITERAL_NONE)
            {
                data->stringLiteralState = _SDSF_STRING_LITERAL_BEGIN;
            }
            else // (data->stringLiteralState == _SDSF_STRING_LITERAL_END)
            {
                data->stringLiteralState = _SDSF_STRING_LITERAL_NONE;
            }
        }
    }

    return true;
}

SdsfValue* _sdsf_val_array_add(SdsfValueArray* array, const SdsfAllocator* allocator)
{
    if (array->capacity == 0)
    {
        array->ptr = (SdsfValue*)allocator->alloc(SDSF_VALUES_ARRAY_DEFAULT_CAPACITY * sizeof(SdsfValue), allocator->userData);
        memset(array->ptr, 0, SDSF_VALUES_ARRAY_DEFAULT_CAPACITY * sizeof(SdsfValue));
        array->size = 0;
        array->capacity = SDSF_VALUES_ARRAY_DEFAULT_CAPACITY;
    }
    else if (array->size == array->capacity)
    {
        const size_t newCapacity = array->capacity * 2;
        const size_t oldCapacity = array->capacity;

        SdsfValue* const newMem = (SdsfValue*)allocator->alloc(newCapacity * sizeof(SdsfValue), allocator->userData);
        memcpy(newMem, array->ptr, oldCapacity * sizeof(SdsfValue));
        allocator->dealloc(array->ptr, oldCapacity * sizeof(SdsfValue), allocator->userData);
        void* const toZero = newMem + oldCapacity;
        memset(toZero, 0, (newCapacity - oldCapacity) * sizeof(SdsfValue));
        
        array->ptr = newMem;
        array->capacity = newCapacity;
    }

    return &array->ptr[array->size++];
}

void _sdsf_val_array_clear(SdsfValueArray* array, const SdsfAllocator* allocator)
{
    if (array->capacity)
    {
        allocator->dealloc(array->ptr, array->capacity * sizeof(SdsfValue), allocator->userData);
    }
}

SdsfValue** _sdsf_val_ptr_array_add(SdsfValuePtrArray* array, const SdsfAllocator* allocator)
{
    if (array->capacity == 0)
    {
        array->ptr = (SdsfValue**)allocator->alloc(SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY * sizeof(SdsfValue*), allocator->userData);
        memset(array->ptr, 0, SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY * sizeof(SdsfValue*));
        array->size = 0;
        array->capacity = SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY;
    }
    else if (array->size == array->capacity)
    {
        const size_t newCapacity = array->capacity * 2;
        const size_t oldCapacity = array->capacity;

        SdsfValue** const newMem = (SdsfValue**)allocator->alloc(newCapacity * sizeof(SdsfValue*), allocator->userData);
        memcpy(newMem, array->ptr, oldCapacity * sizeof(SdsfValue*));
        allocator->dealloc(array->ptr, oldCapacity * sizeof(SdsfValue*), allocator->userData);
        void* const toZero = newMem + oldCapacity;
        memset(toZero, 0, (newCapacity - oldCapacity) * sizeof(SdsfValue*));
        
        array->ptr = newMem;
        array->capacity = newCapacity;
    }

    return &array->ptr[array->size++];
}

void _sdsf_val_ptr_array_clear(SdsfValuePtrArray* array, const SdsfAllocator* allocator)
{
    if (array->capacity)
    {
        allocator->dealloc(array->ptr, array->capacity * sizeof(SdsfValue*), allocator->userData);
    }
}

char* _sdsf_string_array_save(SdsfStringArray* array, const SdsfAllocator* allocator, const char* string, size_t stringLength)
{
    if (array->capacity == 0)
    {
        const size_t initialCapacity = (stringLength + 1) > SDSF_STRING_ARRAY_DEFAULT_CAPACITY ? (stringLength + 1) : SDSF_STRING_ARRAY_DEFAULT_CAPACITY;
        array->ptr = allocator->alloc(initialCapacity, allocator->userData);
        memset(array->ptr, 0, initialCapacity);
        array->size = 0;
        array->capacity = initialCapacity;
    }
    else if ((array->size + stringLength + 1) >= array->capacity)
    {
        const size_t requiredCapacity = array->size + stringLength + 1;
        const size_t doubledCapacity = array->capacity * 2;
        const size_t newCapacity = (requiredCapacity > doubledCapacity) ? requiredCapacity : doubledCapacity;
        const size_t oldCapacity = array->capacity;

        char* const newMem = (char*)allocator->alloc(newCapacity, allocator->userData);
        memcpy(newMem, array->ptr, oldCapacity);
        allocator->dealloc(array->ptr, oldCapacity, allocator->userData);
        void* const toZero = newMem + oldCapacity;
        memset(toZero, 0, newCapacity - oldCapacity);
        
        array->ptr = newMem;
        array->capacity = newCapacity;
    }

    char* result = &array->ptr[array->size];
    if (string)
    {
        memcpy(result, string, stringLength);
    }
    array->size += stringLength + 1;

    return result;
}

void _sdsf_string_array_clear(SdsfStringArray* array, const SdsfAllocator* allocator)
{
    if (array->capacity)
    {
        allocator->dealloc(array->ptr, array->capacity, allocator->userData);
    }
}

SdsfDeserializationError sdsf_deserialize(SdsfDeserializedResult* sdsf, const void* data, size_t dataSize, SdsfAllocator allocator)
{
    _SdsfTokenizerData tokenizerData;
    tokenizerData.data                  = (const char*)data;
    tokenizerData.dataSize              = dataSize;
    tokenizerData.stringLiteralState    = _SDSF_STRING_LITERAL_NONE;
    tokenizerData.stringConsumePtr      = 0;

    _SdsfConsumedToken token;
    _SdsfConsumedToken previousToken = {0};

    *sdsf = (SdsfDeserializedResult){0};
    sdsf->allocator = allocator;

    SdsfValuePtrArray* topLevelValues = &sdsf->topLevelValues;
    SdsfValueArray* values = &sdsf->values;
    SdsfStringArray* strings = &sdsf->strings;
    SdsfValue* currentValue = NULL;

    bool expectsBinaryDataBlob = false;
    bool shouldRun = true;

    while (shouldRun && _sdsf_consume_token(sdsf, &token, &tokenizerData))
    {
        if (token.tokenType == _SDSF_TOKEN_TYPE_INVALID)
        {
            // Error message is set in _sdsf_match_string
            return SDSF_DESERIALIZATION_ERROR_TOKENIZER_FAILED;
        }

        if (token.tokenType == _SDSF_TOKEN_TYPE_IDENTIFIER)
        {
            if (currentValue)
            {
                if (currentValue->type == SDSF_VALUE_UNDEFINED)
                {
                    sdsf->errorMsg = "Unexpected identifier - got two identifiers in a row";
                    return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_IDENTIFIER;
                }
                if (currentValue->type != SDSF_VALUE_COMPOSITE)
                {
                    sdsf->errorMsg = "Unexpected identifier - only composite values can have named childs";
                    return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_IDENTIFIER;
                }
            }

            SdsfValue* const val = _sdsf_val_array_add(values, &allocator);
            val->parent = currentValue;
            val->name = _sdsf_string_array_save(strings, &allocator, token.stringPtr, token.stringSize);

            if (currentValue)
            {
                SdsfValue** const ptr = _sdsf_val_ptr_array_add(&currentValue->asComposite.childs, &allocator);
                *ptr = val;
            }
            else
            {
                SdsfValue** const ptr = _sdsf_val_ptr_array_add(topLevelValues, &allocator);
                *ptr = val;
            }
            
            currentValue = val;
        }
        else if (token.tokenType == _SDSF_TOKEN_TYPE_RESERVED_SYMBOL)
        {
            switch (token.stringPtr[0])
            {
                case ',':
                {
                    if (!currentValue || currentValue->type != SDSF_VALUE_ARRAY)
                    {
                        sdsf->errorMsg = "Unexpected ',' character - commas can be used in arrays only";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    if (currentValue->asArray.childs.size == 0)
                    {
                        sdsf->errorMsg = "Unexpected ',' character - commas must be used only after first array child";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    if (previousToken.tokenType == _SDSF_TOKEN_TYPE_RESERVED_SYMBOL && previousToken.stringPtr[0] == ',')
                    {
                        sdsf->errorMsg = "Unexpected ',' character - can't have multiple commas in a row";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    // @NOTE : comma doesn't allocate new values, new value is allocated when processing literals or creating composites/arrays
                } break;

                case ']':
                {
                    if (!currentValue || currentValue->type != SDSF_VALUE_ARRAY)
                    {
                        sdsf->errorMsg = "Unexpected ']' character - only arrays can end with this symbol";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    currentValue = currentValue->parent;
                } break;

                case '}':
                {
                    if (!currentValue || currentValue->type != SDSF_VALUE_COMPOSITE)
                    {
                        sdsf->errorMsg = "Unexpected '}' character - only composites can end with this symbol";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    currentValue = currentValue->parent;
                } break;

                case '[':
                {
                    if (!currentValue)
                    {
                        sdsf->errorMsg = "Unexpected '[' character - new array value can be created only after identifier or as child of another array";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    if (currentValue->type == SDSF_VALUE_UNDEFINED)
                    {
                        // Array with an identifier
                        currentValue->type = SDSF_VALUE_ARRAY;
                    }
                    else if (currentValue->type == SDSF_VALUE_ARRAY)
                    {
                        // Array in array
                        SdsfValue* const newChild = _sdsf_val_array_add(values, &allocator);
                        SdsfValue** const newChildPtr = _sdsf_val_ptr_array_add(&currentValue->asArray.childs, &allocator);
                        *newChildPtr = newChild;
                        newChild->parent = currentValue;
                        currentValue = newChild;
                        currentValue->type = SDSF_VALUE_ARRAY;
                    }
                    else
                    {
                        sdsf->errorMsg = "New array value can be created only after identifier or in the another array";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                } break;

                case '{':
                {
                    if (!currentValue)
                    {
                        sdsf->errorMsg = "Unexpected '[' character - new composite value can be created only after identifier or as child of the array";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    if (currentValue->type == SDSF_VALUE_UNDEFINED)
                    {
                        // Composite with an identifier
                        currentValue->type = SDSF_VALUE_COMPOSITE;
                    }
                    else if (currentValue->type == SDSF_VALUE_ARRAY)
                    {
                        // Composite in an array
                        SdsfValue* const newChild = _sdsf_val_array_add(values, &allocator);
                        SdsfValue** const newChildPtr = _sdsf_val_ptr_array_add(&currentValue->asArray.childs, &allocator);
                        *newChildPtr = newChild;
                        newChild->parent = currentValue;
                        currentValue = newChild;
                        currentValue->type = SDSF_VALUE_COMPOSITE;
                    }
                    else
                    {
                        sdsf->errorMsg = "Unexpected '[' character - new composite value can be created only after identifier or as child of the array";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                } break;

                case '\"':
                {
                    // Nothing
                } break;

                case '@':
                {
                    if (!expectsBinaryDataBlob)
                    {
                        sdsf->errorMsg = "Unexpected binary data blob - no binary literals were used";
                        return SDSF_DESERIALIZATION_ERROR_UNEXPECTED_BINARY_DATA_BLOB;
                    }

                    const size_t binaryDataSize = tokenizerData.dataSize - tokenizerData.stringConsumePtr;
                    if (binaryDataSize)
                    {
                        void* const memory = allocator.alloc(binaryDataSize, allocator.userData);
                        sdsf->binaryData = memory;
                        sdsf->binaryDataSize = binaryDataSize;
                        const void* const from = tokenizerData.data + tokenizerData.stringConsumePtr;
                        memcpy(memory, from, binaryDataSize);
                    }

                    // Binary data blob is always in the end of file
                    shouldRun = false;
                } break;
            }
        }
        else
        {
            if (!currentValue)
            {
                sdsf->errorMsg = "Values must be associated with identifier or array";
                return SDSF_DESERIALIZATION_ERROR_EXPECTED_IDENTIFIER;
            }

            SdsfValue* valueToUpdate = NULL;
            if (currentValue->type == SDSF_VALUE_ARRAY)
            {
                valueToUpdate = _sdsf_val_array_add(values, &allocator);
                valueToUpdate->parent = currentValue;
                SdsfValue** const ptr = _sdsf_val_ptr_array_add(&currentValue->asArray.childs, &allocator);
                *ptr = valueToUpdate;
            }
            else
            {
                if (!currentValue->name)
                {
                    sdsf->errorMsg = "Unexpected unnamed value. Only arrays can have values without names";
                    return SDSF_DESERIALIZATION_ERROR_EXPECTED_IDENTIFIER;
                }
                valueToUpdate = currentValue;
                currentValue = currentValue->parent;
            }

            switch (token.tokenType)
            {
                case _SDSF_TOKEN_TYPE_BOOL_LITERAL:
                {
                    valueToUpdate->asBool = token.stringPtr[0] == 't' ? true : false;
                    valueToUpdate->type = SDSF_VALUE_BOOL;
                } break;
                case _SDSF_TOKEN_TYPE_INT_LITERAL:
                {
                    valueToUpdate->asInt = atoi(token.stringPtr);
                    valueToUpdate->type = SDSF_VALUE_INT;
                } break;
                case _SDSF_TOKEN_TYPE_FLOAT_LITERAL:
                {
                    valueToUpdate->asFloat = (float)atof(token.stringPtr);
                    valueToUpdate->type = SDSF_VALUE_FLOAT;
                } break;
                case _SDSF_TOKEN_TYPE_BINARY_LITERAL:
                {
                    //
                    // If tokenizer produces _SDSF_TOKEN_TYPE_BINARY_LITERAL token that means:
                    //  - string starts with 'b' character, so we can skip it
                    //  - string has only one dash '-' symbol
                    //  - string has two numbers divided by the dash
                    //  - string size is at leas 4 characters (minimum binary literal is b0-0)
                    //
                    const char* string = token.stringPtr;
                    const size_t from = strtoull(&string[1], NULL, 10);
                    for (size_t it = 1; it < token.stringSize; it++)
                    {
                        if (string[it] == '-')
                        {
                            string = &string[it + 1];
                        }
                    }
                    const size_t to = strtoull(string, NULL, 10);

                    if (to < from)
                    {
                        sdsf->errorMsg = "Invalid binary literal : \"to\" must be always equal or bigger than \"from\"";
                        return SDSF_DESERIALIZATION_ERROR_INVALID_BINARY_LITERAL;
                    }

                    valueToUpdate->asBinary.dataOffset = from;
                    valueToUpdate->asBinary.dataSize = to - from;
                    valueToUpdate->type = SDSF_VALUE_BINARY;
                    expectsBinaryDataBlob = true;
                } break;
                case _SDSF_TOKEN_TYPE_STRING_LITERAL:
                {
                    valueToUpdate->asString = _sdsf_string_array_save(strings, &allocator, token.stringPtr, token.stringSize);
                    valueToUpdate->type = SDSF_VALUE_STRING;
                } break;
            }
        }

        previousToken = token;
    }
    
    return SDSF_DESERIALIZATION_ERROR_ALL_FINE;
}

void sdsf_deserialized_result_free(SdsfDeserializedResult* sdsf)
{
    for (size_t it = 0; it < sdsf->values.size; it++)
    {
        SdsfValue* const value = &sdsf->values.ptr[it];
        if (value->type == SDSF_VALUE_COMPOSITE)
        {
            _sdsf_val_ptr_array_clear(&value->asComposite.childs, &sdsf->allocator);
        }
        else if (value->type == SDSF_VALUE_ARRAY)
        {
            _sdsf_val_ptr_array_clear(&value->asArray.childs, &sdsf->allocator);
        }
    }
    _sdsf_val_array_clear(&sdsf->values, &sdsf->allocator);
    _sdsf_string_array_clear(&sdsf->strings, &sdsf->allocator);

    if (sdsf->binaryData && sdsf->binaryDataSize)
    {
        sdsf->allocator.dealloc(sdsf->binaryData, sdsf->binaryDataSize, sdsf->allocator.userData);
    }
}

// ==============================================================================================================
//
//
// Serializer
//
//
// ==============================================================================================================

void _sdsf_ensure_buffer_capacity(SdsfAllocator* allocator, void** buffer, size_t* capacity, size_t size, size_t additionalSize)
{
    const size_t initialCapacity = *capacity;
    void* const initialBuffer = *buffer;
    if ((initialCapacity - size) >= additionalSize)
    {
        return;
    }

    const size_t requiredCapacity = size + additionalSize;
    const size_t doubledCapacity = initialCapacity * 2;
    const size_t newCapacity = (requiredCapacity > doubledCapacity) ? requiredCapacity : doubledCapacity;

    void* const newBuffer = allocator->alloc(newCapacity, allocator->userData);
    memcpy(newBuffer, initialBuffer, size);

    allocator->dealloc(initialBuffer, initialCapacity, allocator->userData);
    *buffer = newBuffer;
    *capacity = newCapacity;
}

void _sdsf_push_to_stack(SdsfSerializer* sdsf, _SdsfSerializerStackEntry entry)
{
    if (sdsf->stackSize >= sdsf->stackCapacity)
    {
        const size_t newCapacity = sdsf->stackCapacity * 2;
        _SdsfSerializerStackEntry* const newMem = (_SdsfSerializerStackEntry*)sdsf->allocator.alloc(sizeof(_SdsfSerializerStackEntry) * newCapacity, sdsf->allocator.userData);
        memcpy(newMem, sdsf->stack, sizeof(_SdsfSerializerStackEntry) * sdsf->stackCapacity);
        sdsf->allocator.dealloc(sdsf->stack, sizeof(_SdsfSerializerStackEntry) * sdsf->stackCapacity, sdsf->allocator.userData);
        sdsf->stack = newMem;
        sdsf->stackCapacity = newCapacity;
    }
    sdsf->stack[sdsf->stackSize++] = entry;
}

inline _SdsfSerializerStackEntry _sdsf_pop_from_stack(SdsfSerializer* sdsf)
{
    return sdsf->stackSize ? sdsf->stack[--sdsf->stackSize] : _SDSF_SERIALIZER_IN_NOTHING;
}

inline _SdsfSerializerStackEntry _sdsf_peek_stack(SdsfSerializer* sdsf)
{
    if (sdsf->stackSize)
    {
        return sdsf->stack[sdsf->stackSize - 1];
    }
    else
    {
        return _SDSF_SERIALIZER_IN_NOTHING;
    }
}

inline void _sdsf_push_to_main_buffer(SdsfSerializer* sdsf, const void* data, size_t dataSize)
{
    _sdsf_ensure_buffer_capacity(&sdsf->allocator, &sdsf->mainBuffer, &sdsf->mainBufferCapacity, sdsf->mainBufferSize, dataSize);
    char* const buffer = ((char*)sdsf->mainBuffer) + sdsf->mainBufferSize;
    memcpy(buffer, data, dataSize);
    sdsf->mainBufferSize += dataSize;
}

inline void _sdsf_push_to_binary_buffer(SdsfSerializer* sdsf, const void* data, size_t dataSize)
{
    _sdsf_ensure_buffer_capacity(&sdsf->allocator, &sdsf->binaryDataBuffer, &sdsf->binaryDataBufferCapacity, sdsf->binaryDataBufferSize, dataSize);
    void* const buffer = ((char*)sdsf->binaryDataBuffer) + sdsf->binaryDataBufferSize;
    memcpy(buffer, data, dataSize);
    sdsf->binaryDataBufferSize += dataSize;
}

inline void _sdsf_push_indent(SdsfSerializer* sdsf)
{
    for (size_t it = 0; it < sdsf->stackSize; it++)
    {
        _sdsf_push_to_main_buffer(sdsf, _SDSF_INDENT, _SDSF_INDENT_SIZE);
    }
}

inline SdsfSerializationError _sdsf_begin_value(SdsfSerializer* sdsf, const char* name)
{
    const bool isInArray = _sdsf_peek_stack(sdsf) == _SDSF_SERIALIZER_IN_ARRAY;
    if (!isInArray && !name)
    {
        sdsf->errorMsg = "Only arrays can have unnamed children";
        return SDSF_SERIALIZATION_ERROR_NO_NAME_PROVIDED;
    }

    if (_sdsf_is_number(name[0]))
    {
        sdsf->errorMsg = "Indentifiers can't start with number";
        return SDSF_SERIALIZATION_ERROR_INVALID_NAME;
    }

    const size_t nameLength = strlen(name);
    for (size_t it = 0; it < nameLength; it++)
    {
        const char c = name[it];
        if (_sdsf_is_skipped_char(c) || _sdsf_is_reserved_symbol(c))
        {
            sdsf->errorMsg = "Identifiers can't have special or skip-characters";
            return SDSF_SERIALIZATION_ERROR_INVALID_NAME;
        }
        if (c == '.' || c == '-')
        {
            sdsf->errorMsg = "Identifiers can't have '.' and ',' characters";
            return SDSF_SERIALIZATION_ERROR_INVALID_NAME;
        }
        if (c == '\0')
        {
            sdsf->errorMsg = "Identifiers can't have '\\0' characters";
            return SDSF_SERIALIZATION_ERROR_INVALID_NAME;
        }
    }

    _sdsf_push_indent(sdsf);
    if (!isInArray)
    {
        _sdsf_push_to_main_buffer(sdsf, name, nameLength);
        _sdsf_push_to_main_buffer(sdsf, " ", 1);
    }

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

inline void _sdsf_end_value(SdsfSerializer* sdsf)
{
    if (_sdsf_peek_stack(sdsf) == _SDSF_SERIALIZER_IN_ARRAY)
    {
        _sdsf_push_to_main_buffer(sdsf, ",\r\n", 3);
    }
    else
    {
        _sdsf_push_to_main_buffer(sdsf, "\r\n", 2);
    }
}

SdsfSerializer sdsf_serializer_begin(SdsfAllocator allocator)
{
    char* const stagingBuffer1 = (char*)allocator.alloc(SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY, allocator.userData);
    char* const stagingBuffer2 = (char*)allocator.alloc(SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY, allocator.userData);
    _SdsfSerializerStackEntry* const stack = (_SdsfSerializerStackEntry*)allocator.alloc(sizeof(_SdsfSerializerStackEntry) * SDSF_SERIALIZER_STACK_DEFAULT_CAPACITY, allocator.userData);
    void* const buffer = allocator.alloc(SDSF_SERIALIZER_MAIN_BUFFER_DEFAULT_CAPACITY, allocator.userData);
    void* const binaryDataBuffer = allocator.alloc(SDSF_SERIALIZER_BINARY_DATA_BUFFER_DEFAULT_CAPACITY, allocator.userData);

    SdsfSerializer result = {0};
    result.allocator                = allocator;
    result.stagingBuffer1           = stagingBuffer1;
    result.stagingBuffer2           = stagingBuffer2;
    result.stack                    = stack;
    result.stackSize                = 0;
    result.stackCapacity            = SDSF_SERIALIZER_STACK_DEFAULT_CAPACITY;
    result.binaryDataBuffer         = binaryDataBuffer;
    result.binaryDataBufferSize     = 0;
    result.binaryDataBufferCapacity = SDSF_SERIALIZER_BINARY_DATA_BUFFER_DEFAULT_CAPACITY;
    result.mainBuffer               = buffer;
    result.mainBufferSize           = 0;
    result.mainBufferCapacity       = SDSF_SERIALIZER_MAIN_BUFFER_DEFAULT_CAPACITY;
    return result;
}

SdsfSerializationError sdsf_serialize_bool(SdsfSerializer* sdsf, const char* name, bool value)
{
    const SdsfSerializationError beginValueError = _sdsf_begin_value(sdsf, name);
    if (beginValueError)
    {
        return beginValueError;
    }
    
    _sdsf_push_to_main_buffer(sdsf, value ? "t" : "f", 1);
    _sdsf_end_value(sdsf);

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

SdsfSerializationError sdsf_serialize_int(SdsfSerializer* sdsf, const char* name, int32_t value)
{
    const int written = snprintf(sdsf->stagingBuffer1, SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY, "%d", value);
    if (written < 0 || written >= SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY)
    {
        sdsf->errorMsg = "Unable to convert integer to string. Consider increasing SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY";
        return SDSF_SERIALIZATION_ERROR_UNABLE_TO_CONVERT_VALUE_TO_STRING;
    }

    const SdsfSerializationError beginValueError = _sdsf_begin_value(sdsf, name);
    if (beginValueError)
    {
        return beginValueError;
    }

    _sdsf_push_to_main_buffer(sdsf, sdsf->stagingBuffer1, (size_t)written);
    _sdsf_end_value(sdsf);

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

SdsfSerializationError sdsf_serialize_float(SdsfSerializer* sdsf, const char* name, float value)
{
    const int written = snprintf(sdsf->stagingBuffer1, SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY, "%f", value);
    if (written < 0 || written >= SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY)
    {
        sdsf->errorMsg = "Unable to convert float to string. Consider increasing SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY";
        return SDSF_SERIALIZATION_ERROR_UNABLE_TO_CONVERT_VALUE_TO_STRING;
    }

    const SdsfSerializationError error = _sdsf_begin_value(sdsf, name);
    if (error)
    {
        return error;
    }

    _sdsf_push_to_main_buffer(sdsf, sdsf->stagingBuffer1, (size_t)written);
    _sdsf_end_value(sdsf);

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

SdsfSerializationError sdsf_serialize_string(SdsfSerializer* sdsf, const char* name, const char* value)
{
    if (!value)
    {
        sdsf->errorMsg = "Unable to serialize string - value is null";
        return SDSF_SERIALIZATION_ERROR_NO_VALUE_PROVIDED;
    }

    const SdsfSerializationError beginValueError = _sdsf_begin_value(sdsf, name);
    if (beginValueError)
    {
        return beginValueError;
    }

    _sdsf_push_to_main_buffer(sdsf, "\"", 1);
    const size_t valueLength = strlen(value);
    _sdsf_push_to_main_buffer(sdsf, value, valueLength);
    _sdsf_push_to_main_buffer(sdsf, "\"", 1);
    _sdsf_end_value(sdsf);

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

SdsfSerializationError sdsf_serialize_binary(SdsfSerializer* sdsf, const char* name, const void* value, size_t size)
{
    const size_t from = sdsf->binaryDataBufferSize;
    const size_t to = from + size;

    const int written1 = snprintf(sdsf->stagingBuffer1, SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY, "%zu", from);
    if (written1 < 0 || written1 >= SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY)
    {
        sdsf->errorMsg = "Unable to convert first binary offset to string. Consider increasing SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY";
        return SDSF_SERIALIZATION_ERROR_UNABLE_TO_CONVERT_VALUE_TO_STRING;
    }

    const int written2 = snprintf(sdsf->stagingBuffer2, SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY, "%zu", to);
    if (written2 < 0 || written2 >= SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY)
    {
        sdsf->errorMsg = "Unable to convert second binary offset to string. Consider increasing SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY";
        return SDSF_SERIALIZATION_ERROR_UNABLE_TO_CONVERT_VALUE_TO_STRING;
    }

    const SdsfSerializationError beginValueError = _sdsf_begin_value(sdsf, name);
    if (beginValueError)
    {
        return beginValueError;
    }

    if (value && size)
    {
        _sdsf_push_to_binary_buffer(sdsf, value, size);
    }

    _sdsf_push_to_main_buffer(sdsf, "b", 1);
    _sdsf_push_to_main_buffer(sdsf, sdsf->stagingBuffer1, (size_t)written1);
    _sdsf_push_to_main_buffer(sdsf, "-", 1);
    _sdsf_push_to_main_buffer(sdsf, sdsf->stagingBuffer2, (size_t)written2);
    _sdsf_end_value(sdsf);

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

SdsfSerializationError sdsf_serialize_array_start(SdsfSerializer* sdsf, const char* name)
{
    const SdsfSerializationError beginValueError = _sdsf_begin_value(sdsf, name);
    if (beginValueError)
    {
        return beginValueError;
    }

    _sdsf_push_to_main_buffer(sdsf, "[\r\n", 3);
    _sdsf_push_to_stack(sdsf, _SDSF_SERIALIZER_IN_ARRAY);

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

SdsfSerializationError sdsf_serialize_array_end(SdsfSerializer* sdsf)
{
    const _SdsfSerializerStackEntry entry = _sdsf_pop_from_stack(sdsf);
    if (entry != _SDSF_SERIALIZER_IN_ARRAY)
    {
        sdsf->errorMsg = "Trying to end array, but currently serializer is in another state (in composite, or not in anyrhing)";
        return SDSF_SERIALIZATION_ERROR_UNABLE_TO_END_ARRAY;
    }

    _sdsf_push_indent(sdsf);
    _sdsf_push_to_main_buffer(sdsf, "]", 1);
    _sdsf_end_value(sdsf);

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

SdsfSerializationError sdsf_serialize_composite_start(SdsfSerializer* sdsf, const char* name)
{
    const SdsfSerializationError beginValueError = _sdsf_begin_value(sdsf, name);
    if (beginValueError)
    {
        return beginValueError;
    }

    _sdsf_push_to_main_buffer(sdsf, "{\r\n", 3);
    _sdsf_push_to_stack(sdsf, _SDSF_SERIALIZER_IN_COMPOSITE);

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

SdsfSerializationError sdsf_serialize_composite_end(SdsfSerializer* sdsf)
{
    const _SdsfSerializerStackEntry entry = _sdsf_pop_from_stack(sdsf);
    if (entry != _SDSF_SERIALIZER_IN_COMPOSITE)
    {
        sdsf->errorMsg = "Trying to end composite, but currently serializer is in another state (in array, or not in anyrhing)";
        return SDSF_SERIALIZATION_ERROR_UNABLE_TO_END_COMPOSITE;
    }

    _sdsf_push_indent(sdsf);
    _sdsf_push_to_main_buffer(sdsf, "}", 1);
    _sdsf_end_value(sdsf);

    return SDSF_SERIALIZATION_ERROR_ALL_FINE;
}

SdsfSerializationError sdsf_serializer_end(SdsfSerializer* sdsf, SdsfSerializedResult* result)
{
    SdsfSerializationError error = SDSF_SERIALIZATION_ERROR_ALL_FINE;
    if (sdsf->stackSize != 0)
    {
        sdsf->errorMsg = "Not all composites/arrays were finished";
        error = SDSF_SERIALIZATION_ERROR_UNFINISHED_ARRAY_OR_COMPOSITE_VALUES;
    }

    if (!error && sdsf->binaryDataBuffer && sdsf->binaryDataBufferSize)
    {
        _sdsf_push_to_main_buffer(sdsf, "\r\n@", 3);
        _sdsf_push_to_main_buffer(sdsf, sdsf->binaryDataBuffer, sdsf->binaryDataBufferSize);
    }

    if (sdsf->stagingBuffer1)
    {
        sdsf->allocator.dealloc(sdsf->stagingBuffer1, SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY, sdsf->allocator.userData);
    }

    if (sdsf->stagingBuffer2)
    {
        sdsf->allocator.dealloc(sdsf->stagingBuffer2, SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY, sdsf->allocator.userData);
    }

    if (sdsf->binaryDataBuffer && sdsf->binaryDataBufferCapacity)
    {
        sdsf->allocator.dealloc(sdsf->binaryDataBuffer, sdsf->binaryDataBufferCapacity, sdsf->allocator.userData);
    }

    if (sdsf->stack && sdsf->stackCapacity)
    {
        sdsf->allocator.dealloc(sdsf->stack, sizeof(_SdsfSerializerStackEntry) * sdsf->stackCapacity, sdsf->allocator.userData);
    }
    
    if (!error)
    {
        *result = (SdsfSerializedResult) { sdsf->allocator, sdsf->mainBuffer, sdsf->mainBufferSize, sdsf->mainBufferCapacity };
    }
    else
    {
        *result = (SdsfSerializedResult) {0};
    }
    *sdsf = (SdsfSerializer) {0};

    return error; 
}

void sdsf_serialized_result_free(SdsfSerializedResult* sdsf)
{
    if (sdsf->buffer && sdsf->bufferCapacity)
    {
        sdsf->allocator.dealloc(sdsf->buffer, sdsf->bufferCapacity, sdsf->allocator.userData);
    }
    *sdsf = (SdsfSerializedResult) {0};
}

#ifdef __cplusplus
}
#endif

#endif // _SDSF_IMPL_INNER_
#endif // SDSF_IMPL
