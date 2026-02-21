#include "hnm.h"

/**
 * print_symbol_table64 - prints the symbol table for a 64-bit ELF file
 * @section_header: pointer to the symbol table section header
 * @symbol_table: pointer to the beginning of the symbol table
 * @string_table: pointer to the string table
 * @section_headers: pointer to the section headers array
 * Return: nothing
 */

void print_symbol_table64(Elf64_Shdr *section_header,
	Elf64_Sym *symbol_table,
	char *string_table,
	Elf64_Shdr *section_headers)
{
	int i;
	int symbol_count;
	char *symbol_name;
	char symbol_type;

	symbol_count = section_header->sh_size / sizeof(Elf64_Sym);

	for (i = 0; i < symbol_count; i++)
	{
		Elf64_Sym symbol = symbol_table[i];

		symbol_name = string_table + symbol.st_name;

		if (symbol.st_name != 0 &&
		    ELF64_ST_TYPE(symbol.st_info) != STT_FILE)
		{
			symbol_type = '?';

			if (ELF64_ST_BIND(symbol.st_info) == STB_WEAK)
			{
				if (symbol.st_shndx == SHN_UNDEF)
					symbol_type = 'w';
				else if (ELF64_ST_TYPE(symbol.st_info) ==
					 STT_OBJECT)
					symbol_type = 'V';
				else
					symbol_type = 'W';
			}
			else if (symbol.st_shndx == SHN_UNDEF)
				symbol_type = 'U';
			else if (symbol.st_shndx == SHN_ABS)
				symbol_type = 'A';
			else if (symbol.st_shndx == SHN_COMMON)
				symbol_type = 'C';
			else if (symbol.st_shndx < SHN_LORESERVE)
			{
				Elf64_Shdr symbol_section =
					section_headers[symbol.st_shndx];

				if (ELF64_ST_BIND(symbol.st_info) ==
				    STB_GNU_UNIQUE)
					symbol_type = 'u';
				else if (symbol_section.sh_type ==
					 SHT_NOBITS &&
					 symbol_section.sh_flags ==
					 (SHF_ALLOC | SHF_WRITE))
					symbol_type = 'B';
				else if (symbol_section.sh_type ==
					 SHT_PROGBITS)
				{
					if (symbol_section.sh_flags ==
					    (SHF_ALLOC | SHF_EXECINSTR))
						symbol_type = 'T';
					else if (symbol_section.sh_flags ==
						 SHF_ALLOC)
						symbol_type = 'R';
					else if (symbol_section.sh_flags ==
						 (SHF_ALLOC | SHF_WRITE))
						symbol_type = 'D';
				}
				else if (symbol_section.sh_type ==
					 SHT_DYNAMIC)
					symbol_type = 'D';
				else
					symbol_type = 't';
			}

			if (ELF64_ST_BIND(symbol.st_info) ==
			    STB_LOCAL)
				symbol_type = tolower(symbol_type);

			if (symbol_type != 'U' &&
			    symbol_type != 'w')
			{
				printf("%016lx %c %s\n",
				       symbol.st_value,
				       symbol_type,
				       symbol_name);
			}
			else
			{
				printf("                 %c %s\n",
				       symbol_type,
				       symbol_name);
			}
		}
	}
}

/**
 * process_elf_file64 - processes a 64-bit ELF file
 * @file_path: path to the ELF file
 * Return: nothing
 */

void process_elf_file64(char *file_path)
{
	int symbol_table_index;
	int string_table_index;
	int i;
	int is_little_endian;
	int is_big_endian;
	FILE *file;
	Elf64_Ehdr elf_header;
	Elf64_Shdr *section_headers;
	Elf64_Shdr symbol_table_header;
	Elf64_Shdr string_table_header;
	Elf64_Sym *symbol_table;
	char *string_table;

	symbol_table_index = -1;

	file = fopen(file_path, "rb");
	if (file == NULL)
	{
		fprintf(stderr,
			"./hnm: %s: failed to open file\n",
			file_path);
		return;
	}

	fread(&elf_header, sizeof(Elf64_Ehdr), 1, file);

	if (elf_header.e_ident[EI_CLASS] != ELFCLASS32 &&
	    elf_header.e_ident[EI_CLASS] != ELFCLASS64)
	{
		fprintf(stderr,
			"./hnm: %s: unsupported ELF file format\n",
			file_path);
		fclose(file);
		return;
	}

	is_little_endian =
		(elf_header.e_ident[EI_DATA] == ELFDATA2LSB);
	is_big_endian =
		(elf_header.e_ident[EI_DATA] == ELFDATA2MSB);

	if (!is_little_endian && !is_big_endian)
	{
		fprintf(stderr,
			"./hnm: %s: unsupported ELF endianness\n",
			file_path);
		fclose(file);
		return;
	}

	section_headers = malloc(elf_header.e_shentsize *
				 elf_header.e_shnum);
	if (section_headers == NULL)
	{
		fprintf(stderr,
			"./hnm: %s: memory allocation error\n",
			file_path);
		fclose(file);
		return;
	}

	fseek(file, elf_header.e_shoff, SEEK_SET);
	fread(section_headers,
	      elf_header.e_shentsize,
	      elf_header.e_shnum,
	      file);

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
		fprintf(stderr,
			"./hnm: %s: no symbols\n",
			file_path);
		free(section_headers);
		fclose(file);
		return;
	}

	symbol_table_header =
		section_headers[symbol_table_index];

	symbol_table = malloc(symbol_table_header.sh_size);
	if (symbol_table == NULL)
	{
		free(section_headers);
		fclose(file);
		return;
	}

	fseek(file, symbol_table_header.sh_offset, SEEK_SET);
	fread(symbol_table,
	      symbol_table_header.sh_size,
	      1,
	      file);

	string_table_index = symbol_table_header.sh_link;
	string_table_header =
		section_headers[string_table_index];

	string_table = malloc(string_table_header.sh_size);
	if (string_table == NULL)
	{
		free(symbol_table);
		free(section_headers);
		fclose(file);
		return;
	}

	fseek(file, string_table_header.sh_offset, SEEK_SET);
	fread(string_table,
	      string_table_header.sh_size,
	      1,
	      file);

	print_symbol_table64(&symbol_table_header,
			     symbol_table,
			     string_table,
			     section_headers);

	free(string_table);
	free(symbol_table);
	free(section_headers);
	fclose(file);
}
