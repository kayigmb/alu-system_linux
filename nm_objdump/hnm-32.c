#include "hnm.h"

/**
 * get_symbol_type_weak - gets symbol type for weak binding
 * @symbol: the 32-bit ELF symbol
 * Return: the character representing the symbol type
 */

static char get_symbol_type_weak(Elf32_Sym symbol)
{
	if (symbol.st_shndx == SHN_UNDEF)
		return ('w');
	if (ELF32_ST_TYPE(symbol.st_info) == STT_OBJECT)
		return ('V');
	return ('W');
}

/**
 * get_symbol_type_section - gets symbol type based on section attributes
 * @symbol_section: the section header of the symbol's section
 * @symbol: the 32-bit ELF symbol
 * Return: the character representing the symbol type
 */

static char get_symbol_type_section(Elf32_Shdr symbol_section, Elf32_Sym symbol)
{
	if (ELF32_ST_BIND(symbol.st_info) == STB_GNU_UNIQUE)
		return ('u');
	if (symbol_section.sh_type == SHT_NOBITS &&
		symbol_section.sh_flags == (SHF_ALLOC | SHF_WRITE))
		return ('B');
	if (symbol_section.sh_type == SHT_DYNAMIC)
		return ('D');
	if (symbol_section.sh_type != SHT_PROGBITS)
		return ('t');
	if (symbol_section.sh_flags == (SHF_ALLOC | SHF_EXECINSTR))
		return ('T');
	if (symbol_section.sh_flags == SHF_ALLOC)
		return ('R');
	if (symbol_section.sh_flags == (SHF_ALLOC | SHF_WRITE))
		return ('D');
	return ('t');
}

/**
 * get_symbol_type32 - determines the type character for a 32-bit ELF symbol
 * @symbol: the 32-bit ELF symbol
 * @section_headers: pointer to the array of section headers
 * Return: the character representing the symbol type
 */

static char get_symbol_type32(Elf32_Sym symbol, Elf32_Shdr *section_headers)
{
	char symbol_type = '?';

	if (ELF32_ST_BIND(symbol.st_info) == STB_WEAK)
		return (get_symbol_type_weak(symbol));
	if (symbol.st_shndx == SHN_UNDEF)
		return ('U');
	if (symbol.st_shndx == SHN_ABS)
		return ('A');
	if (symbol.st_shndx == SHN_COMMON)
		return ('C');
	if (symbol.st_shndx < SHN_LORESERVE)
		symbol_type = get_symbol_type_section(
			section_headers[symbol.st_shndx], symbol);
	if (ELF32_ST_BIND(symbol.st_info) == STB_LOCAL)
		symbol_type = tolower(symbol_type);
	return (symbol_type);
}

/**
 * print_symbol_table32 - prints the symbol table for a 32-bit ELF file
 * considering special section indices and visibility attributes
 * @section_header: a pointer to the section header of the symbol table
 * @symbol_table: a pointer to the beginning of the symbol table
 * @string_table: a pointer to the beginning of the string table,
 *                which contains the names of the symbols
 * @section_headers: a pointer to the array of section headers
 * Return: nothing (void)
 * Author: Frank Onyema Orji
 */

void print_symbol_table32(Elf32_Shdr *section_header, Elf32_Sym *symbol_table,
	char *string_table, Elf32_Shdr *section_headers)
{
	int i;
	int symbol_count = section_header->sh_size / sizeof(Elf32_Sym);
	char *symbol_name, symbol_type;

	for (i = 0; i < symbol_count; i++)
	{
		Elf32_Sym symbol = symbol_table[i];

		if (symbol.st_name == 0 ||
			ELF32_ST_TYPE(symbol.st_info) == STT_FILE)
			continue;
		symbol_name = string_table + symbol.st_name;
		symbol_type = get_symbol_type32(symbol, section_headers);
		if (symbol_type != 'U' && symbol_type != 'w')
			printf("%08x %c %s\n",
				symbol.st_value, symbol_type, symbol_name);
		else
			printf("         %c %s\n", symbol_type, symbol_name);
	}
}

/**
 * process_elf_file32 - processes a 32-bit ELF file at the given file path
 * opens the file, reads the ELF header, verifies the ELF format
 * and endianness, reads the section headers to locate the symbol table
 * and the string table, reads both tables from the file,
 * and calls 'print_symbol_table32' to print the symbol information
 * @file_path: a pointer to a string containing the path to the ELF file
 * Return: nothing (void)
 */

void process_elf_file32(char *file_path)
{
	int symbol_table_index = -1;
	int i, is_little_endian, is_big_endian, string_table_index;
	Elf32_Ehdr elf_header;
	Elf32_Shdr *section_headers;
	Elf32_Shdr symbol_table_header, string_table_header;
	Elf32_Sym *symbol_table;
	char *string_table;
	FILE *file = fopen(file_path, "rb");

	if (file == NULL)
	{
		fprintf(stderr, "./hnm: %s: failed to open file\n", file_path);
		return;
	}
	fread(&elf_header, sizeof(Elf32_Ehdr), 1, file);
	if (elf_header.e_ident[EI_CLASS] != ELFCLASS32 &&
		elf_header.e_ident[EI_CLASS] != ELFCLASS64)
	{
		fprintf(stderr,
			"./hnm: %s: unsupported ELF file format\n", file_path);
		fclose(file);
		return;
	}
	is_little_endian = (elf_header.e_ident[EI_DATA] == ELFDATA2LSB);
	is_big_endian = (elf_header.e_ident[EI_DATA] == ELFDATA2MSB);
	if (!is_little_endian && !is_big_endian)
	{
		fprintf(stderr,
			"./hnm: %s: unsupported ELF file endianness\n", file_path);
		fclose(file);
		return;
	}
	section_headers = malloc(elf_header.e_shentsize * elf_header.e_shnum);
	if (section_headers == NULL)
	{
		fprintf(stderr,
			"./hnm: %s: memory allocation error\n", file_path);
		fclose(file);
		return;
	}
	fseek(file, elf_header.e_shoff, SEEK_SET);
	fread(section_headers, elf_header.e_shentsize, elf_header.e_shnum, file);
	for (i = 0; i < elf_header.e_shnum; i++)
	{
		if (section_headers[i].sh_type == SHT_SYMTAB)
		{
			symbol_table_index = i;
			break;
		}
	}
	if (symbol_table_index == -1)
	{
		fprintf(stderr, "./hnm: %s: no symbols\n", file_path);
		fclose(file);
		free(section_headers);
		return;
	}
	symbol_table_header = section_headers[symbol_table_index];
	symbol_table = malloc(symbol_table_header.sh_size);
	fseek(file, symbol_table_header.sh_offset, SEEK_SET);
	fread(symbol_table, symbol_table_header.sh_size, 1, file);
	string_table_index = symbol_table_header.sh_link;
	string_table_header = section_headers[string_table_index];
	string_table = malloc(string_table_header.sh_size);
	fseek(file, string_table_header.sh_offset, SEEK_SET);
	fread(string_table, string_table_header.sh_size, 1, file);
	print_symbol_table32(
		&symbol_table_header, symbol_table, string_table, section_headers);
	fclose(file);
	free(section_headers);
	free(symbol_table);
	free(string_table);
}
