#include "hnm.h"

/**
 * get_symbol_type_section64 - gets symbol type based on section attributes
 * @sym_sec: the section header of the symbol's section
 * @symbol: the 64-bit ELF symbol
 *
 * Return: the character representing the symbol type
 */

static char get_symbol_type_section64(Elf64_Shdr sym_sec, Elf64_Sym symbol)
{
    if (ELF64_ST_BIND(symbol.st_info) == STB_GNU_UNIQUE)
        return ('u');
    if (sym_sec.sh_type == SHT_NOBITS &&
        sym_sec.sh_flags == (SHF_ALLOC | SHF_WRITE))
        return ('B');
    if (sym_sec.sh_type == SHT_DYNAMIC)
        return ('D');
    if (sym_sec.sh_type != SHT_PROGBITS)
        return ('t');
    if (sym_sec.sh_flags == (SHF_ALLOC | SHF_EXECINSTR))
        return ('T');
    if (sym_sec.sh_flags == SHF_ALLOC)
        return ('R');
    if (sym_sec.sh_flags == (SHF_ALLOC | SHF_WRITE))
        return ('D');
    return ('t');
}

/**
 * get_symbol_type64 - determines the type character for a 64-bit ELF symbol
 * @symbol: the 64-bit ELF symbol
 * @section_headers: pointer to the array of section headers
 *
 * Return: the character representing the symbol type
 */

static char get_symbol_type64(Elf64_Sym symbol, Elf64_Shdr *section_headers)
{
    char symbol_type = '?';

    if (ELF64_ST_BIND(symbol.st_info) == STB_WEAK)
    {
        if (symbol.st_shndx == SHN_UNDEF)
            return ('w');
        if (ELF64_ST_TYPE(symbol.st_info) == STT_OBJECT)
            return ('V');
        return ('W');
    }
    if (symbol.st_shndx == SHN_UNDEF)
        return ('U');
    if (symbol.st_shndx == SHN_ABS)
        return ('A');
    if (symbol.st_shndx == SHN_COMMON)
        return ('C');
    if (symbol.st_shndx < SHN_LORESERVE)
        symbol_type = get_symbol_type_section64(
            section_headers[symbol.st_shndx], symbol);
    if (ELF64_ST_BIND(symbol.st_info) == STB_LOCAL)
        symbol_type = tolower(symbol_type);
    return (symbol_type);
}

/**
 * print_symbol_table64 - prints the symbol table for a 64-bit ELF file
 * @section_header: pointer to the section header of the symbol table
 * @symbol_table: pointer to the beginning of the symbol table
 * @string_table: pointer to the beginning of the string table
 * @section_headers: pointer to the array of section headers
 *
 * Return: void
 * Author: Frank Onyema Orji
 */

void print_symbol_table64(Elf64_Shdr *section_header, Elf64_Sym *symbol_table,
    char *string_table, Elf64_Shdr *section_headers)
{
    int i;
    int symbol_count = section_header->sh_size / sizeof(Elf64_Sym);
    char *symbol_name;
    char symbol_type;

    for (i = 0; i < symbol_count; i++)
    {
        Elf64_Sym symbol = symbol_table[i];

        if (symbol.st_name == 0 || ELF64_ST_TYPE(symbol.st_info) == STT_FILE)
            continue;
        symbol_name = string_table + symbol.st_name;
        symbol_type = get_symbol_type64(symbol, section_headers);
        if (symbol_type != 'U' && symbol_type != 'w')
            printf("%016lx %c %s\n", symbol.st_value, symbol_type,
                symbol_name);
        else
            printf("                 %c %s\n", symbol_type, symbol_name);
    }
}

/**
 * open_and_read_elf64 - opens a 64-bit ELF file and reads its header
 * @file_path: path to the ELF file
 * @elf_header: pointer to store the read ELF header
 *
 * Return: FILE pointer on success, NULL on failure
 */

static FILE *open_and_read_elf64(char *file_path, Elf64_Ehdr *elf_header)
{
    FILE *file = fopen(file_path, "rb");

    if (file == NULL)
    {
        fprintf(stderr, "./hnm: %s: failed to open file\n", file_path);
        return (NULL);
    }
    fread(elf_header, sizeof(Elf64_Ehdr), 1, file);
    if (elf_header->e_ident[EI_CLASS] != ELFCLASS32 &&
        elf_header->e_ident[EI_CLASS] != ELFCLASS64)
    {
        fprintf(stderr, "./hnm: %s: unsupported ELF file format\n",
            file_path);
        fclose(file);
        return (NULL);
    }
    if (elf_header->e_ident[EI_DATA] != ELFDATA2LSB &&
        elf_header->e_ident[EI_DATA] != ELFDATA2MSB)
    {
        fprintf(stderr, "./hnm: %s: unsupported ELF file endianness\n",
            file_path);
        fclose(file);
        return (NULL);
    }
    return (file);
}

/**
 * process_elf_file64 - processes a 64-bit ELF file at the given file path
 * @file_path: a pointer to a string containing the path to the ELF file
 *
 * Return: void
 */

void process_elf_file64(char *file_path)
{
    int i, sym_idx = -1;
    Elf64_Ehdr hdr;
    Elf64_Shdr *shdrs, sym_hdr, str_hdr;
    Elf64_Sym *sym_tab;
    char *str_tab;
    FILE *file = open_and_read_elf64(file_path, &hdr);

    if (!file)
        return;
    shdrs = malloc(hdr.e_shentsize * hdr.e_shnum);
    if (!shdrs)
    {
        fprintf(stderr, "./hnm: %s: memory allocation error\n", file_path);
        fclose(file);
        return;
    }
    fseek(file, hdr.e_shoff, SEEK_SET);
    fread(shdrs, hdr.e_shentsize, hdr.e_shnum, file);
    for (i = 0; i < hdr.e_shnum && sym_idx == -1; i++)
        if (shdrs[i].sh_type == SHT_SYMTAB)
            sym_idx = i;
    if (sym_idx == -1)
    {
        fprintf(stderr, "./hnm: %s: no symbols\n", file_path);
        free(shdrs);
        fclose(file);
        return;
    }
    sym_hdr = shdrs[sym_idx];
    str_hdr = shdrs[sym_hdr.sh_link];
    sym_tab = malloc(sym_hdr.sh_size);
    str_tab = malloc(str_hdr.sh_size);
    fseek(file, sym_hdr.sh_offset, SEEK_SET);
    fread(sym_tab, sym_hdr.sh_size, 1, file);
    fseek(file, str_hdr.sh_offset, SEEK_SET);
    fread(str_tab, str_hdr.sh_size, 1, file);
    print_symbol_table64(&sym_hdr, sym_tab, str_tab, shdrs);
    fclose(file);
    free(shdrs);
    free(sym_tab);
    free(str_tab);
}
