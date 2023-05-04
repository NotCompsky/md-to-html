#include "md_to_html.h"

#include <compsky/utils/ptrdiff.hpp>
#include <compsky/macros/likely.hpp>
#include <string>
#include <compsky/os/write.hpp> // for write

bool PRINT_DEBUG = false;

int main(int argc,  const char* argv[]){
	if (likely(argc >= 2)){
		if ((argv[1][0] == '-') and (argv[1][1] == 'd') and (argv[1][2] == 0)){
			PRINT_DEBUG = true;
		}
		char* const html_buf = reinterpret_cast<char*>(malloc(1024*1024 * 20));
		if (likely(html_buf != nullptr)){
			char* const html_end = md_to_html(argv[argc-1], html_buf);
			write(1, html_buf, compsky::utils::ptrdiff(html_end,html_buf));
			return 0;
		}
	}
	constexpr const char* errmsg = "USAGE: [-d]? [/path/to/file.rmd]\n";
	write(2, errmsg, std::char_traits<char>::length(errmsg));
	return 1;
}
