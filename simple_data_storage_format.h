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
       !important - if c file api is used to read file (fopen, fread, etc.), "rb" mode must be used because "r" mode can alter file size and stuff!
    2) provide SdsfAllocator for library to use
    3) preallocate SdsfDeserializedResult value (can be on stack)
    4) call sdsf_deserialize function with all required args
    5) check for SdsfError value

    To serialize file user must:
    1) provide SdsfAllocator for library to use
    2) call sdsf_serializer_begin
    3) call sdsf_serialize_* functions to store data to sdsf
    4) call sdsf_serializer_end
    5) use SdsfSerializedResult data as needed
    6) free SdsfSerializedResult using sdsf_serialized_result_free

    User can alter library behaviour using preprocessor commands:
        Integer defines:
            SDSF_VALUES_ARRAY_DEFAULT_CAPACITY                  - defines default size for SdsfValueArray
            SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY              - defines default size for SdsfValuePtrArray
            SDSF_STRING_ARRAY_DEFAULT_CAPACITY                  - defines default size for SdsfStringArray
            SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY             - defines size for serializer's staging buffer (used for converting integers and floats to string)
            SDSF_SERIALIZER_MAIN_BUFFER_DEFAULT_CAPACITY        - defines default size for serializer's main (aka result) buffer
            SDSF_SERIALIZER_STACK_DEFAULT_CAPACITY              - defines default size for serializer's _SdsfSerializerStackEntry stack
            SDSF_SERIALIZER_BINARY_DATA_BUFFER_DEFAULT_CAPACITY - defines default size for serializer's binary data buffer
        functional defines:
            sdsf_assert - can be redefined to use custom error-handling logic

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
    SDSF_ERROR_ALL_FINE = 0,
    SDSF_ERROR_SOMETHING_REALLY_BAD,
    SDSF_ERROR_TOKENIZER_FAILED,
    SDSF_ERROR_EXPECTED_IDENTIFIER,
    SDSF_ERROR_EXPECTED_VALUE,
    SDSF_ERROR_UNEXPECTED_RESERVED_SYMBOL,
    SDSF_ERROR_EXPECTED_STRING_LITERAL,
    SDSF_ERROR_INVALID_BINARY_LITERAL,
    SDSF_ERROR_UNEXPECTED_BINARY_DATA_BLOB,
} SdsfError;

const char* SDSF_ERROR_TO_STR[] =
{
    "SDSF_ERROR_ALL_FINE",
    "SDSF_ERROR_SOMETHING_REALLY_BAD",
    "SDSF_ERROR_TOKENIZER_FAILED",
    "SDSF_ERROR_EXPECTED_IDENTIFIER",
    "SDSF_ERROR_EXPECTED_VALUE",
    "SDSF_ERROR_UNEXPECTED_RESERVED_SYMBOL",
    "SDSF_ERROR_EXPECTED_STRING_LITERAL",
    "SDSF_ERROR_INVALID_BINARY_LITERAL",
    "SDSF_ERROR_UNEXPECTED_BINARY_DATA_BLOB",
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
    SdsfAllocator allocator;
    SdsfValuePtrArray topLevelValues;
    SdsfValueArray values;
    SdsfStringArray strings;
    void* binaryData;
    size_t binaryDataSize;
} SdsfDeserializedResult;

typedef enum 
{
    _SDSF_SERIALIZER_IN_NOTHING,
    _SDSF_SERIALIZER_IN_ARRAY,
    _SDSF_SERIALIZER_IN_COMPOSITE,
} _SdsfSerializerStackEntry;

typedef struct
{
    SdsfAllocator allocator;

    char* stagingBuffer;
    size_t stagingBufferSize;

    _SdsfSerializerStackEntry* stack;
    size_t stackSize;
    size_t stackCapacity;

    void* binaryDataBuffer;
    size_t binaryDataBufferSize;
    size_t binaryDataBufferCapacity;

    void* mainBuffer;
    size_t mainBufferSize;
    size_t mainBufferCapacity;
} SdsfSerializer;

typedef struct
{
    SdsfAllocator allocator;
    void* buffer;
    size_t bufferSize;
    size_t bufferCapacity;
} SdsfSerializedResult;

SdsfError sdsf_deserialize(SdsfDeserializedResult* result, const void* data, size_t dataSize, SdsfAllocator allocator);
void sdsf_deserialized_result_free(SdsfDeserializedResult* sdsf);

