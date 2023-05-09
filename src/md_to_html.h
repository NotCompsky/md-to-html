#pragma once

#include <string>
#include <cstring>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <compsky/os/metadata.hpp>

constexpr std::size_t HALF_BUF_SZ = 1024*1024*50;

constexpr
bool startswithreplace(const char* const str){
	return (
		(str[0 ] == 'R') and
		(str[1 ] == '_') and
		(str[2 ] == 'E') and
		(str[3 ] == '_') and
		(str[4 ] == 'P') and
		(str[5 ] == '_') and
		(str[6 ] == 'L') and
		(str[7 ] == '_') and
		(str[8 ] == 'A') and
		(str[9 ] == '_') and
		(str[10] == 'C') and
		(str[11] == '_') and
		(str[12] == 'E') and
		(str[13] == '_')
	);
}

char* md_to_html(const char* const filepath,  char* const dest_buf);

struct Filename {
	std::string_view name;
	std::string_view contents;
	unsigned n_uses;
	Filename(char(&filepath)[4096],  const unsigned dirpath_len,  const char* _name)
	: n_uses(0)
	{
		std::size_t fname_len = strlen(_name);
		memcpy(filepath+dirpath_len, _name, fname_len+1);
		if (unlikely(not startswithreplace(_name))){
			fprintf(stderr, "WARNING: File does not begin with R_E_P_L_A_C_E_: %.*s\n", (int)fname_len, _name);
		} else {
			_name += 14;
			fname_len -= 14;
		}
		
		char* const _buf1 = reinterpret_cast<char*>(malloc(fname_len));
		memcpy(_buf1, _name, fname_len);
		this->name = std::string_view(_buf1, fname_len);
		
		const std::size_t f_sz = compsky::os::get_file_sz<0>(filepath);
		char* _buf = reinterpret_cast<char*>(malloc(f_sz));
		const int fd = open(filepath, O_RDONLY);
		read(fd, _buf, f_sz);
		close(fd);
		this->contents = std::string_view(_buf, f_sz);
	}
	void deepcopy(const Filename& othr){
		this->name = othr.name;
		this->contents = othr.contents;
		this->n_uses = othr.n_uses;
	}
	Filename& operator =(const Filename&& othr){
		this->deepcopy(othr);
		return *this;
	}
	Filename(const Filename&& othr){
		this->deepcopy(othr);
	}
	~Filename(){}
	void deconstruct() const;
};
