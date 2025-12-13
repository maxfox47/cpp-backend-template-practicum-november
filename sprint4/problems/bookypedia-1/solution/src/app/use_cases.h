#pragma once

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
    std::string title;
    int publication_year = 0;
};

class UseCases {
public:
    virtual void AddAuthor(const std::string& name) = 0;
    virtual void AddBook(const BookDto& book) = 0;
    virtual std::vector<AuthorDto> GetAuthors() const = 0;
    virtual std::vector<BookDto> GetBooks() const = 0;
    virtual std::vector<BookDto> GetAuthorBooks(const std::string& author_id) const = 0;

protected:
    ~UseCases() = default;
};

}  // namespace app