SdsfSerializer sdsf_serializer_begin(SdsfAllocator allocator);
void sdsf_serialize_bool(SdsfSerializer* sdsf, const char* name, bool value);
void sdsf_serialize_int(SdsfSerializer* sdsf, const char* name, int32_t value);
void sdsf_serialize_float(SdsfSerializer* sdsf, const char* name, float value);
void sdsf_serialize_string(SdsfSerializer* sdsf, const char* name, const char* value);
void sdsf_serialize_binary(SdsfSerializer* sdsf, const char* name, const void* value, size_t size);
void sdsf_serialize_array_start(SdsfSerializer* sdsf, const char* name);
void sdsf_serialize_array_end(SdsfSerializer* sdsf);
void sdsf_serialize_composite_start(SdsfSerializer* sdsf, const char* name);
void sdsf_serialize_composite_end(SdsfSerializer* sdsf);
SdsfSerializedResult sdsf_serializer_end(SdsfSerializer* sdsf);
void sdsf_serialized_result_free(SdsfSerializedResult* sdsf);

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

#ifndef sdsf_assert
#   include <assert.h>
#   define sdsf_assert(condition, msg) assert((condition) && (msg))
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

_SdsfTokenType _sdsf_match_string(const _SdsfComsumedString* str)
{
    sdsf_assert(str->ptr, "Unable to match string : string pointer is null");
    sdsf_assert(str->size, "Unable to match string : string size is zero");

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
        else
        {
            sdsf_assert(!_sdsf_is_skipped_char(firstChar), "Unable to match string : unexpected skipped character");
            sdsf_assert(!_sdsf_is_reserved_symbol(firstChar), "Unable to match string : unexpected reserved symbol");
            possibilitySpace = _POSSIBLE_IDENTIFIER;
        }

        for (size_t it = 1; it < str->size; it++)
        {
            const char c = str->ptr[it];
            if (_sdsf_is_number(c))
            {
                continue; // can be anything
            }

            sdsf_assert(!_sdsf_is_skipped_char(c), "Unable to match string : unexpected skipped character");
            sdsf_assert(!_sdsf_is_reserved_symbol(c), "Unable to match string : unexpected reserved symbol");
            switch(c)
            {
                case '.':
                {
                    if (dotFound) return _SDSF_TOKEN_TYPE_INVALID;
                    // only floats can have '.'
                    possibilitySpace &= _POSSIBLE_FLOAT_LITERAL;
                    dotFound = true;
                } break;
                case '-':
                {
                    if (dashFound) return _SDSF_TOKEN_TYPE_INVALID;
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

    return _SDSF_TOKEN_TYPE_INVALID;
}

bool _sdsf_consume_token(_SdsfConsumedToken* result, _SdsfTokenizerData* data)
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
        result->tokenType = _sdsf_match_string(&consumedString);
        if (result->tokenType == _SDSF_TOKEN_TYPE_RESERVED_SYMBOL && consumedString.ptr[0] == '\"')
        {
            sdsf_assert(data->stringLiteralState != _SDSF_STRING_LITERAL_BEGIN, "Unable to consume token : unexpected string literal state");
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

SdsfError sdsf_deserialize(SdsfDeserializedResult* result, const void* data, size_t dataSize, SdsfAllocator allocator)
{
    _SdsfTokenizerData tokenizerData;
    tokenizerData.data                  = (const char*)data;
    tokenizerData.dataSize              = dataSize;
    tokenizerData.stringLiteralState    = _SDSF_STRING_LITERAL_NONE;
    tokenizerData.stringConsumePtr      = 0;

    _SdsfConsumedToken token;
    _SdsfConsumedToken previousToken = {0};

    *result = (SdsfDeserializedResult){0};
    result->allocator = allocator;

    SdsfValuePtrArray* topLevelValues = &result->topLevelValues;
    SdsfValueArray* values = &result->values;
    SdsfStringArray* strings = &result->strings;
    SdsfValue* currentValue = NULL;

    bool expectsBinaryDataBlob = false;
    bool shouldRun = true;

    while (shouldRun && _sdsf_consume_token(&token, &tokenizerData))
    {
        if (token.tokenType == _SDSF_TOKEN_TYPE_INVALID)
        {
            return SDSF_ERROR_TOKENIZER_FAILED;
        }

        if (token.tokenType == _SDSF_TOKEN_TYPE_IDENTIFIER)
        {
            if (currentValue)
            {
                if (currentValue->type == SDSF_VALUE_UNDEFINED)
                {
                    // Two identifiers in a row
                    return SDSF_ERROR_EXPECTED_VALUE;
                }
                if (currentValue->type != SDSF_VALUE_COMPOSITE)
                {
                    // Only composite values can have named childs
                    return SDSF_ERROR_EXPECTED_VALUE;
                }
            }

            SdsfValue* const val = _sdsf_val_array_add(values, &allocator);
            val->parent = currentValue;
            val->name = _sdsf_string_array_save(strings, &allocator, token.stringPtr, token.stringSize);

            if (currentValue)
            {
                sdsf_assert(currentValue->type == SDSF_VALUE_COMPOSITE, "Unable to process identifier : only composite values can have named children");
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
                        // Commas can be used in arrays only
                        return SDSF_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    if (currentValue->asArray.childs.size == 0)
                    {
                        // Commas must be used only after first array child
                        return SDSF_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    const SdsfValue* const lastChild = currentValue->asArray.childs.ptr[currentValue->asArray.childs.size - 1];
                    if (lastChild->type == SDSF_VALUE_UNDEFINED)
                    {
                        // Previous array child must be initialized before using comma
                        return SDSF_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    // @NOTE : comma doesn't allocate new values, new value is allocated when processing literals or creating composites/arrays
                } break;

                case ']':
                {
                    if (!currentValue || currentValue->type != SDSF_VALUE_ARRAY)
                    {
                        // Only arrays can end with ']'
                        return SDSF_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    currentValue = currentValue->parent;
                } break;

                case '}':
                {
                    if (!currentValue || currentValue->type != SDSF_VALUE_COMPOSITE)
                    {
                        // Only composites can end with '}'
                        return SDSF_ERROR_UNEXPECTED_RESERVED_SYMBOL;
                    }
                    currentValue = currentValue->parent;
                } break;

                case '[':
                {
                    if (!currentValue)
                    {
                        // Expected identifier or another array
                        return SDSF_ERROR_EXPECTED_IDENTIFIER;
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
                        // New array can be created only after identifier (aka currentValue->type == SDSF_VALUE_UNDEFINED)
                        // or in the another array (aka currentValue->type == SDSF_VALUE_ARRAY)
                        return SDSF_ERROR_SOMETHING_REALLY_BAD;
                    }
                } break;

                case '{':
                {
                    if (!currentValue)
                    {
                        // Expected identifier or array
                        return SDSF_ERROR_EXPECTED_IDENTIFIER;
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
                        // New array can be created only after identifier (aka currentValue->type == SDSF_VALUE_UNDEFINED)
                        // or in the another array (aka currentValue->type == SDSF_VALUE_ARRAY)
                        return SDSF_ERROR_SOMETHING_REALLY_BAD;
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
                        // No binary literals were used, so binary data is not expected
                        return SDSF_ERROR_UNEXPECTED_BINARY_DATA_BLOB;
                    }

                    const size_t binaryDataSize = tokenizerData.dataSize - tokenizerData.stringConsumePtr;
                    if (binaryDataSize)
                    {
                        void* const memory = allocator.alloc(binaryDataSize, allocator.userData);
                        result->binaryData = memory;
                        result->binaryDataSize = binaryDataSize;
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
                // Values must be associated with identifier or array
                return SDSF_ERROR_EXPECTED_IDENTIFIER;
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
                    // Only arrays can have values without names
                    return SDSF_ERROR_EXPECTED_IDENTIFIER;
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
                        // "to" must be always equal or bigger than "from"
                        return SDSF_ERROR_INVALID_BINARY_LITERAL;
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
    
    return SDSF_ERROR_ALL_FINE;
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
    if (sdsf->stackSize == sdsf->stackCapacity)
    {
        const size_t newCapacity = sdsf->stackCapacity * 2;
        _SdsfSerializerStackEntry* const newMem = (_SdsfSerializerStackEntry*)sdsf->allocator.alloc(sizeof(_SdsfSerializerStackEntry) * newCapacity, sdsf->allocator.userData);
        memcpy(newMem, sdsf->stack, sizeof(_SdsfSerializerStackEntry) * sdsf->stackCapacity);
        sdsf->allocator.dealloc(sdsf->stack, sizeof(_SdsfSerializerStackEntry) * sdsf->stackCapacity, sdsf->allocator.userData);
        sdsf->stack = newMem;
        sdsf->stackCapacity = newCapacity;
    }
    sdsf_assert(sdsf->stackSize < sdsf->stackCapacity, "Unable to push value to stack : no free space");
    sdsf->stack[sdsf->stackSize++] = entry;
}

inline _SdsfSerializerStackEntry _sdsf_pop_from_stack(SdsfSerializer* sdsf)
{
    sdsf_assert(sdsf->stackSize, "Unable to pop value from stack : stack is free");
    return sdsf->stack[--sdsf->stackSize];
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
    sdsf_assert(data, "Unable to push value to main buffer : ptr is null");
    sdsf_assert(dataSize, "Unable to push value to main buffer : size is zero");

    _sdsf_ensure_buffer_capacity(&sdsf->allocator, &sdsf->mainBuffer, &sdsf->mainBufferCapacity, sdsf->mainBufferSize, dataSize);
    char* const buffer = ((char*)sdsf->mainBuffer) + sdsf->mainBufferSize;
    memcpy(buffer, data, dataSize);
    sdsf->mainBufferSize += dataSize;
}

inline void _sdsf_push_to_binary_buffer(SdsfSerializer* sdsf, const void* data, size_t dataSize)
{
    sdsf_assert(data, "Unable to push value to binary buffer : ptr is null");
    sdsf_assert(dataSize, "Unable to push value to binary buffer : size is zero");

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

inline void _sdsf_begin_value(SdsfSerializer* sdsf, const char* name)
{
    _sdsf_push_indent(sdsf);
    if (_sdsf_peek_stack(sdsf) != _SDSF_SERIALIZER_IN_ARRAY)
    {
        sdsf_assert(name, "Unable to being value serialization : name string is null (only arrays can have unnamed children)");
        const size_t nameLength = strlen(name);
        _sdsf_push_to_main_buffer(sdsf, name, nameLength);
        _sdsf_push_to_main_buffer(sdsf, " ", 1);
    }
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
    char* const stagingBuffer = (char*)allocator.alloc(SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY, allocator.userData);
    _SdsfSerializerStackEntry* const stack = (_SdsfSerializerStackEntry*)allocator.alloc(sizeof(_SdsfSerializerStackEntry) * SDSF_SERIALIZER_STACK_DEFAULT_CAPACITY, allocator.userData);
    void* const buffer = allocator.alloc(SDSF_SERIALIZER_MAIN_BUFFER_DEFAULT_CAPACITY, allocator.userData);
    void* const binaryDataBuffer = allocator.alloc(SDSF_SERIALIZER_BINARY_DATA_BUFFER_DEFAULT_CAPACITY, allocator.userData);

    SdsfSerializer result = {0};
    result.allocator                = allocator;
    result.stagingBuffer            = stagingBuffer;
    result.stagingBufferSize        = SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY;
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

void sdsf_serialize_bool(SdsfSerializer* sdsf, const char* name, bool value)
{
    _sdsf_begin_value(sdsf, name);
    _sdsf_push_to_main_buffer(sdsf, value ? "t" : "f", 1);
    _sdsf_end_value(sdsf);
}

void sdsf_serialize_int(SdsfSerializer* sdsf, const char* name, int32_t value)
{
    _sdsf_begin_value(sdsf, name);
    const int written = snprintf(sdsf->stagingBuffer, sdsf->stagingBufferSize, "%d", value);
    sdsf_assert(written > 0 && written < sdsf->stagingBufferSize, "Unable to serialize integer value : staging buffer is too small. Try increasing SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY");
    _sdsf_push_to_main_buffer(sdsf, sdsf->stagingBuffer, (size_t)written);
    _sdsf_end_value(sdsf);
}

void sdsf_serialize_float(SdsfSerializer* sdsf, const char* name, float value)
{
    _sdsf_begin_value(sdsf, name);
    const int written = snprintf(sdsf->stagingBuffer, sdsf->stagingBufferSize, "%f", value);
    sdsf_assert(written > 0 && written < sdsf->stagingBufferSize, "Unable to serialize float value : staging buffer is too small. Try increasing SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY");
    _sdsf_push_to_main_buffer(sdsf, sdsf->stagingBuffer, (size_t)written);
    _sdsf_end_value(sdsf);
}

void sdsf_serialize_string(SdsfSerializer* sdsf, const char* name, const char* value)
{
    _sdsf_begin_value(sdsf, name);
    sdsf_assert(value, "Unable to serialize string value : value is null");
    _sdsf_push_to_main_buffer(sdsf, "\"", 1);
    const size_t valueLength = strlen(value);
    _sdsf_push_to_main_buffer(sdsf, value, valueLength);
    _sdsf_push_to_main_buffer(sdsf, "\"", 1);
    _sdsf_end_value(sdsf);
}

void sdsf_serialize_binary(SdsfSerializer* sdsf, const char* name, const void* value, size_t size)
{
    _sdsf_begin_value(sdsf, name);

    const size_t from = sdsf->binaryDataBufferSize;
    const size_t to = from + size;
    if (value && size)
    {
        _sdsf_push_to_binary_buffer(sdsf, value, size);
    }

    _sdsf_push_to_main_buffer(sdsf, "b", 1);

    int written;
    written = snprintf(sdsf->stagingBuffer, sdsf->stagingBufferSize, "%zu", from);
    sdsf_assert(written > 0 && written < sdsf->stagingBufferSize, "Unable to serialize binary value : staging buffer is too small. Try increasing SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY");
    _sdsf_push_to_main_buffer(sdsf, sdsf->stagingBuffer, (size_t)written);

    _sdsf_push_to_main_buffer(sdsf, "-", 1);

    written = snprintf(sdsf->stagingBuffer, sdsf->stagingBufferSize, "%zu", to);
    sdsf_assert(written > 0 && written < sdsf->stagingBufferSize, "Unable to serialize binary value : staging buffer is too small. Try increasing SDSF_SERIALIZER_STAGING_BUFFER_CAPACITY");
    _sdsf_push_to_main_buffer(sdsf, sdsf->stagingBuffer, (size_t)written);

    _sdsf_end_value(sdsf);
}

void sdsf_serialize_array_start(SdsfSerializer* sdsf, const char* name)
{
    _sdsf_begin_value(sdsf, name);
    _sdsf_push_to_main_buffer(sdsf, "[\r\n", 3);
    _sdsf_push_to_stack(sdsf, _SDSF_SERIALIZER_IN_ARRAY);
}

void sdsf_serialize_array_end(SdsfSerializer* sdsf)
{
    const _SdsfSerializerStackEntry entry = _sdsf_pop_from_stack(sdsf);
    sdsf_assert(entry == _SDSF_SERIALIZER_IN_ARRAY, "Unable to end array serialization : serializer is currently not in array");
    _sdsf_push_indent(sdsf);
    _sdsf_push_to_main_buffer(sdsf, "]", 1);
    _sdsf_end_value(sdsf);
}

void sdsf_serialize_composite_start(SdsfSerializer* sdsf, const char* name)
{
    _sdsf_begin_value(sdsf, name);
    _sdsf_push_to_main_buffer(sdsf, "{\r\n", 3);
    _sdsf_push_to_stack(sdsf, _SDSF_SERIALIZER_IN_COMPOSITE);
}

void sdsf_serialize_composite_end(SdsfSerializer* sdsf)
{
    const _SdsfSerializerStackEntry entry = _sdsf_pop_from_stack(sdsf);
    sdsf_assert(entry == _SDSF_SERIALIZER_IN_COMPOSITE, "Unable to end array serialization : serializer is currently not in composite");
    _sdsf_push_indent(sdsf);
    _sdsf_push_to_main_buffer(sdsf, "}", 1);
    _sdsf_end_value(sdsf);
}

SdsfSerializedResult sdsf_serializer_end(SdsfSerializer* sdsf)
{
    sdsf_assert(sdsf->stackSize == 0, "Unable to end serialization : serializer stack is not empty. Probably you forgot some calls to sdsf_serialize_composite_end or sdsf_serialize_array_end");

    if (sdsf->binaryDataBuffer && sdsf->binaryDataBufferSize)
    {
        _sdsf_push_to_main_buffer(sdsf, "\r\n@", 3);
        _sdsf_push_to_main_buffer(sdsf, sdsf->binaryDataBuffer, sdsf->binaryDataBufferSize);
    }

    if (sdsf->stagingBuffer && sdsf->stagingBufferSize)
    {
        sdsf->allocator.dealloc(sdsf->stagingBuffer, sdsf->stagingBufferSize, sdsf->allocator.userData);
    }

    if (sdsf->binaryDataBuffer && sdsf->binaryDataBufferCapacity)
    {
        sdsf->allocator.dealloc(sdsf->binaryDataBuffer, sdsf->binaryDataBufferCapacity, sdsf->allocator.userData);
    }

    if (sdsf->stack && sdsf->stackCapacity)
    {
        sdsf->allocator.dealloc(sdsf->stack, sizeof(_SdsfSerializerStackEntry) * sdsf->stackCapacity, sdsf->allocator.userData);
    }
    
    const SdsfSerializedResult result = (SdsfSerializedResult) { sdsf->allocator, sdsf->mainBuffer, sdsf->mainBufferSize, sdsf->mainBufferCapacity };
    *sdsf = (SdsfSerializer) {0};
    return result; 
}

void sdsf_serialized_result_free(SdsfSerializedResult* sdsf)
{
    if (sdsf->buffer && sdsf->bufferCapacity)
    {
        sdsf->allocator.dealloc(sdsf->buffer, sdsf->bufferCapacity, sdsf->allocator.userData);
    }
}

#ifdef __cplusplus
}
#endif

#endif // _SDSF_IMPL_INNER_
#endif // SDSF_IMPL
