#include "http_res.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *render_template(const char *template, placeholder_t *placeholders, size_t num_placeholders)
{
    char *result = NULL;
    const char *start = template;
    const char *end = NULL;
    int offset = 0;

    for (size_t i = 0; i < num_placeholders; i++)
    {
        const char *placeholder = placeholders[i].placeholder;
        const char *value = placeholders[i].value;
        int len = strlen(placeholder);
        int value_len = strlen(value);

        while ((end = strstr(start, placeholder)) != NULL)
        {
            int new_len = end - start + value_len;
            result = (char *)realloc(result, offset + new_len + 1);
            memcpy(result + offset, start, end - start);
            memcpy(result + offset + (end - start), value, value_len);
            offset += new_len;
            start = end + len;
        }
    }

    if (start != template + strlen(template))
    {
        int new_len = template + strlen(template) - start;
        result = (char *)realloc(result, offset + new_len + 1);
        memcpy(result + offset, start, new_len);
        offset += new_len;
    }

    result = (char *)realloc(result, offset + 1);
    result[offset] = '\0';
    return result;
}

char *render_template_file(const char *filepath, placeholder_t *placeholders, size_t num_placeholders)
{
    char *template = NULL;
    char *result = NULL;
    FILE *file = fopen(filepath, "r");

    if (file == NULL)
    {
        fprintf(stderr, "Failed to open template file '%s'\n", filepath);
        return NULL;
    }

    // Get the size of the file
    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Allocate memory for the template
    template = (char *)malloc(filesize + 1);

    // Read the file into the buffer
    fread(template, 1, filesize, file);
    fclose(file);
    template[filesize] = '\0';

    // Call the existing render_template function with the buffer and the placeholders
    result = render_template(template, placeholders, num_placeholders);

    // Free the template buffer
    free(template);

    return result;
}