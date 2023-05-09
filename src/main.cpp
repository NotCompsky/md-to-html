#include "md_to_html.h"

#include <compsky/utils/ptrdiff.hpp>
#include <compsky/macros/likely.hpp>
#include <string>
#include <compsky/os/write.hpp> // for write

extern const char* blockquote_tagname;
bool PRINT_DEBUG = false;

int main(int argc,  const char* const* argv){
	bool any_errors = false;
	++argv;
	while((argv[0][0] == '-') and (argv[0][2] == 0)){
		switch(argv[0][1]){
			case 'd':
				PRINT_DEBUG = true;
				break;
			case 'b':
				blockquote_tagname = *(++argv);
				break;
			default:
				any_errors = true;
				break;
		}
		++argv;
	}
	if (likely(not any_errors) and likely((argc >= 2) and (argc <= 4))){
		int out_fd = 1;
		if (argc-PRINT_DEBUG == 3){
			out_fd = open(argv[1], O_WRONLY|O_CREAT, 0644);
		}
		char* const html_buf = reinterpret_cast<char*>(malloc(1024*1024 * 20));
		if (likely(html_buf != nullptr)){
			char* const html_end = md_to_html(argv[0], html_buf);
			write(out_fd, html_buf, compsky::utils::ptrdiff(html_end,html_buf));
			if (out_fd != 1)
				close(out_fd);
			return 0;
		}
	}
	constexpr const char* errmsg = "USAGE: [-d]? [-b BLOCKQUOTE_TAGNAME]? [/path/to/file.rmd] [/path/to/outfile.html]?\n";
	write(2, errmsg, std::char_traits<char>::length(errmsg));
	return 1;
}
