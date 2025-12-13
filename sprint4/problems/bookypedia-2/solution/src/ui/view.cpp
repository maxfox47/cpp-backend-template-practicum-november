#include "view.h"

#include <algorithm>
#include <boost/algorithm/string/trim.hpp>
#include <cassert>
#include <iostream>
#include <sstream>
#include <regex>
#include <set>

#include "../app/use_cases.h"
#include "../menu/menu.h"

using namespace std::literals;
namespace ph = std::placeholders;

namespace ui {
namespace detail {

std::ostream& operator<<(std::ostream& out, const AuthorInfo& author) {
    out << author.name;
    return out;
}

std::ostream& operator<<(std::ostream& out, const BookInfo& book) {
    out << book.title << " by " << book.author_name << ", " << book.publication_year;
    return out;
}

}  // namespace detail

template <typename T>
void PrintVector(std::ostream& out, const std::vector<T>& vector) {
    int i = 1;
    for (auto& value : vector) {
        out << i++ << " " << value << std::endl;
    }
}

View::View(menu::Menu& menu, app::UseCases& use_cases, std::istream& input, std::ostream& output)
    : menu_{menu}
    , use_cases_{use_cases}
    , input_{input}
    , output_{output} {
    menu_.AddAction("AddAuthor"s, "name"s, "Adds author"s, std::bind(&View::AddAuthor, this, ph::_1));
    menu_.AddAction("AddBook"s, "<pub year> <title>"s, "Adds book"s,
                    std::bind(&View::AddBook, this, ph::_1));
    menu_.AddAction("ShowAuthors"s, {}, "Show authors"s, std::bind(&View::ShowAuthors, this));
    menu_.AddAction("ShowBooks"s, {}, "Show books"s, std::bind(&View::ShowBooks, this));
    menu_.AddAction("ShowAuthorBooks"s, {}, "Show author books"s,
                    std::bind(&View::ShowAuthorBooks, this));
    menu_.AddAction("DeleteAuthor"s, "[name]"s, "Delete author"s,
                    std::bind(&View::DeleteAuthor, this, ph::_1));
    menu_.AddAction("EditAuthor"s, "[name]"s, "Edit author"s,
                    std::bind(&View::EditAuthor, this, ph::_1));
    menu_.AddAction("DeleteBook"s, "[title]"s, "Delete book"s,
                    std::bind(&View::DeleteBook, this, ph::_1));
    menu_.AddAction("EditBook"s, "[title]"s, "Edit book"s,
                    std::bind(&View::EditBook, this, ph::_1));
    menu_.AddAction("ShowBook"s, "[title]"s, "Show book details"s,
                    std::bind(&View::ShowBook, this, ph::_1));
}

bool View::AddAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        use_cases_.AddAuthor(std::move(name));
    } catch (const std::exception&) {
        output_ << "Failed to add author"sv << std::endl;
    }
    return true;
}

bool View::AddBook(std::istream& cmd_input) const {
    try {
        if (auto params = GetBookParams(cmd_input)) {
            app::BookDto dto;
            dto.title = params->title;
            dto.publication_year = params->publication_year;
            dto.author_id = params->author_id;
            dto.tags = params->tags;
            use_cases_.AddBook(dto);
        } else {
            output_ << "Failed to add book"sv << std::endl;
        }
    } catch (const std::exception&) {
        output_ << "Failed to add book"sv << std::endl;
    }
    return true;
}

bool View::ShowAuthors() const {
    PrintVector(output_, GetAuthors());
    return true;
}

bool View::ShowBooks() const {
    PrintVector(output_, GetBooks());
    return true;
}

bool View::ShowAuthorBooks() const {
    try {
        if (auto author_id = SelectAuthor()) {
            PrintVector(output_, GetAuthorBooks(*author_id));
        }
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to Show Books");
    }
    return true;
}

