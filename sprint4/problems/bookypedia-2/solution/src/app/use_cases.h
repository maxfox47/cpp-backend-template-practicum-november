#pragma once

#include <optional>
#include <string>
#include <vector>

#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"

namespace app {

struct AuthorDto {
    std::string id;
    std::string name;
};

struct BookDto {
    std::string id;
    std::string author_id;
    std::string author_name;
    std::string title;
    int publication_year = 0;
    std::vector<std::string> tags;
};

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const BookDto& book) = 0;
    virtual std::vector<AuthorDto> GetAuthors() const = 0;
    virtual std::vector<BookDto> GetBooks() const = 0;
    virtual std::vector<BookDto> GetAuthorBooks(const std::string& author_id) const = 0;
    virtual void DeleteAuthor(const std::string& author_id) = 0;
    virtual void EditAuthor(const std::string& author_id, const std::string& new_name) = 0;
    virtual std::optional<AuthorDto> GetAuthorByName(const std::string& name) const = 0;
    virtual void DeleteBook(const std::string& book_id) = 0;
    virtual void EditBook(const std::string& book_id, const std::optional<std::string>& new_title,
                          const std::optional<int>& new_year,
                          const std::optional<std::vector<std::string>>& new_tags) = 0;
    virtual std::vector<BookDto> GetBooksByTitle(const std::string& title) const = 0;
    virtual std::optional<BookDto> GetBookById(const std::string& book_id) const = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app

