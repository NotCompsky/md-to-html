#include "md_to_html.h"
#include "inline_functions.h"

#include <compsky/os/read.hpp>
#include <compsky/utils/ptrdiff.hpp>
#include <compsky/macros/likely.hpp>
#include <compsky/asciify/asciify.hpp>


extern bool PRINT_DEBUG;
constexpr std::size_t emphasis_max = 2;
constexpr const char* emphasis_open[2] = {
	"<i>",
	"<b>"
};
constexpr const char* emphasis_close[2] = {
	"</i>",
	"</b>"
};
constexpr bool using_knitr_output = true;


const std::string_view mkview(const char* const start,  const char* const end){
	return std::string_view(start, compsky::utils::ptrdiff(end,start));
}


const char* str_if_ends_with__before(const char* const init,  const char d,  const char before1){
	// e.g. (init,d,before1)==("foo bar]",']','\n') => position before ']'
	const char* itr = init;
	while(*itr != 0){
		if (*itr == before1){
			break;
		}
		if (*itr == d){
			return itr-1;
		}
		++itr;
	}
	return init-1;
}
const char* str_if_ends_with__before__pair_up_with(const char* const init,  const char d,  const char before1,  const char pair_with){
	// e.g. (init,d,before1)==("foo [bar] ree]",']','\n','[') => position after "ree" before ']'
	const char* itr = init;
	unsigned n_paired = 0;
	while(*itr != 0){
		if (*itr == before1){
			break;
		}
		if (*itr == pair_with)
			++n_paired;
		if (*itr == d){
			if (n_paired == 0)
				return itr-1;
			else
				--n_paired;
		}
		++itr;
	}
	return init-1;
}

const char* str_if_ends_with(const char* const init,  const char d){
	const char* itr = init;
	while(*itr != 0){
		if (*itr == d){
			return itr-1;
		}
		++itr;
	}
	return init-1;
}

const char* str_if_ends_with3(const char* const init,  const char d1,  const char d2,  const char d3){
	const char* itr = init;
	while(*itr != 0){
		if ((itr[0] == d1) and (itr[1] == d2) and (itr[2] == d3)){
			return itr-1;
		}
		++itr;
	}
	return init-1;
}

void log(const char* const markdown_buf,  const char* const markdown_itr,  const char* const msg,  const char* const msg_var,  const int msg_var_len){
	fprintf(stderr, "WARNING: %s at %lu: %.*s\n", msg, compsky::utils::ptrdiff(markdown_itr,markdown_buf), msg_var_len, msg_var);
}

const char* char2humanvis(const char c){
	static char buf[2] = {0,0};
	switch(c){
		case '\n':
			return "\\n";
		case ' ':
			return "[SPACE]";
		default:
			buf[0] = c;
			return buf;
	}
}

bool was_newline_at(const char* const markdown_buf,  const char* const ptr){
	return ((markdown_buf==ptr+1) or (*ptr == '\n'));
}

