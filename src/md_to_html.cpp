#include "md_to_html.h"
#include "inline_functions.h"

#include <compsky/os/read.hpp>
#include <compsky/utils/ptrdiff.hpp>
#include <compsky/macros/likely.hpp>
#include <compsky/asciify/asciify.hpp>
#include <vector>
#include "utils.hpp"


extern bool IS_VERBOSE;
extern bool INCLUDE_COMMENT_NODES;


void Filename::deconstruct() const {
	if ((this->n_uses == 0) and (IS_VERBOSE))
		fprintf(stderr, "%u uses: R_E_P_L_A_C_E_%.*s\n\t%.*s\n", this->n_uses, (int)name.size(), name.data(), (int)contents.size(), contents.data());
	free(const_cast<char*>(this->contents.data()));
	free(const_cast<char*>(this->name.data()));
}


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
constexpr std::string_view horizontal_rule = "<hr/>";
const char* blockquote_tagname = "blockquote";
std::vector<Filename> replacewith_filenames;

bool replace_strings(char*& dest_itr,  char*& markdown){
	if (unlikely(startswithreplace(markdown))){ // R_E_P_L_A_C_E_
		for (Filename& filename : replacewith_filenames){
			if (str_eq(markdown+14, filename.name)){
				++filename.n_uses;
				compsky::asciify::asciify(dest_itr, filename.contents);
				markdown += 14 + filename.name.size() - 1;
				return true;
			}
		}
		[[unlikely]]
		fprintf(stderr, "WARNING: Not replaced: %.30s...\n", markdown);
	}
	return false;
}


unsigned rm_paragraph_if_just_opened(char*& dest_itr){
	if ((dest_itr[-3] == '<') and (dest_itr[-2] == 'p') and (dest_itr[-1] == '>')){
		dest_itr -= 3;
		return 1;
	}
	return 0;
}
void close_paragraph_if_nonempty_already_open(char*& dest_itr){
	if ((dest_itr[-3] == '<') and (dest_itr[-2] == 'p') and (dest_itr[-1] == '>')){
		dest_itr -= 3;
	} else {
		compsky::asciify::asciify(dest_itr, "</p>");
	}
}

void log(const char* const markdown_buf,  const char* const markdown_itr,  const char* const msg,  const char* const msg_var,  const int msg_var_len){
	fprintf(stderr, "WARNING: %s at %lu: %.*s\n", msg, compsky::utils::ptrdiff(markdown_itr,markdown_buf), msg_var_len, msg_var);
}

bool is_opening_of_some_node(const char* markdown,  const std::vector<std::string_view>& tag_names){
	if (markdown[0] == '<'){
		for (const std::string_view& tagname : tag_names){
			if (str_eq(markdown+1, tagname)){
				const char endchar = markdown[1+tagname.size()];
				if ((endchar=='>') or (endchar==' '))
					return true;
			}
		}
	}
	return false;
}
bool is_some_node(const std::string_view& tag_name,  const std::vector<std::string_view>& tag_names){
	for (const std::string_view& tagname : tag_names){
		if (str_eq(tag_name, tagname))
			return true;
	}
	return false;
}

