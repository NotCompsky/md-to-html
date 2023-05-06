const std::string_view mkview(const char* const start,  const char* const end){
	return std::string_view(start, compsky::utils::ptrdiff(end,start));
}


constexpr
bool str_eq(const char* a,  const char* b){
	bool rc = true;
	while(true){
		rc &= (*a == *b);
		if ((*a)*(*b) == 0)
			break;
		++a;
		++b;
	}
	return rc;
}
constexpr
bool str_eq(const char* a,  const std::string_view& b){
	bool rc = true;
	for (std::size_t i = 0;  i < b.size();  ++i){
		rc &= (a[i] == b.at(i));
		if (*a == 0)
			break;
	}
	return rc;
}
constexpr
bool str_eq(const std::string_view& a,  const std::string_view& b){
	bool rc = true;
	if (a.size() != b.size())
		return false;
	for (std::size_t i = 0;  i < b.size();  ++i){
		rc &= (a.at(i) == b.at(i));
	}
	return rc;
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
