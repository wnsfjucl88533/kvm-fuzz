#ifndef _ELF_PARSER_H
#define _ELF_PARSER_H

#include <iostream>
#include <string>
#include <vector>
#include <elf.h>
#include "common.h"

#define BITS 64

#if BITS == 32
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Shdr Elf32_Shdr
#define Elf_Phdr Elf32_Phdr
#define Elf_Sym  Elf32_Sym
#define Elf_Rela Elf32_Rela
#define Elf_Word Elf32_Word
#define Elf_Half Elf32_Half
#define Elf_Off  Elf32_Off
#define ELF_ST_TYPE ELF32_ST_TYPE
#define ELF_ST_BIND ELF32_ST_BIND
#define ELF_ST_VISIBILITY ELF32_ST_VISIBILITY
#define ELF_R_TYPE ELF32_R_TYPE
#define ELF_R_SYM ELF32_R_SYM
#define ELFCLASS ELFCLASS32
#define EM EM_386
#define EM_S "EM_386"

#elif BITS == 64
#define Elf_Ehdr Elf64_Ehdr
#define Elf_Shdr Elf64_Shdr
#define Elf_Phdr Elf64_Phdr
#define Elf_Sym  Elf64_Sym
#define Elf_Rela Elf64_Rela
#define Elf_Word Elf64_Word
#define Elf_Half Elf64_Half
#define Elf_Off  Elf64_Off
#define ELF_ST_TYPE ELF64_ST_TYPE
#define ELF_ST_BIND ELF64_ST_BIND
#define ELF_ST_VISIBILITY ELF64_ST_VISIBILITY
#define ELF_R_TYPE ELF64_R_TYPE
#define ELF_R_SYM ELF64_R_SYM
#define ELFCLASS ELFCLASS64
#define EM EM_X86_64
#define EM_S "EM_X86_64"
#endif


struct segment_t {
	uint32_t type;
	uint32_t flags;
	vaddr_t offset;
	vaddr_t vaddr;
	paddr_t paddr;
	vsize_t filesize;
	vsize_t memsize;
	vsize_t align;
	void* data;
};

struct section_t {
	std::string name;
	uint32_t type;
	uint64_t flags;
	vaddr_t addr;
	vaddr_t offset;
	vsize_t size;
	uint32_t link;
	uint32_t info;
	vsize_t addralign;
	vsize_t entsize;
	void* data;
};

struct symbol_t {
	std::string name;
	uint8_t type;
	uint8_t binding;
	uint8_t visibility;
	uint16_t shndx;
	vaddr_t value;
	vsize_t size;
};

// Coming Soon™
struct relocation_t {

};

class ElfParser {
	public:
		ElfParser(const std::string& elf_path);
		vaddr_t entry() const;
		vaddr_t load_addr() const;
		std::string path() const;
		//std::string abs_path() const;
		std::string interpreter() const;
		std::vector<segment_t> segments() const;
		std::vector<section_t> sections() const;
		std::vector<symbol_t> symbols() const;
		//std::vector<relocation_t> relocations() const;

	private:
		uint8_t* m_data;
		vaddr_t m_entry;
		vaddr_t m_load_addr;
		std::string m_path;
		std::string m_interpreter;
		std::vector<section_t> m_sections;
		std::vector<segment_t> m_segments;
		std::vector<symbol_t> m_symbols;
		//std::vector<relocation_t> m_relocations;
};
#endif