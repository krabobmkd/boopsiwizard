/*
 * convert.c
 * Purpose: Convert binary file to C unsigned char array
 *
 * Author: krb
 * Copyright (C) 2025
 * Licensed under GPL v2
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    FILE *input_file;
    FILE *output_file;
    unsigned char buffer[16];
    size_t bytes_read;
    size_t total_bytes = 0;
    char output_filename[512];
    char array_name[256];
    char *basename;
    char *dot;
    int i;

    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    /* Open input file */
    input_file = fopen(argv[1], "rb");
    if (!input_file)
    {
        fprintf(stderr, "Error: Cannot open input file '%s'\n", argv[1]);
        return 1;
    }

    /* Generate output filename: input.bin -> input_bin.c */
    basename = strrchr(argv[1], '/');
    if (!basename)
        basename = strrchr(argv[1], '\\');
    if (basename)
        basename++;
    else
        basename = argv[1];

    strncpy(array_name, basename, sizeof(array_name) - 1);
    array_name[sizeof(array_name) - 1] = '\0';

    /* Replace dots and special chars with underscores */
    for (i = 0; array_name[i]; i++)
    {
        if (array_name[i] == '.' || array_name[i] == '-' || array_name[i] == ' ')
            array_name[i] = '_';
    }

    snprintf(output_filename, sizeof(output_filename), "%s.c", array_name);

    /* Open output file */
    output_file = fopen(output_filename, "w");
    if (!output_file)
    {
        fprintf(stderr, "Error: Cannot create output file '%s'\n", output_filename);
        fclose(input_file);
        return 1;
    }

    /* Write C file header */
    fprintf(output_file, "/*\n");
    fprintf(output_file, " * %s\n", output_filename);
    fprintf(output_file, " * Generated from: %s\n", argv[1]);
    fprintf(output_file, " */\n\n");
    fprintf(output_file, "unsigned char %s[] = {\n", array_name);

    /* Read and write data */
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), input_file)) > 0)
    {
        fprintf(output_file, "    ");
        for (i = 0; i < bytes_read; i++)
        {
            fprintf(output_file, "0x%02x", buffer[i]);
            if (total_bytes + i + 1 < total_bytes + bytes_read || !feof(input_file))
                fprintf(output_file, ",");
            if (i < bytes_read - 1)
                fprintf(output_file, " ");
        }
        fprintf(output_file, "\n");
        total_bytes += bytes_read;
    }

    /* Write closing */
    fprintf(output_file, "};\n\n");
    fprintf(output_file, "unsigned int %s_size = %lu;\n", array_name, (unsigned long)total_bytes);

    /* Close files */
    fclose(input_file);
    fclose(output_file);

    printf("Converted %lu bytes from '%s' to '%s'\n",
           (unsigned long)total_bytes, argv[1], output_filename);
    printf("Array name: %s[%lu]\n", array_name, (unsigned long)total_bytes);

    return 0;
}

/*
 * Binary data packed into C arrays! 📦
 *      ___
 *     /   \
 *    | 0 1 |  <- This robot converts bytes to code!
 *    |  _  |
 *     \___/
 *      | |
 */
