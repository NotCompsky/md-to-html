#include "md_to_html.h"

#include <compsky/utils/ptrdiff.hpp>
#include <compsky/macros/likely.hpp>
#include <string>
#include <compsky/os/write.hpp> // for write

bool PRINT_DEBUG = false;

int main(int argc,  const char* argv[]){
	if (likely((argc >= 2) and (argc <= 4))){
		if ((argv[1][0] == '-') and (argv[1][1] == 'd') and (argv[1][2] == 0)){
			PRINT_DEBUG = true;
		}
		int out_fd = 1;
		if (argc-PRINT_DEBUG == 3){
			out_fd = open(argv[2+PRINT_DEBUG], O_WRONLY|O_CREAT, 0644);
		}
		char* const html_buf = reinterpret_cast<char*>(malloc(1024*1024 * 20));
		if (likely(html_buf != nullptr)){
			char* const html_end = md_to_html(argv[1+PRINT_DEBUG], html_buf);
			write(out_fd, html_buf, compsky::utils::ptrdiff(html_end,html_buf));
			if (out_fd != 1)
				close(out_fd);
			return 0;
		}
	}
	constexpr const char* errmsg = "USAGE: [-d]? [/path/to/file.rmd] [/path/to/outfile.html]?\n";
	write(2, errmsg, std::char_traits<char>::length(errmsg));
	return 1;
}
