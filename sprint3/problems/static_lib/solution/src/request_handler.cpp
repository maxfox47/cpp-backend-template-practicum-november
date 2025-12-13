#include "request_handler.h"

#include <algorithm>
#include <unordered_map>

namespace http_handler {

std::string ToLower(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(),
						[](unsigned char c) { return std::tolower(c); });
	return str;
}

std::string GetMimeType(const fs::path& ext) {
	static const std::unordered_map<std::string, std::string> mime_types{
		 {".htm", "text/html"},			{".html", "text/html"},
		 {".css", "text/css"},			{".txt", "text/plain"},
		 {".js", "text/javascript"},	{".json", "application/json"},
		 {".xml", "application/xml"}, {".png", "image/png"},
		 {".jpg", "image/jpeg"},		{".jpe", "image/jpeg"},
		 {".jpeg", "image/jpeg"},		{".gif", "image/gif"},
		 {".bmp", "image/bmp"},			{".ico", "image/vnd.microsoft.icon"},
		 {".tiff", "image/tiff"},		{".tif", "image/tiff"},
		 {".svg", "image/svg+xml"},	{".svgz", "image/svg+xml"},
		 {".mp3", "audio/mpeg"}};
	auto ext_str = ToLower(ext.string());
	auto it = mime_types.find(ext_str);
	return it != mime_types.end() ? it->second : "application/octet-stream";
}

bool IsSubPath(fs::path path, fs::path base) {
	path = fs::weakly_canonical(path);
	base = fs::weakly_canonical(base);

	for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
		if (p == path.end() || *p != *b) {
			return false;
		}
	}
	return true;
}

std::string DecodeUrl(std::string_view str) {
	std::string result;
	result.reserve(str.size());

	for (size_t i = 0; i < str.size(); ++i) {
		if (str[i] == '%' && i + 2 < str.size()) {
			int hex_value;
			if (sscanf(str.substr(i + 1, 2).data(), "%2x", &hex_value) == 1) {
				result += static_cast<char>(hex_value);
				i += 2;
			} else {
				result += str[i];
			}
		} else if (str[i] == '+') {
			result += ' ';
		} else {
			result += str[i];
		}
	}

	return result;
}

} // namespace http_handler

