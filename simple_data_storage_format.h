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
            array [0, 1, 2, { text "composite value inside of an array" }]
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
    3) preallocate SdsfDeserialized value (can be on stack)
    4) call sdsf_deserialize function with all required args
    5) check for SdsfError value

    To serialize file user must:
     -- TODO ---

*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef sdsf_size_t
#   include <stdint.h>
#   define sdsf_size_t uint64_t
#   define sdsf_size_t_max UINT64_MAX
#endif

#ifndef sdsf_string_to_size_t
#   include <limits.h>
#   if sdsf_size_t_max == ULLONG_MAX
#       include <stdlib.h>
#       define sdsf_string_to_size_t(str) strtoull(str, NULL, 10)
#   else
#       error Define new sdsf_string_to_size_t (by default we use strtoull, but currently ULLONG_MAX != sdsf_size_t_max and default conversion might fail)
#   endif
#endif

#ifndef sdsf_int
#   include <stdint.h>
#   define sdsf_int int32_t
#   define sdsf_int_max INT32_MAX
#endif

#ifndef sdsf_string_to_int
#   include <limits.h>
#   if sdsf_int_max == INT_MAX
#       include <stdlib.h>
#       define sdsf_string_to_int(str) atoi(str)
#   else
#       error Define new sdsf_string_to_int (by default atoi, but currently INT_MAX != sdsf_int_max and default conversion might fail)
#   endif
#endif

#ifndef sdsf_bool
#   include <stdbool.h>
#   define sdsf_bool bool
#   define sdsf_true true
#   define sdsf_false false
#endif

#ifndef SDSF_VALUES_ARRAY_DEFAULT_CAPACITY
#   define SDSF_VALUES_ARRAY_DEFAULT_CAPACITY 1024
#endif

#ifndef SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY
#   define SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY 8
#endif

#ifndef SDSF_STRING_ARRAY_DEFAULT_CAPACITY
#   define SDSF_STRING_ARRAY_DEFAULT_CAPACITY 2048
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
    void* userData;
    void* (*alloc)(sdsf_size_t size, void* userData);
    void (*dealloc)(void* ptr, sdsf_size_t size, void* userData);
} SdsfAllocator;

typedef struct
{
    struct SdsfValue** ptr;
    sdsf_size_t size;
    sdsf_size_t capacity;
} SdsfValuePtrArray;

typedef struct
{
    struct SdsfValue* ptr;
    sdsf_size_t size;
    sdsf_size_t capacity;
} SdsfValueArray;

typedef struct
{
    char* ptr;
    sdsf_size_t size;
    sdsf_size_t capacity;
} SdsfStringArray;