char* md_to_html(const char* const filepath,  char* const dest_buf){
	compsky::os::ReadOnlyFile f(filepath);
	if (unlikely(f.is_null())){
		log(nullptr, nullptr, "Cannot open file", filepath, strlen(filepath));
		return dest_buf;
	}
	char* const markdown_buf = reinterpret_cast<char*>(malloc(f.size()+1));
	f.read_into_buf(markdown_buf, f.size());
	markdown_buf[f.size()] = 0;
	
	char* dest_itr = dest_buf;
	const char* markdown = markdown_buf;
	std::string_view titlestr;
	if ((markdown[0]=='-')and(markdown[1]=='-')and(markdown[2]=='-')and(markdown[3]=='\n')){
		// Skip RMD information part
		markdown += 7;
		while(  (markdown[-4]!='-') or (markdown[-3]!='-') or (markdown[-2]!='-') or (markdown[-1]!='\n')  ){
			if (
				(markdown[-4] == 't') and
				(markdown[-3] == 'i') and
				(markdown[-2] == 't') and
				(markdown[-1] == 'l') and
				(markdown[ 0] == 'e') and
				(markdown[ 1] == ':') and
				(markdown[ 2] == ' ') and
				(markdown[ 3] == '"')
			){
				markdown += 3;
				const char* const title_start = markdown;
				while(*markdown != '"')
					++markdown;
				titlestr = std::string_view(title_start, compsky::utils::ptrdiff(markdown,title_start));
			}
			++markdown;
		}
	}
	compsky::asciify::asciify(dest_itr,
		"<!DOCTYPE html>\n"
		"<html>\n"
		"<head>\n"
		"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\">\n"
		"<meta charset=\"utf-8\">\n"
		"<meta name=\"generator\" content=\"pandoc\">\n"
		"<meta http-equiv=\"X-UA-Compatible\" content=\"IE=EDGE\">\n"
		"<title>", titlestr, "</title>\n"
		"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
		"<!-- tabsets --><!-- code folding -->\n"
		"</head>\n"
		"<body>\n"
	);
	bool is_in_blockquote = false;
	const char* is_in_anchor_whose_title_ends_at = nullptr;
	const char* is_in_anchor_which_ends_at = nullptr;
	while (true){
		if (PRINT_DEBUG){
			printf("%s\n", char2humanvis(*markdown)); fflush(stdout);
		}
		++markdown;
		bool copy_this_char_into_html = true;
		bool should_break_out = false;
		switch(markdown[-1]){
			case 0:
			case '\n': {
				if (is_in_blockquote){
					compsky::asciify::asciify(dest_itr, "</blockquote>");
					is_in_blockquote = false;
				}
				if (markdown[-1] == 0){
					should_break_out = true;
				}
				break;
			}
			case '#': {
				if (was_newline_at(markdown_buf, markdown-2)){
					unsigned num_hashes = 1;
					const char* itr = markdown;
					while (*itr == '#'){
						++num_hashes;
						++itr;
					}
					while (*itr == ' '){
						++itr;
					}
					const char* const title_end = str_if_ends_with(itr, '\n');
					if (unlikely(title_end == itr-1)){
						log(markdown_buf, itr, "Empty title", itr, 0);
					} else {
						compsky::asciify::asciify(dest_itr, "<h", num_hashes, ">", mkview(itr,title_end+1), "</h", num_hashes, ">");
						markdown = title_end + 1;
						copy_this_char_into_html = false;
					}
				}
				break;
			}
			case ']':
				if (markdown == is_in_anchor_whose_title_ends_at){
					compsky::asciify::asciify(dest_itr, "</a>");
					markdown = is_in_anchor_which_ends_at;
					copy_this_char_into_html = false;
					is_in_anchor_whose_title_ends_at = nullptr;
					is_in_anchor_which_ends_at = nullptr;
				}
				break;
			case '[': {
				const char* const title_end = str_if_ends_with__before__pair_up_with(markdown, ']', '\n', '[');
				if ((likely(title_end != markdown-1)) and (likely(title_end[2] == '('))){
					const char* const link_end = str_if_ends_with__before(title_end+3, ')', '\n');
					if (likely(link_end != title_end+3-1)){
						if (likely(is_in_anchor_whose_title_ends_at == nullptr)){
							compsky::asciify::asciify(dest_itr, "<a href=\"", mkview(title_end+3,link_end+1), "\">");
							is_in_anchor_whose_title_ends_at = title_end+2;
							is_in_anchor_which_ends_at = link_end + 2;
							copy_this_char_into_html = false;
						} else {
							log(markdown_buf, markdown-1, "[link]() within [link]()", markdown-1, compsky::utils::ptrdiff(link_end+1,markdown-1));
						}
					}
				}
				break;
			}
			case '<': {
				const char* itr = markdown;
				const char* const tagname_start = itr; // for <something this is at the 's' place
				if ((itr[0]=='!') and (itr[1]=='-') and (itr[2]=='-')){ // <!-- -->
					itr += 6;
					while(
						(itr[-3]!='-') or (itr[-2]!='-') or (itr[-1]!='>')
					)
						++itr;
					markdown = itr;
					copy_this_char_into_html = false;
				} else if (  (itr[0]=='s') and (itr[1]=='c') and (itr[2]=='r') and (itr[3]=='i') and (itr[4]=='p') and (itr[5]=='t') and ((itr[6]=='>') or (itr[6]==' '))  ){ // <script></script>
					itr += 7+9;
					while(
						(itr[-9]!='<') or
						(itr[-8]!='/') or
						(itr[-7]!='s') or
						(itr[-6]!='c') or
						(itr[-5]!='r') or
						(itr[-4]!='i') or
						(itr[-3]!='p') or
						(itr[-2]!='t') or
						(itr[-1]!='>')
					)
						++itr;
					compsky::asciify::asciify(dest_itr, mkview(markdown-1,itr));
					markdown = itr;
					copy_this_char_into_html = false;
				} else if (  (itr[0]=='s') and (itr[1]=='t') and (itr[2]=='y') and (itr[3]=='l') and (itr[4]=='e') and ((itr[5]=='>') or (itr[5]==' '))  ){ // <style></style>
					itr += 6+8;
					while(
						(itr[-8]!='<') or
						(itr[-7]!='/') or
						(itr[-6]!='s') or
						(itr[-5]!='t') or
						(itr[-4]!='y') or
						(itr[-3]!='l') or
						(itr[-2]!='e') or
						(itr[-1]!='>')
					)
						++itr;
					compsky::asciify::asciify(dest_itr, mkview(markdown-1,itr));
					markdown = itr;
					copy_this_char_into_html = false;
				} else if ((*itr >= 'a') and (*itr <= 'z')){
					while (
						((*itr >= 'a') and (*itr <= 'z')) or
						((*itr >= '0') and (*itr <= '9')) or
						(*itr == '-')
					)
						++itr;
					if ((itr[-1] == '-') or ((*itr != '>') and (*itr != ' ')))
						continue;
					const std::size_t tagname_len = compsky::utils::ptrdiff(itr,tagname_start);
					if (*itr != '>'){
						while((*itr != 0) and (*itr != '\n') and (*itr != '>'))
							++itr;
					}
					if (*itr == '>'){
						++itr;
						while(true){
							const char* const itr2 = str_if_ends_with__before(itr, '<', '\n'); // __before '\n' means that HTML is only direct-pasted if it is in one line
							const char* tagname2_start;
							if (unlikely(itr2 == itr-1))
								break;
							bool matches = false;
							if (itr2[2] == '/'){
								// WARNING: Not proper HTML parsing! Doesn't account for possibility of nested HTML tags (but could be implemented by keeping track of depth of outer tag) # TODO
								tagname2_start = itr2+3;
								matches = true;
								for(unsigned i = 0;  i < tagname_len;  ++i){
									matches &= (tagname2_start[i] == tagname_start[i]);
								}
								matches &= (tagname2_start[tagname_len] == '>');
							}
							if (matches){
								if ((tagname_start[0]=='b') and (tagname_start[1]=='>')){
									fprintf(stderr, "<b>: %.*s\n", (int)compsky::utils::ptrdiff(tagname2_start-2,tagname_start+2), tagname_start+2);
								}
								compsky::asciify::asciify(dest_itr, mkview(tagname_start-1,tagname2_start+tagname_len+1));
								markdown = tagname2_start+tagname_len + 1;
								copy_this_char_into_html = false;
								break;
							} else {
								itr = itr2+1+1;
							}
						}
					}
				}
				break;
			}
			case '>': {
				if ((not is_in_blockquote) and (was_newline_at(markdown_buf, markdown-2))){
					const char* itr = markdown;
					const char* const line_end = str_if_ends_with(itr, '\n');
					if (unlikely(line_end == itr-1)){
						log(markdown_buf, itr, "Empty blockquote", itr, 0);
					} else {
						while(*itr == ' ')
							++itr;
						compsky::asciify::asciify(dest_itr, "<blockquote>");
						markdown = itr;
						copy_this_char_into_html = false;
						is_in_blockquote = true;
					}
				}
				break;
			}
			case '*': {
				unsigned n_asterisks_l = 1;
				const char* itr = markdown;
				while(*itr == '*'){
					++n_asterisks_l;
					++itr;
				}
				const char* const after_asterisks = itr;
				if ((n_asterisks_l == 3) and was_newline_at(markdown_buf, markdown-2) and (*after_asterisks == '\n')){
					compsky::asciify::asciify(dest_itr, "<hr/>");
					markdown = itr+1;
					copy_this_char_into_html = false;
				} else if ((n_asterisks_l == 1) and (*after_asterisks == ' ')){
					compsky::asciify::asciify(dest_itr, '-');
					markdown = itr;
					copy_this_char_into_html = false;
				} else {
					if ((*after_asterisks != ' ') and (n_asterisks_l <= emphasis_max)){
						const char* const start_of_emphasised_text = itr;
						unsigned n_asterisks_r = 0;
						while((*itr != 0) and (*itr != '\n')){
							if (*itr != '*'){
								n_asterisks_r = 0;
							} else {
								++n_asterisks_r;
								if (n_asterisks_r == n_asterisks_l){
									break;
								}
							}
							++itr;
						}
						if (likely(n_asterisks_r == n_asterisks_l)){
							// TODO: Deal with [links](https://...)
							compsky::asciify::asciify(dest_itr, emphasis_open[n_asterisks_l-1], mkview(start_of_emphasised_text,itr+1-n_asterisks_r), emphasis_close[n_asterisks_l-1]);
							fprintf(stderr, "from */**: %.*s\n", (int)(compsky::utils::ptrdiff(itr,start_of_emphasised_text)+1-n_asterisks_r), start_of_emphasised_text);
							markdown = itr+1;
							copy_this_char_into_html = false;
						}
					}
					if (copy_this_char_into_html and (n_asterisks_l == 1)){
						// failed to match it to an emphasis
						const char* itr_backwards = markdown;
						bool any_non_spaces = false;
						while((*itr_backwards != '\n') and (itr_backwards != markdown_buf)){
							--itr_backwards;
							if (*itr_backwards != ' '){
								any_non_spaces = true;
								break;
							}
						}
						if (not any_non_spaces){
							// TODO: HTML list
						}
					}
				}
				break;
			}
			case '`': {
				if (using_knitr_output){
					// NOTE: There should be no ``` in the <script>
					
					if (was_newline_at(markdown_buf, markdown-2)){ // current character was the start of line
						if ((markdown[0] == '`') and (markdown[1] == '`')){
							bool is_badly_formatted_R_execstr = true;
							if ((markdown[2] == 'r') and (markdown[3] == '\n')){
								// "```r\n"   This is a comment which shows the R source code
								const char* itr = markdown+8;
								while(  (itr[0] != '\n') or (itr[1] != '`') or (itr[2] != '`') or (itr[3] != '`')  )
									++itr;
								markdown = itr+4;
								is_badly_formatted_R_execstr = false;
							} else if ((markdown[2] == '\n') and (markdown[3] == '#') and (markdown[4] == '#') and (markdown[5] == ' ')){
								// "```\n## " This shows the code's output - which might be a string (visible HTML output) or the error/cat output
								if ((markdown[6] == '[') and (markdown[7] == '1') and (markdown[8] == ']') and (markdown[9] == ' ') and (markdown[10] == '"')){
									// "```\n## [1] \""   visible HTML output, as a string array of length 1
									const char* itr = markdown+11;
									while(*itr != '\n'){
										switch(*itr){
											case '\\':
												++itr;
												switch(*itr){
													case '"':
														break;
													default:
														log(markdown_buf, itr, "Unplanned escape char in knitr output", itr-10, 20);
														abort();
												}
												break;
											default:
												compsky::asciify::asciify(dest_itr, *itr);
										}
										++itr;
									}
									if ((itr[1] != '`') or (itr[2] != '`') or (itr[3] != '`')){
										log(markdown_buf, itr, "Bad R output", markdown+11-10, 30);
										abort();
									} else {
										itr += 4;
										if (dest_itr[-1] == '"')
											--dest_itr;
										markdown = itr;
										is_badly_formatted_R_execstr = false;
									}
								} else {
									// "```\n## "   error/cat outputs
									const char* itr = markdown+8;
									while(  (itr[0] != '\n') or (itr[1] != '`') or (itr[2] != '`') or (itr[3] != '`')  )
										++itr;
									markdown = itr+4;
									is_badly_formatted_R_execstr = false;
								}
							}
							if (unlikely(is_badly_formatted_R_execstr)){
								log(markdown_buf, markdown, "Bad inline R", markdown-1, 100);
							} else {
								copy_this_char_into_html = false;
							}
						}
					} else {
						log(markdown_buf, markdown, "` in knitr output not at newline", markdown-10, 20);
						fprintf(stderr, "markdown-1==>>>%c<<<\nmarkdown[-2]==>>>%c<<<\n", markdown[-1], markdown[-2]); fflush(stdout);
					}
				} else {
					bool is_badly_formatted_R_execstr = true;
					if ((markdown[0] == 'r') and (markdown[1] == ' ')){
						const char* const R_statement_end = str_if_ends_with__before(markdown+2, '`', '\n');
						if (likely(R_statement_end != markdown+2-1)){
							printf("Inline R: %.*s\n", (int)compsky::utils::ptrdiff(R_statement_end,markdown+2-1), markdown+2); fflush(stdout);
							markdown = R_statement_end + 2;
							is_badly_formatted_R_execstr = false;
						}
					} else if ((markdown[0] == '`') and (markdown[1] == '`') and (markdown[2] == '{') and (markdown[3] == 'r')){
						if ((markdown[4] == '}') and (markdown[5] == '\n')){
							const char* const R_statement_end = str_if_ends_with3(markdown+6, '`','`','`');
							if (likely(R_statement_end != markdown+6-1)){
								is_badly_formatted_R_execstr = false;
							}
						} else if ((markdown[4] == ' ') and (markdown[5] == 'c') and (markdown[6] == 'h') and (markdown[7] == 'i') and (markdown[8] == 'l') and (markdown[9] == 'd') and (markdown[10] == ' ') and (markdown[11] == '=') and (markdown[12] == ' ') and (markdown[13] == '\'')){
							const char* const filepath_end = str_if_ends_with__before(markdown+14, '\'', '\n');
							const char* const R_statement_end = str_if_ends_with3(markdown+14, '`','`','`');
							if ((likely(filepath_end != markdown+14-1)) and (likely(R_statement_end != markdown+14-1))){
								printf("CHILD %.*s\n", (int)compsky::utils::ptrdiff(filepath_end+1,markdown+14), markdown+14);
								markdown = R_statement_end+3+2;
								is_badly_formatted_R_execstr = false;
							}
						}
					}
					copy_this_char_into_html = is_badly_formatted_R_execstr;
					if (unlikely(is_badly_formatted_R_execstr)){
						log(markdown_buf, markdown, "Bad inline R", markdown, 100);
					}
				}
				break;
			}
			case '\\': {
				switch(*markdown){
					case '\\':
						++markdown;
					case '$':
					case '*':
						copy_this_char_into_html = false; // The '$' will be though
						break;
					case '"':
						break;
					default: {
						const char* itr = markdown;
						log(markdown_buf, itr, "Bad escape", markdown-50, 101);
						abort();
					}
				}
				break;
			}
		}
		if (unlikely(should_break_out))
			break;
		if (copy_this_char_into_html)
			compsky::asciify::asciify(dest_itr, markdown[-1]);
	}
	compsky::asciify::asciify(dest_itr, "</body></html>");
	return dest_itr;
}