void add_tagnames_to_ls(const char* const itr,  std::vector<std::string_view>& tag_names){
	const char* _enddd = itr;
	const char* _start = itr-1;
	while(*_start != '}'){
		--_start;
	}
	fprintf(stderr, "FOUND %.*s\n", (int)compsky::utils::ptrdiff(_enddd,_start), _start);
	while(_start != _enddd){
		++_start;
		if (unlikely(startswithreplace(_start)))
			_start += 14;
		switch(*_start){
			case ' ':
			case '\n':
			case '\t':
			case ',':
			case '{':
				break;
			case '/':
				if (_start[1] == '*'){
					while(  (_start[-1] != '*') or (_start[0] != '/')  )
						++_start;
				} else {
					fprintf(stderr, "WARNING: Unexpected '/' in <style>display:block; thingie within: %.20s\n", _start-10);
				}
				break;
			case '#':
			case '.':
				while((*_start != ',') and (_start != _enddd))
					++_start;
				--_start;
				break;
			case 'a' ... 'z': {
				const char* _tagname_start = _start;
				while(((*_start >= 'a') and (*_start <= 'z')) or (*_start == '-'))
					++_start;
				if (
					(*_start == ',') or
					((_start[0] == ' ') and (_start[1] == '{')) or
					(_start[0] == '{')
				){
					fprintf(stderr, "ADDED %.*s\n", (int)compsky::utils::ptrdiff(_start,_tagname_start), _tagname_start);
					tag_names.emplace_back(_tagname_start, compsky::utils::ptrdiff(_start,_tagname_start));
				}
				break;
			}
			default:
				fprintf(stderr, "Encountered unexpected <style>display:block; thingie: %c within: %.20s\n", *_start, _start-10);
		}
	}
}

