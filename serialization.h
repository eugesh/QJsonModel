#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <iostream>

/**
 * @brief szn namespace
 * Set of function for conversion of different types to array of bytes.
 */
namespace szn {

template<typename T>
void print(T byte)
{
     for (int i = 7; i >= 0; i--) {
        int b = byte >> i;
        if (b & 1)
            std::cout << "1";
        else
            std::cout << "0";
    }
}

/**
 * @brief floatToBytes Converts float or double to array of bytes.
 * @param bytes bytes array.
 * @param n float or double.
 */
template<typename T>
void floatToBytes(unsigned char *bytes, T n)
{
    memcpy(bytes, reinterpret_cast<T*>(&n), sizeof(T));
}

/**
 * @brief bytesToFloat Converts bytes array to float or double.
 * @param n float or double.
 * @param bytes Bytes array.
 */
template<typename T>
void bytesToFloat(T &n, const unsigned char *bytes)
{
    memcpy(reinterpret_cast<T*>(&n), bytes, sizeof(T));
}

/**
 * @brief intToBytes Converts integer to array of bytes.
 * @param bytes Bytes array.
 * @param n int16, int32 or int64.
 */
template<typename T>
void intToBytes(unsigned char *bytes, T n)
{
    for (size_t i = 0; i < sizeof(n); ++i) {
        bytes[i] = (n >> (sizeof(n) * 8 - (i + 1) * 8)) & 0xFF;
    }
}

/**
 * @brief bytesToInt Converts array of bytes to integer.
 * @param n int16, int32 or int64.
 * @param bytes Bytes array.
 */
template<typename T>
void bytesToInt(T &n, const unsigned char *bytes)
{
    n = 0;
    for (size_t i = 0; i < sizeof(n); ++i) {
        n += bytes[i] & 0xFF;
        if (i < (sizeof(n) - 1))
            n <<= 8;
    }
}

}

#endif // SERIALIZATION_H