bool View::DeleteAuthor(std::istream& cmd_input) const {
    std::string name;
    std::getline(cmd_input, name);
    boost::algorithm::trim(name);
    
    std::optional<std::string> author_id;
    if (!name.empty()) {
        author_id = SelectAuthorByName(name);
        if (!author_id.has_value()) {
            output_ << "Failed to delete author"sv << std::endl;
            return true;
        }
    } else {
        author_id = SelectAuthor();
        if (!author_id.has_value()) {
            return true;
        }
    }
    
    try {
        use_cases_.DeleteAuthor(*author_id);
        // При успешном удалении ничего не выводим
    } catch (const std::exception&) {
        // При любой ошибке выводим сообщение
        output_ << "Failed to delete author"sv << std::endl;
    }
    return true;
}

bool View::EditAuthor(std::istream& cmd_input) const {
    try {
        std::string name;
        std::getline(cmd_input, name);
        boost::algorithm::trim(name);
        
        std::optional<std::string> author_id;
        if (!name.empty()) {
            author_id = SelectAuthorByName(name);
            if (!author_id.has_value()) {
                output_ << "Failed to edit author"sv << std::endl;
                return true;
            }
        } else {
            author_id = SelectAuthor();
            if (!author_id.has_value()) {
                return true;
            }
        }
        
        output_ << "Enter new name:" << std::endl;
        std::string new_name;
        if (!std::getline(input_, new_name)) {
            return true;
        }
        boost::algorithm::trim(new_name);
        
        use_cases_.EditAuthor(*author_id, new_name);
    } catch (const std::exception&) {
        output_ << "Failed to edit author"sv << std::endl;
    }
    return true;
}

bool View::DeleteBook(std::istream& cmd_input) const {
    std::string title;
    std::getline(cmd_input, title);
    boost::algorithm::trim(title);
    
    std::optional<std::string> book_id;
    if (!title.empty()) {
        auto books_dto = use_cases_.GetBooksByTitle(title);
        if (books_dto.empty()) {
            output_ << "Book not found"sv << std::endl;
            return true;
        }
        if (books_dto.size() == 1) {
            book_id = books_dto[0].id;
        } else {
            book_id = SelectBook(title);
            if (!book_id.has_value()) {
                return true;
            }
        }
    } else {
        book_id = SelectBook();
        if (!book_id.has_value()) {
            return true;
        }
    }
    
    try {
        use_cases_.DeleteBook(*book_id);
        // При успешном удалении ничего не выводим
    } catch (const std::runtime_error& e) {
        std::string error_msg = e.what();
        if (error_msg == "Book not found") {
            output_ << "Book not found"sv << std::endl;
        } else {
            output_ << "Failed to delete book"sv << std::endl;
        }
    } catch (const std::exception&) {
        output_ << "Failed to delete book"sv << std::endl;
    }
    return true;
}