void write_n_spaces(char*& dest_itr,  unsigned n){
	while(n != 0){
		compsky::asciify::asciify(dest_itr, ' ');
		--n;
	}
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
	
	char* dest_itr = dest_buf + HALF_BUF_SZ;
	const char* markdown = markdown_buf;
	std::string_view titlestr;
	if ((markdown[0]=='-')and(markdown[1]=='-')and(markdown[2]=='-')and(markdown[3]=='\n')){
		// Skip RMD information part
		markdown += 8;
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
				markdown += 4;
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
		"<title>", titlestr, "</title>\n"
		"<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n"
		"<!-- tabsets --><!-- code folding -->\n"
		"</head>\n"
		"<body>\n"
	);
	bool is_in_blockquote = false;
	unsigned n_open_paragraphs = 0;
	unsigned dom_tag_depth_for_opening_of_paragraph = 0;
	const char* is_in_anchor_whose_title_ends_at = nullptr;
	const char* is_in_anchor_which_ends_at = nullptr;
	std::vector<std::string_view> open_dom_tag_names;
	unsigned line_began_with_n_spaces = 0;
	std::vector<std::string_view> noninline_div_tag_names;
	std::vector<std::string_view> inline_div_tag_names;
	std::vector<std::string_view> warned_about_tag_names;
	std::vector<unsigned> spaces_per_list_depth;
	noninline_div_tag_names.reserve(23+10);
	noninline_div_tag_names.emplace_back("br");
	noninline_div_tag_names.emplace_back("hr");
	noninline_div_tag_names.emplace_back("div");
	noninline_div_tag_names.emplace_back("script");
	noninline_div_tag_names.emplace_back("style");
	noninline_div_tag_names.emplace_back("h1");
	noninline_div_tag_names.emplace_back("h2");
	noninline_div_tag_names.emplace_back("h3");
	noninline_div_tag_names.emplace_back("h4");
	noninline_div_tag_names.emplace_back("h5");
	noninline_div_tag_names.emplace_back("h6");
	noninline_div_tag_names.emplace_back("h7");
	noninline_div_tag_names.emplace_back("h8");
	noninline_div_tag_names.emplace_back("h9");
	noninline_div_tag_names.emplace_back("h10");
	noninline_div_tag_names.emplace_back(blockquote_tagname);
	noninline_div_tag_names.emplace_back("ul");
	noninline_div_tag_names.emplace_back("li");
	noninline_div_tag_names.emplace_back("canvas");
	noninline_div_tag_names.emplace_back("button");
	noninline_div_tag_names.emplace_back("p");
	noninline_div_tag_names.emplace_back("table");
	noninline_div_tag_names.emplace_back("tr");
	inline_div_tag_names.reserve(17+10);
	inline_div_tag_names.emplace_back("svg");
	inline_div_tag_names.emplace_back("circle");
	inline_div_tag_names.emplace_back("path");
	inline_div_tag_names.emplace_back("img");
	inline_div_tag_names.emplace_back("td");
	inline_div_tag_names.emplace_back("th");
	inline_div_tag_names.emplace_back("time");
	inline_div_tag_names.emplace_back("input");
	inline_div_tag_names.emplace_back("highlighttagnamehere"); // TODO
	inline_div_tag_names.emplace_back("a");
	inline_div_tag_names.emplace_back("span");
	inline_div_tag_names.emplace_back("q");
	inline_div_tag_names.emplace_back("em");
	inline_div_tag_names.emplace_back("i");
	inline_div_tag_names.emplace_back("b");
	inline_div_tag_names.emplace_back("strong");
	inline_div_tag_names.emplace_back("label");
	bool done_left_quote_mark = false;
	while (true){
		if (PRINT_DEBUG){
			printf("%s\n", char2humanvis(*markdown)); fflush(stdout);
		}
		++markdown;
		bool copy_this_char_into_html = true;
		bool should_break_out = false;
		if (likely(markdown > markdown_buf+2)){
			if (unlikely((markdown[-2] == '\n') and (markdown[-3] == '\n'))){
				if (not is_opening_of_some_node(markdown-1, noninline_div_tag_names)){
					compsky::asciify::asciify(dest_itr, "<p>");
					++n_open_paragraphs;
					dom_tag_depth_for_opening_of_paragraph = open_dom_tag_names.size();
				}
			}
		}
		const char current_c = markdown[-1];
		switch(current_c){
			case 0:
			case '\n': {
				if (is_in_blockquote){
					compsky::asciify::asciify(dest_itr, "</", blockquote_tagname, ">");
					is_in_blockquote = false;
				}
				if ((n_open_paragraphs!=0) or (spaces_per_list_depth.size()!=0)){
					if (markdown[-2]=='\n')
						--dest_itr;
					if (
						(markdown[-1]==0) or
						(markdown[-2]=='\n')
					){
						if (spaces_per_list_depth.size()!=0){
							for (unsigned i = 0;  i < spaces_per_list_depth.size();  ++i){
								compsky::asciify::asciify(
									dest_itr,
									"</li>"
								);
							}
							compsky::asciify::asciify(
								dest_itr,
								"</ul>"
							);
							spaces_per_list_depth.clear();
						}
						if (n_open_paragraphs != 0){
							char* _itr = dest_itr;
							while((*_itr == ' ') or (*_itr == '\n'))
								--_itr;
							if (
								(_itr[-2] == '<') and
								(_itr[-1] == 'p') and
								(_itr[ 0] == '>')
							){
								dest_itr = _itr - 2; // Remove <p>
							} else {
								compsky::asciify::asciify(
									dest_itr,
									"</p>"
								);
							}
							--n_open_paragraphs;
						}
					}
					if (markdown[-2]=='\n')
						compsky::asciify::asciify(dest_itr, '\n');
				}
				if (markdown[-1] == 0){
					should_break_out = true;
				}
				compsky::asciify::asciify(dest_itr, '\n');
				copy_this_char_into_html = false;
				line_began_with_n_spaces = 0;
				break;
			}
			case '#': {
				if (was_newline_at(markdown_buf, markdown-2)){
					if (n_open_paragraphs != 0){
						if ((dest_itr[-3] == '<') and (dest_itr[-2] == 'p') and (dest_itr[-1] == '>'))
							dest_itr -= 3;
						--n_open_paragraphs;
					}
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
			case '"':
				if (done_left_quote_mark){
					compsky::asciify::asciify(dest_itr, "”");
					copy_this_char_into_html = false;
					done_left_quote_mark = false;
				} else {
					const char* itr = markdown;
					while((*itr != '"') and (*itr != '\n') and (*itr != 0))
						++itr;
					if (likely(*itr == '"')){
						compsky::asciify::asciify(dest_itr, "“");
						done_left_quote_mark = true;
						copy_this_char_into_html = false;
					}
				}
				break;
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
					const char* const link_end = str_if_ends_with__before__allowescapes(title_end+3, ')', '\n');
					if (likely(link_end != title_end+3-1)){
						if (likely(is_in_anchor_whose_title_ends_at == nullptr)){
							compsky::asciify::asciify(dest_itr, "<a href=\"", mkview(title_end+3,link_end+1), "\">");
							is_in_anchor_whose_title_ends_at = title_end+2;
							is_in_anchor_which_ends_at = link_end + 2;
							copy_this_char_into_html = false;
						} else {
							log(markdown_buf, markdown-1, "[link]() within [link]()", markdown-1, compsky::utils::ptrdiff(link_end+1,markdown-1));
						}
					} else if (str_if_ends_with__before(title_end+3, ')', '\n') != title_end+3-1){
						fprintf(stderr, "WARNING: Possibly invalid [](link) URL syntax: %.200s\n", title_end);
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
					if (INCLUDE_COMMENT_NODES)
						compsky::asciify::asciify(dest_itr, mkview(markdown-1,itr)); // Yes, copy comment HTML into final output - helps detect errors in R
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
					){
						++itr;
					}
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
					){
						if (unlikely(
							(itr[-17]=='{') and
							(itr[-16]=='\n') and
							(itr[-15]=='\t') and
							(itr[-14]=='d') and
							(itr[-13]=='i') and
							(itr[-12]=='s') and
							(itr[-11]=='p') and
							(itr[-10]=='l') and
							(itr[-9 ]=='a') and
							(itr[-8 ]=='y') and
							(itr[-7 ]==':') and
							(itr[-6 ]=='b') and
							(itr[-5 ]=='l') and
							(itr[-4 ]=='o') and
							(itr[-3 ]=='c') and
							(itr[-2 ]=='k') and
							(itr[-1 ]==';')
						)){ // display:block; // TODO: Improve? but why bother if it works for rpill
							add_tagnames_to_ls(itr-17, noninline_div_tag_names);
						}
						if (unlikely(
							(itr[-18]=='{') and
							(itr[-17]=='\n') and
							(itr[-16]=='\t') and
							(itr[-15]=='d') and
							(itr[-14]=='i') and
							(itr[-13]=='s') and
							(itr[-12]=='p') and
							(itr[-11]=='l') and
							(itr[-10]=='a') and
							(itr[-9 ]=='y') and
							(itr[-8 ]==':') and
							(itr[-7 ]=='i') and
							(itr[-6 ]=='n') and
							(itr[-5 ]=='l') and
							(itr[-4 ]=='i') and
							(itr[-3 ]=='n') and
							(itr[-2 ]=='e') and
							(itr[-1 ]==';')
						)){ // display:inline; // TODO: Improve? but why bother if it works for rpill
							add_tagnames_to_ls(itr-18, inline_div_tag_names);
						}
						if (unlikely(
							(itr[-24]=='{') and
							(itr[-23]=='\n') and
							(itr[-22]=='\t') and
							(itr[-21]=='d') and
							(itr[-20]=='i') and
							(itr[-19]=='s') and
							(itr[-18]=='p') and
							(itr[-17]=='l') and
							(itr[-16]=='a') and
							(itr[-15]=='y') and
							(itr[-14]==':') and
							(itr[-13]=='i') and
							(itr[-12]=='n') and
							(itr[-11]=='l') and
							(itr[-10]=='i') and
							(itr[-9 ]=='n') and
							(itr[-8 ]=='e') and
							(itr[-7 ]=='-') and
							(itr[-6 ]=='b') and
							(itr[-5 ]=='l') and
							(itr[-4 ]=='o') and
							(itr[-3 ]=='c') and
							(itr[-2 ]=='k') and
							(itr[-1 ]==';')
						)){ // display:inline-block; // TODO: Improve? but why bother if it works for rpill
							add_tagnames_to_ls(itr-24, inline_div_tag_names);
						}
						++itr;
					}
					compsky::asciify::asciify(dest_itr, mkview(markdown-1,itr));
					markdown = itr;
					copy_this_char_into_html = false;
				} else if (
					((*itr >= 'a') and (*itr <= 'z')) or
					(unlikely(startswithreplace(itr)))
				){
					if (unlikely(startswithreplace(itr)))
						itr += 14;
					while (
						((*itr >= 'a') and (*itr <= 'z')) or
						((*itr >= '0') and (*itr <= '9')) or
						(*itr == '-')
					)
						++itr;
					if ((itr[-1] != '-') and ((*itr == '>') or (*itr == ' '))){
						const std::size_t tagname_len = compsky::utils::ptrdiff(itr,tagname_start);
						
						if (*itr != '>'){
							while((*itr != 0) and (*itr != '\n') and (*itr != '>'))
								++itr;
						}
						if (likely(*itr == '>')){
							if (itr[-1] != '/'){
								if (not (
									((tagname_start[0] == 'b') and (tagname_start[1] == 'r') and (tagname_len == 2))
								)){
									open_dom_tag_names.emplace_back(tagname_start, tagname_len);
								}
							}
							++itr;
							
							if (not (is_some_node(std::string_view(tagname_start,tagname_len), noninline_div_tag_names) or is_some_node(std::string_view(tagname_start,tagname_len), inline_div_tag_names))){
								if (not is_some_node(std::string_view(tagname_start,tagname_len), warned_about_tag_names)){
									fprintf(stderr, "WARNING: Node not given inline or block CSS rule: %.*s\n", (int)tagname_len, tagname_start);
									warned_about_tag_names.emplace_back(tagname_start,tagname_len);
								}
							}
							
							compsky::asciify::asciify(dest_itr, mkview(markdown-1,itr));
							markdown = itr;
							copy_this_char_into_html = false;
						}
					}
				} else if (*itr == '/'){
					const std::string_view last_open_tagname = open_dom_tag_names[open_dom_tag_names.size()-1];
					if (str_eq(itr+1, last_open_tagname) and (itr[1+last_open_tagname.size()] == '>')){
						if (n_open_paragraphs != 0){
							if (open_dom_tag_names.size() == dom_tag_depth_for_opening_of_paragraph){
								close_paragraph_if_nonempty_already_open(dest_itr);
								--n_open_paragraphs;
							}
						}
						
						compsky::asciify::asciify(dest_itr, '<', '/', last_open_tagname, '>');
						markdown = itr+1 + last_open_tagname.size() + 1;
						open_dom_tag_names.pop_back();
						copy_this_char_into_html = false;
					} else {
						int itr_sz = 0;
						while(
							((itr[itr_sz+1] >= 'a') and (itr[itr_sz+1] <= 'z')) or
							(itr[itr_sz+1] == '-') or
							((itr[itr_sz+1] >= 'A') and (itr[itr_sz+1] <= 'Z'))
						)
							++itr_sz;
						fprintf(stderr, "Expecting </%.*s> but received </%.*s>\n%.200s\n", (int)last_open_tagname.size(), last_open_tagname.data(), itr_sz, itr+1,  markdown-190); fflush(stdout);
						abort();
					}
				} else {
					fprintf(stderr, "Treating < as NOT a tag: %.70s\n", markdown-35);
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
						n_open_paragraphs -= rm_paragraph_if_just_opened(dest_itr);
						while(*itr == ' ')
							++itr;
						compsky::asciify::asciify(dest_itr, "<",blockquote_tagname,">");
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
					n_open_paragraphs -= rm_paragraph_if_just_opened(dest_itr);
					compsky::asciify::asciify(dest_itr, horizontal_rule);
					markdown = itr+1;
					copy_this_char_into_html = false;
				} else if (
					(n_asterisks_l == 1) and
					(*after_asterisks == ' ') and
					((markdown-2-line_began_with_n_spaces == markdown_buf-1) or (*(markdown-2-line_began_with_n_spaces) == '\n'))
				){
					markdown = itr+1; // Avoid the space
					copy_this_char_into_html = false;
					const auto splds = spaces_per_list_depth.size();
					if (splds == 0)
						n_open_paragraphs -= rm_paragraph_if_just_opened(dest_itr);
					dest_itr -= (1 + line_began_with_n_spaces);
					bool is_invalid = false;
					if (splds == 0){
						compsky::asciify::asciify(dest_itr, "<ul>");
						spaces_per_list_depth.emplace_back(line_began_with_n_spaces);
					} else {
						if (spaces_per_list_depth[splds-1] < line_began_with_n_spaces){
							compsky::asciify::asciify(dest_itr, "<ul>");
							spaces_per_list_depth.emplace_back(line_began_with_n_spaces);
						} else {
							bool matched = false;
							for (unsigned i = splds;  i != 0;  ){
								--i;
								if (spaces_per_list_depth[i] == line_began_with_n_spaces){
									for (unsigned j = i;  j < splds-1;  ++j){
										compsky::asciify::asciify(dest_itr, "</li></ul>");
										spaces_per_list_depth.pop_back();
									}
									compsky::asciify::asciify(dest_itr, "</li>");
									matched = true;
									break;
								}
							}
							if (unlikely(not matched)){
								is_invalid = true;
								fprintf(stderr, "ERROR: Bad spacing in list at '%.3s': %.100s\n", markdown-1, markdown-50);
							}
						}
					}
					if (likely(not is_invalid)){
						compsky::asciify::asciify(dest_itr, "\n");
						write_n_spaces(dest_itr, line_began_with_n_spaces);
						compsky::asciify::asciify(dest_itr, "<li>");
					}
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
			case ' ': {
				if (was_newline_at(markdown_buf, markdown-2)){
					const char* itr = markdown;
					while((*itr == ' '))
						++itr;
					line_began_with_n_spaces = 1 + compsky::utils::ptrdiff(itr,markdown);
					if (*itr == '\n'){
						fprintf(stderr, "WARNING: Empty line containing %u whitespaces\n", line_began_with_n_spaces);
						line_began_with_n_spaces = 0;
						markdown = itr;
					} else {
						if (spaces_per_list_depth.size() == 0){
							if (itr[0] != '<')
								fprintf(stderr, "ERROR: Line starts with ' ' and not in <ul>: >>>%.100s<<<\n", markdown-1);
						} else {
							if (unlikely((itr[0] != '*') or (itr[1] != ' '))){
								fprintf(stderr, "ERROR: Line starts with ' ' and is after <ul> but no '* ': >>>%.100s<<<\n", markdown-1);
							} else {
								// copy_this_char_into_html = true; because line_began_with_n_spaces already deals with this
							}
						}
					}
				}
				break;
			}
		}
		if (unlikely(should_break_out))
			break;
		if (copy_this_char_into_html){
			compsky::asciify::asciify(dest_itr, current_c);
		}
	}
	if (open_dom_tag_names.size() != 0){
		for (unsigned i = 0;  i < open_dom_tag_names.size();  ++i){
			const std::string_view s = open_dom_tag_names[open_dom_tag_names.size()-i-1];
			fprintf(stderr, "Unclosed tag: %.*s\n", (int)s.size(), s.data());
		}
		fflush(stderr);
		abort();
	}
	compsky::asciify::asciify(dest_itr, "</body></html>", '\0');
	
	{
		char* dest_itr1 = dest_buf + HALF_BUF_SZ;
		char* dest_itr2 = dest_buf;
		while(*dest_itr1 != 0){
			if (likely(not replace_strings(dest_itr2, dest_itr1))){
				compsky::asciify::asciify(dest_itr2, *dest_itr1);
			}
			++dest_itr1;
		}
		return dest_itr2;
	}
}
