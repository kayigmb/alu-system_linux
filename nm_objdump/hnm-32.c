#include "hnm.h"

/**
 * main - entry point for ELF file processing (32-bit variant)
 * @argc: number of command-line arguments
 * @argv: array of command-line arguments where argv[1] is the ELF file path
 *
 * Return: 0 on success, 0 on error (missing argument or unsupported ELF type)
 * Author: Frank Onyema Orji
 */

int main(int argc, char *argv[])
{
    char *file_path;
    FILE *file;
    Elf64_Ehdr elf_header;

    if (argc < 2)
    {
        printf("Il faut fournir un fichier ELF !\n");
        return (0);
    }

    file_path = argv[1];
    file = fopen(file_path, "rb");
    if (file == NULL)
    {
        printf("Il y a une erreur pour de l'ouverture du fichier\n");
        return (0);
    }

    fread(&elf_header, sizeof(Elf64_Ehdr), 1, file);

    if (elf_header.e_ident[EI_CLASS] == ELFCLASS32)
    {
        process_elf_file32(file_path);
    }
    else if (elf_header.e_ident[EI_CLASS] == ELFCLASS64)
    {
        process_elf_file64(file_path);
    }
    else
    {
        printf("Type de fichier ELF non pris en charge...\n");
    }

    fclose(file);

    return (0);
}
