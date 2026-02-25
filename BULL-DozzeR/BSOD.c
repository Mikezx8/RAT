#include <stdio.h>
#include <stdlib.h>

int main() {
    FILE *file;
    const char *filepath = "C:\\windows\\system32\\config\\OSDATA";
    const char *content = "what is this data";
    
    // Attempt to create and write to the file
    file = fopen(filepath, "w");
    
    if (file == NULL) {
        printf("Error: Could not create file. You may need to run this program as administrator.\n");
        return 1;
    }
    
    // Write content to the file
    fprintf(file, "%s", content);
    
    // Close the file
    fclose(file);
    
    printf("File created successfully: %s\n", filepath);
    printf("Content written: \"%s\"\n", content);
    
    return 0;
}