typedef struct SdsfValue
{
    struct SdsfValue* parent;
    const char* name;
    SdsfValueType type;
    union
    {
        sdsf_bool asBool;
        sdsf_int asInt;
        float asFloat;
        const char* asString;
        struct
        {
            sdsf_size_t dataOffset;
            sdsf_size_t dataSize;
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
    sdsf_size_t binaryDataSize;
} SdsfDeserialized;

SdsfError sdsf_deserialize(SdsfDeserialized* result, const void* data, sdsf_size_t dataSize, SdsfAllocator allocator);
void sdsf_free(SdsfDeserialized* sdsf);

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
#   define sdsf_assert(condition) assert(condition)
#endif

#ifndef sdsf_memcpy
#   include <string.h>
#   define sdsf_memcpy(to, from, size) memcpy(to, from, size)
#endif

#ifndef sdsf_memset
#   include <string.h>
#   define sdsf_memset(dst, val, size) memset(dst, val, size)
#endif

#ifndef sdsf_atof
#   include <stdlib.h>
#   define sdsf_atof(str) atof(str)
#endif

typedef struct
{
    const char* ptr;
    sdsf_size_t size;
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
    sdsf_size_t dataSize;
    _SdsfStringLiteralState stringLiteralState;
    sdsf_size_t stringConsumePtr;
} _SdsfTokenizerData;

typedef struct
{
    const char* stringPtr;
    sdsf_size_t stringSize;
    _SdsfTokenType tokenType;
} _SdsfConsumedToken;

inline sdsf_bool _sdsf_is_skipped_char(char c)
{
    return (c == ' ') || (c == '\n') || (c == '\r') || (c == '\t');
}

inline sdsf_bool _sdsf_is_reserved_symbol(char c)
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

inline sdsf_bool _sdsf_is_number(char c)
{
    return c >= '0' && c <= '9';
}

sdsf_bool _sdsf_consume_string(const char* sourceBuffer, sdsf_size_t sourceBufferSize, sdsf_size_t* consumePtr, _SdsfComsumedString* result, sdsf_bool isStringLiteral)
{
    if (*consumePtr >= sourceBufferSize)
    {
        return sdsf_false;
    }

    sdsf_size_t beginning;
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
            return sdsf_true;
        }

        // Consume string until reserved or skip symbol
        beginning = *consumePtr;
        for (; *consumePtr < sourceBufferSize; *consumePtr += 1)
        {
            const char c = sourceBuffer[*consumePtr];
            const sdsf_bool isSkipped = _sdsf_is_skipped_char(c);
            const sdsf_bool isReserved = _sdsf_is_reserved_symbol(c);
            if (isSkipped || isReserved) break;
        }

        // Zero size means we finished
        const sdsf_size_t size = *consumePtr - beginning;
        if (!size)
        {
            return sdsf_false;
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
    return sdsf_true;
}

_SdsfTokenType _sdsf_match_string(const _SdsfComsumedString* str)
{
    sdsf_assert(str->ptr);
    sdsf_assert(str->size);

    if (str->size == 1)
    {
        const char c = *str->ptr;
        const sdsf_bool isReserved = _sdsf_is_reserved_symbol(c);
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
        sdsf_int possibilitySpace = 0;
        sdsf_bool dotFound = sdsf_false;
        sdsf_bool dashFound = sdsf_false;

        const char firstChar = *str->ptr;
        if (firstChar == 'b')
        {
            possibilitySpace = _POSSIBLE_BINARY_LITERAL | _POSSIBLE_IDENTIFIER;
        }
        else if (_sdsf_is_number(firstChar) || firstChar == '-')
        {
            possibilitySpace = _POSSIBLE_INT_LITERAL | _POSSIBLE_FLOAT_LITERAL;
            if (firstChar == '-') dashFound = sdsf_true;
        }
        else if (firstChar == '.' )
        {
            possibilitySpace = _POSSIBLE_FLOAT_LITERAL;
            dotFound = sdsf_true;
        }
        else
        {
            sdsf_assert(!_sdsf_is_skipped_char(firstChar));
            sdsf_assert(!_sdsf_is_reserved_symbol(firstChar));
            possibilitySpace = _POSSIBLE_IDENTIFIER;
        }

        for (sdsf_size_t it = 1; it < str->size; it++)
        {
            const char c = str->ptr[it];
            if (_sdsf_is_number(c))
            {
                continue; // can be anything
            }

            sdsf_assert(!_sdsf_is_skipped_char(c));
            sdsf_assert(!_sdsf_is_reserved_symbol(c));
            switch(c)
            {
                case '.':
                {
                    if (dotFound) return _SDSF_TOKEN_TYPE_INVALID;
                    // only floats can have '.'
                    possibilitySpace &= _POSSIBLE_FLOAT_LITERAL;
                    dotFound = sdsf_true;
                } break;
                case '-':
                {
                    if (dashFound) return _SDSF_TOKEN_TYPE_INVALID;
                    // everything, but identifier can have '-'
                    possibilitySpace &= ~_POSSIBLE_IDENTIFIER;
                    dashFound = sdsf_true;
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
            // It is possible, to have both _POSSIBLE_IDENTIFIER and _POSSIBLE_BINARY_LITERAL is possibility space,
            // but if we have _POSSIBLE_IDENTIFIER that means binary literal wasn't fully completed (it must have '-' symbol)
            return _SDSF_TOKEN_TYPE_IDENTIFIER;
        }
        if (possibilitySpace & _POSSIBLE_INT_LITERAL)
        {
            // It is possible, to have both _POSSIBLE_INT_LITERAL and _POSSIBLE_FLOAT_LITERAL is possibility space,
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

sdsf_bool _sdsf_consume_token(_SdsfConsumedToken* result, _SdsfTokenizerData* data)
{
    _SdsfComsumedString consumedString;
    if (!_sdsf_consume_string(data->data, data->dataSize, &data->stringConsumePtr, &consumedString, data->stringLiteralState == _SDSF_STRING_LITERAL_BEGIN))
    {
        return sdsf_false;
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
            sdsf_assert(data->stringLiteralState != _SDSF_STRING_LITERAL_BEGIN);
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

    return sdsf_true;
}

SdsfValue* _sdsf_val_array_add(SdsfValueArray* array, const SdsfAllocator* allocator)
{
    if (array->capacity == 0)
    {
        array->ptr = (SdsfValue*)allocator->alloc(SDSF_VALUES_ARRAY_DEFAULT_CAPACITY * sizeof(SdsfValue), allocator->userData);
        sdsf_memset(array->ptr, 0, SDSF_VALUES_ARRAY_DEFAULT_CAPACITY * sizeof(SdsfValue));
        array->size = 0;
        array->capacity = SDSF_VALUES_ARRAY_DEFAULT_CAPACITY;
    }
    else if (array->size == array->capacity)
    {
        const sdsf_size_t newCapacity = array->capacity * 2;
        const sdsf_size_t oldCapacity = array->capacity;

        SdsfValue* const newMem = (SdsfValue*)allocator->alloc(newCapacity * sizeof(SdsfValue), allocator->userData);
        sdsf_memcpy(newMem, array->ptr, oldCapacity * sizeof(SdsfValue));
        allocator->dealloc(array->ptr, oldCapacity * sizeof(SdsfValue), allocator->userData);
        void* const toZero = newMem + oldCapacity;
        sdsf_memset(toZero, 0, (newCapacity - oldCapacity) * sizeof(SdsfValue));
        
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
        sdsf_memset(array->ptr, 0, SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY * sizeof(SdsfValue*));
        array->size = 0;
        array->capacity = SDSF_VALUES_PTR_ARRAY_DEFAULT_CAPACITY;
    }
    else if (array->size == array->capacity)
    {
        const sdsf_size_t newCapacity = array->capacity * 2;
        const sdsf_size_t oldCapacity = array->capacity;

        SdsfValue** const newMem = (SdsfValue**)allocator->alloc(newCapacity * sizeof(SdsfValue*), allocator->userData);
        sdsf_memcpy(newMem, array->ptr, oldCapacity * sizeof(SdsfValue*));
        allocator->dealloc(array->ptr, oldCapacity * sizeof(SdsfValue*), allocator->userData);
        void* const toZero = newMem + oldCapacity;
        sdsf_memset(toZero, 0, (newCapacity - oldCapacity) * sizeof(SdsfValue*));
        
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

char* _sdsf_string_array_save(SdsfStringArray* array, const SdsfAllocator* allocator, const char* string, sdsf_size_t stringLength)
{
    if (array->capacity == 0)
    {
        const sdsf_size_t initialCapacity = (stringLength + 1) > SDSF_STRING_ARRAY_DEFAULT_CAPACITY ? (stringLength + 1) : SDSF_STRING_ARRAY_DEFAULT_CAPACITY;
        array->ptr = allocator->alloc(initialCapacity, allocator->userData);
        sdsf_memset(array->ptr, 0, initialCapacity);
        array->size = 0;
        array->capacity = initialCapacity;
    }
    else if ((array->size + stringLength + 1) >= array->capacity)
    {
        const sdsf_size_t requiredCapacity = array->size + stringLength + 1;
        const sdsf_size_t doubledCapacity = array->capacity * 2;
        const sdsf_size_t newCapacity = (requiredCapacity > doubledCapacity) ? requiredCapacity : doubledCapacity;
        const sdsf_size_t oldCapacity = array->capacity;

        char* const newMem = (char*)allocator->alloc(newCapacity, allocator->userData);
        sdsf_memcpy(newMem, array->ptr, oldCapacity);
        allocator->dealloc(array->ptr, oldCapacity, allocator->userData);
        void* const toZero = newMem + oldCapacity;
        sdsf_memset(toZero, 0, newCapacity - oldCapacity);
        
        array->ptr = newMem;
        array->capacity = newCapacity;
    }

    char* result = &array->ptr[array->size];
    if (string)
    {
        sdsf_memcpy(result, string, stringLength);
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

SdsfError sdsf_deserialize(SdsfDeserialized* result, const void* data, sdsf_size_t dataSize, SdsfAllocator allocator)
{
    _SdsfTokenizerData tokenizerData;
    tokenizerData.data                  = (const char*)data;
    tokenizerData.dataSize              = dataSize;
    tokenizerData.stringLiteralState    = _SDSF_STRING_LITERAL_NONE;
    tokenizerData.stringConsumePtr      = 0;

    _SdsfConsumedToken token;
    _SdsfConsumedToken previousToken = {0};

    *result = (SdsfDeserialized){0};
    result->allocator = allocator;

    SdsfValuePtrArray* topLevelValues = &result->topLevelValues;
    SdsfValueArray* values = &result->values;
    SdsfStringArray* strings = &result->strings;
    SdsfValue* currentValue = NULL;

    sdsf_bool expectsBinaryDataBlob = sdsf_false;
    sdsf_bool shouldRun = sdsf_true;

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
                sdsf_assert(currentValue->type == SDSF_VALUE_COMPOSITE);
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
                    if (previousToken.tokenType == _SDSF_TOKEN_TYPE_RESERVED_SYMBOL && previousToken.stringPtr[0] == ',')
                    {
                        // Every ',' must be followed by a value
                        return SDSF_ERROR_EXPECTED_VALUE;
                    }
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

                    const sdsf_size_t binaryDataSize = tokenizerData.dataSize - tokenizerData.stringConsumePtr;
                    if (binaryDataSize)
                    {
                        void* const memory = allocator.alloc(binaryDataSize, allocator.userData);
                        result->binaryData = memory;
                        result->binaryDataSize = binaryDataSize;
                        const void* const from = tokenizerData.data + tokenizerData.stringConsumePtr;
                        sdsf_memcpy(memory, from, binaryDataSize);
                    }

                    // Binary data blob is always in the end of file
                    shouldRun = sdsf_false;
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
                    valueToUpdate->asBool = token.stringPtr[0] == 't' ? sdsf_true : sdsf_false;
                    valueToUpdate->type = SDSF_VALUE_BOOL;
                } break;
                case _SDSF_TOKEN_TYPE_INT_LITERAL:
                {
                    valueToUpdate->asInt = sdsf_string_to_int(token.stringPtr);
                    valueToUpdate->type = SDSF_VALUE_INT;
                } break;
                case _SDSF_TOKEN_TYPE_FLOAT_LITERAL:
                {
                    valueToUpdate->asFloat = sdsf_atof(token.stringPtr);
                    valueToUpdate->type = SDSF_VALUE_FLOAT;
                } break;
                case _SDSF_TOKEN_TYPE_BINARY_LITERAL:
                {
                    //
                    // If tokenizer produces _SDSF_TOKEN_TYPE_BINARY_LITERAL token that means:
                    //  - string starts with 'b' character, so we can skip it
                    //  - string has only one dash '-' symbols
                    //  - string has two numbers divided by the dash
                    //  - string size is at leas 4 characters (minimum binary literal is b0-0)
                    //

                    const char* string = token.stringPtr;
                    const sdsf_size_t from = sdsf_string_to_size_t(&string[1]);
                    for (sdsf_size_t it = 1; it < token.stringSize; it++)
                    {
                        if (string[it] == '-')
                        {
                            string = &string[it + 1];
                        }
                    }
                    const sdsf_size_t to = sdsf_string_to_size_t(string);

                    if (to < from)
                    {
                        // "to" must be always equal or bigger than "from"
                        return SDSF_ERROR_INVALID_BINARY_LITERAL;
                    }

                    valueToUpdate->asBinary.dataOffset = from;
                    valueToUpdate->asBinary.dataSize = to - from;
                    valueToUpdate->type = SDSF_VALUE_BINARY;
                    expectsBinaryDataBlob = sdsf_true;
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

void sdsf_free(SdsfDeserialized* sdsf)
{
    for (sdsf_size_t it = 0; it < sdsf->values.size; it++)
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

#ifdef __cplusplus
}
#endif

#endif // _SDSF_IMPL_INNER_
#endif // SDSF_IMPL
