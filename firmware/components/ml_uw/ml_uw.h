/*
 * UTF-8 to Windows 125x Converter for Raspberry Pi Pico 2
 * 
 * 
 * Codepages :
 *   874  - Thai (เ ป ย ง อ้ น) 
 *  1250  - Central European (ąćęłńóśźż čžř őű)
 *  1251  - Cyrillic (Привет мир Україна)
 *  1252  - Western European (àéèùç äöüß ñ)
 *  1253  - Greek (αβγδε Ελληνικά)
 *  1254  - Turkish (Ğğ İı Şş)
 *  1255  - Hebrew (שלום עולם)
 *  1256  - Arabic (مرحبا بالعالم)
 *  1257  - Baltic (ĀāĒēĪī Ūū)
 *  1258  - Vietnamese (ăâêô ươ)
 */

#ifndef ML_UW_H
#define ML_UW_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* by default, only codepage 1252 is supported */

#define ML_UW_ALL 

#ifdef ML_UW_ALL
#define ML_UW_CP437
#define ML_UW_CP874
#define ML_UW_CP1250    
#define ML_UW_CP1251    
#define ML_UW_CP1253    
#define ML_UW_CP1254    
#define ML_UW_CP1255    
#define ML_UW_CP1256    
#define ML_UW_CP1257    
#define ML_UW_CP1258    
#endif

#define CP_IBM_PC_US        437
#define CP_THAI             874
#define CP_CENTRAL_EUROPEAN 1250
#define CP_CYRILLIC         1251
#define CP_WESTERN          1252
#define CP_GREEK            1253
#define CP_TURKISH          1254
#define CP_HEBREW           1255
#define CP_ARABIC           1256
#define CP_BALTIC           1257
#define CP_VIETNAMESE       1258

#define CODEPAGE CP_WESTERN

typedef struct
{
    const char *utf8_str;
    size_t      utf8_len;
    size_t      index;
} ml_uw_utf8_decoder_t;


/**
 * Configures the codepage to be used for conversions.
 * 
 * @param codepage Codepage identifier (874, 1250-1258)
 * 
 * Example:
 *   set_codepage(CP_CYRILLIC);  // For Russian/Ukrainian
 *   set_codepage(CP_ARABIC);    // For Arabic
 *   set_codepage(CP_THAI);      // For Thai
 */
void ml_uw_set_codepage(int codepage);

/**
 * Gets the currently configured codepage
 * 
 * @return Current codepage identifier
 */
int ml_uw_get_codepage(void);

/**
 * Converts a UTF-8 string to the configured Windows codepage
 * 
 * @param utf8_str Source string in UTF-8 (terminated by \0)
 * @param win_buffer Destination buffer 
 * @param buffer_size Size of the destination buffer
 * @return Number of bytes written, or -1 if an error occurred
 * 
 * Multilingual example:
 *   set_codepage(CP_GREEK);
 *   utf8_to_win(“Καλημέρα κόσμε”, buffer, sizeof(buffer));
 */
int ml_uw_utf8_to_win(const char* utf8_str, char* win_buffer, size_t buffer_size);

/**
 * Converts a string from the configured Windows codepage to UTF-8
* 
* @param win_str Source string in Windows codepage (terminated by \0)
* @param utf8_buffer UTF-8 destination buffer
* @param buffer_size Size of the destination buffer  
* @return Number of bytes written, or -1 if an error occurs
 * 
 * Example:
 *   set_codepage(CP_HEBREW);
 *   win_to_utf8(hebrew_text, utf8_buffer, sizeof(utf8_buffer));
 */
int ml_uw_win_to_utf8(const char* win_str, char* utf8_buffer, size_t buffer_size);


/**
 * Validates if a string is well-formed UTF-8
 * 
 * @param utf8_str Input string to validate
 * @return true if valid UTF-8, false otherwise
 */
bool ml_uw_utf8_is_valid(const char *utf8_str);

/**
 * Initializes a UTF-8 to ANSI decoder state
 * 
 * @param dec Decoder state structure to initialize
 * @param utf8_str Input UTF-8 string to decode
 * @return true on success, false on error (e.g. null pointer)
 */
bool ml_uw_start_utf8_to_ansi_decoder(ml_uw_utf8_decoder_t *dec, const char *utf8_str);

/**
 * Decodes the next UTF-8 character from the decoder state and returns its ANSI equivalent
 * 
 * @param dec Decoder state structure
 * @return Next ANSI character, or '?' if the UTF-8 character cannot be represented in ANSI
 * 
 * Example usage:
 *   ml_uw_utf8_decoder_t dec;
 *   ml_uw_start_utf8_to_ansi_decoder(&dec, utf8_string);
 *   char ansi_char;
 *   while ((ansi_char = ml_uw_utf8_decode_next_ansi_char(&dec)) != 0) {
 *       // Process ansi_char
 *   }
 */
char ml_uw_utf8_decode_next_ansi_char(ml_uw_utf8_decoder_t *dec);

#endif // ML_UW_H