bool View::EditBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        std::optional<std::string> book_id;
        if (!title.empty()) {
            book_id = SelectBook(title);
            if (!book_id.has_value()) {
                // Если книга не найдена или выбор отменен, выводим "Book not found" только если книга действительно не найдена
                // Проверяем, была ли книга найдена в базе
                auto books_dto = use_cases_.GetBooksByTitle(title);
                if (books_dto.empty()) {
                    output_ << "Book not found"sv << std::endl;
                }
                // Если книги найдены, но выбор отменен, ничего не выводим
                return true;
            }
        } else {
            book_id = SelectBook();
            if (!book_id.has_value()) {
                // При отмене выбора книги ничего не выводим и не читаем дальнейший ввод
                return true;
            }
        }
        
        auto book = use_cases_.GetBookById(*book_id);
        if (!book) {
            output_ << "Book not found"sv << std::endl;
            return true;
        }
        
        output_ << "Enter new title or empty line to use the current one (" << book->title << "):" << std::endl;
        std::string new_title;
        std::getline(input_, new_title);
        boost::algorithm::trim(new_title);
        
        output_ << "Enter publication year or empty line to use the current one (" << book->publication_year << "):" << std::endl;
        std::string year_str;
        std::getline(input_, year_str);
        boost::algorithm::trim(year_str);
        
        std::ostringstream tags_stream;
        tags_stream << "Enter tags (current tags: ";
        if (!book->tags.empty()) {
            for (size_t i = 0; i < book->tags.size(); ++i) {
                if (i > 0) tags_stream << ", ";
                tags_stream << book->tags[i];
            }
        }
        tags_stream << "):" << std::endl;
        output_ << tags_stream.str();
        
        std::string tags_input;
        std::getline(input_, tags_input);
        
        std::optional<std::string> final_title = new_title.empty() ? std::nullopt : std::make_optional(new_title);
        std::optional<int> final_year;
        if (!year_str.empty()) {
            try {
                final_year = std::stoi(year_str);
            } catch (...) {
                final_year = std::nullopt;
            }
        }
        
        std::optional<std::vector<std::string>> final_tags;
        if (!tags_input.empty()) {
            final_tags = NormalizeTags(tags_input);
        } else {
            // Пустая строка означает удаление всех тегов (пустой вектор)
            final_tags = std::vector<std::string>();
        }
        
        use_cases_.EditBook(*book_id, final_title, final_year, final_tags);
    } catch (const std::exception&) {
        output_ << "Book not found"sv << std::endl;
    }
    return true;
}

bool View::ShowBook(std::istream& cmd_input) const {
    try {
        std::string title;
        std::getline(cmd_input, title);
        boost::algorithm::trim(title);
        
        std::optional<std::string> book_id;
        if (!title.empty()) {
            book_id = SelectBook(title);
        } else {
            book_id = SelectBook();
        }
        
        if (!book_id.has_value()) {
            return true;
        }
        
        auto book = use_cases_.GetBookById(*book_id);
        if (!book) {
            return true;
        }
        
        output_ << "Title: " << book->title << std::endl;
        output_ << "Author: " << book->author_name << std::endl;
        output_ << "Publication year: " << book->publication_year << std::endl;
        if (!book->tags.empty()) {
            output_ << "Tags: ";
            for (size_t i = 0; i < book->tags.size(); ++i) {
                if (i > 0) output_ << ", ";
                output_ << book->tags[i];
            }
            output_ << std::endl;
        }
    } catch (const std::exception&) {
    }
    return true;
}

std::optional<detail::AddBookParams> View::GetBookParams(std::istream& cmd_input) const {
    detail::AddBookParams params;

    cmd_input >> params.publication_year;
    std::getline(cmd_input, params.title);
    boost::algorithm::trim(params.title);

    output_ << "Enter author name or empty line to select from list:" << std::endl;
    std::string author_input;
    std::getline(input_, author_input);
    boost::algorithm::trim(author_input);
    
    std::optional<std::string> author_id;
    if (!author_input.empty()) {
        auto author = use_cases_.GetAuthorByName(author_input);
        if (!author) {
            output_ << "No author found. Do you want to add " << author_input << " (y/n)?" << std::endl;
            std::string answer;
            std::getline(input_, answer);
            boost::algorithm::trim(answer);
            if (answer == "y" || answer == "Y") {
                use_cases_.AddAuthor(author_input);
                author = use_cases_.GetAuthorByName(author_input);
                if (author) {
                    author_id = author->id;
                }
            } else {
                return std::nullopt;
            }
        } else {
            author_id = author->id;
        }
    } else {
        author_id = SelectAuthor();
    }
    
    if (!author_id.has_value()) {
        // Читаем и выбрасываем теги из буфера, чтобы они не интерпретировались как команды
        std::string dummy;
        std::getline(input_, dummy);
        return std::nullopt;
    }
    params.author_id = *author_id;
    
    output_ << "Enter tags (comma separated):" << std::endl;
    std::string tags_input;
    if (!std::getline(input_, tags_input)) {
        return std::nullopt;
    }
    params.tags = NormalizeTags(tags_input);
    
    return params;
}

std::optional<std::string> View::SelectAuthor(const std::string& prompt_name) const {
    if (prompt_name.empty()) {
        output_ << "Select author:" << std::endl;
    } else {
        output_ << prompt_name << std::endl;
    }
    auto authors = GetAuthors();
    PrintVector(output_, authors);
    output_ << "Enter author # or empty line to cancel" << std::endl;

    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }

    int author_idx;
    try {
        author_idx = std::stoi(str);
    } catch (std::exception const&) {
        return std::nullopt;
    }

    --author_idx;
    if (author_idx < 0 || author_idx >= static_cast<int>(authors.size())) {
        return std::nullopt;
    }

    return authors[author_idx].id;
}

std::optional<std::string> View::SelectAuthorByName(const std::string& name) const {
    auto author = use_cases_.GetAuthorByName(name);
    if (!author) {
        return std::nullopt;
    }
    return author->id;
}

std::optional<std::string> View::SelectBook(const std::string& title) const {
    std::vector<detail::BookInfo> books;
    if (!title.empty()) {
        auto books_dto = use_cases_.GetBooksByTitle(title);
        if (books_dto.empty()) {
            return std::nullopt;
        }
        if (books_dto.size() == 1) {
            return books_dto[0].id;
        }
        for (const auto& b : books_dto) {
            books.push_back({b.id, b.title, b.author_name, b.publication_year});
        }
    } else {
        books = GetBooks();
    }
    
    if (books.empty()) {
        return std::nullopt;
    }
    
    if (books.size() == 1 && !title.empty()) {
        return books[0].id;
    }
    
    PrintVector(output_, books);
    output_ << "Enter the book # or empty line to cancel:" << std::endl;
    
    std::string str;
    if (!std::getline(input_, str) || str.empty()) {
        return std::nullopt;
    }
    
    int book_idx;
    try {
        book_idx = std::stoi(str);
    } catch (std::exception const&) {
        return std::nullopt;
    }
    
    --book_idx;
    if (book_idx < 0 || book_idx >= static_cast<int>(books.size())) {
        return std::nullopt;
    }
    
    return books[book_idx].id;
}

std::vector<detail::AuthorInfo> View::GetAuthors() const {
    std::vector<detail::AuthorInfo> dst_autors;
    for (const auto& a : use_cases_.GetAuthors()) {
        dst_autors.push_back({a.id, a.name});
    }
    return dst_autors;
}

std::vector<detail::BookInfo> View::GetBooks() const {
    std::vector<detail::BookInfo> books;
    for (const auto& b : use_cases_.GetBooks()) {
        books.push_back({b.id, b.title, b.author_name, b.publication_year});
    }
    return books;
}

std::vector<detail::BookInfo> View::GetAuthorBooks(const std::string& author_id) const {
    std::vector<detail::BookInfo> books;
    for (const auto& b : use_cases_.GetAuthorBooks(author_id)) {
        books.push_back({b.id, b.title, b.author_name, b.publication_year});
    }
    return books;
}

std::vector<std::string> View::NormalizeTags(const std::string& tags_input) const {
    std::vector<std::string> result;
    std::set<std::string> seen;
    
    std::istringstream stream(tags_input);
    std::string tag;
    
    while (std::getline(stream, tag, ',')) {
        std::string normalized = NormalizeTag(tag);
        if (!normalized.empty() && seen.find(normalized) == seen.end()) {
            result.push_back(normalized);
            seen.insert(normalized);
        }
    }
    
    std::sort(result.begin(), result.end());
    return result;
}

std::string View::NormalizeTag(const std::string& tag) const {
    std::string result = tag;
    boost::algorithm::trim(result);
    
    if (result.empty()) {
        return "";
    }
    
    std::regex multiple_spaces("\\s+");
    result = std::regex_replace(result, multiple_spaces, " ");
    
    return result;
}

}  // namespace ui

