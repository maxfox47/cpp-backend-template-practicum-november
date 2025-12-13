#pragma once
#include "../domain/author_fwd.h"
#include "../domain/book_fwd.h"
#include "use_cases.h"

namespace app {

class UseCasesImpl : public UseCases {
public:
    UseCasesImpl(domain::AuthorRepository& authors, domain::BookRepository& books)
        : authors_{authors}
        , books_{books} {
    }

    void AddAuthor(const std::string& name) override;
    void AddBook(const BookDto& book) override;
    std::vector<AuthorDto> GetAuthors() const override;
    std::vector<BookDto> GetBooks() const override;
    std::vector<BookDto> GetAuthorBooks(const std::string& author_id) const override;
    void DeleteAuthor(const std::string& author_id) override;
    void EditAuthor(const std::string& author_id, const std::string& new_name) override;
    std::optional<AuthorDto> GetAuthorByName(const std::string& name) const override;
    void DeleteBook(const std::string& book_id) override;
    void EditBook(const std::string& book_id, const std::optional<std::string>& new_title,
                  const std::optional<int>& new_year,
                  const std::optional<std::vector<std::string>>& new_tags) override;
    std::vector<BookDto> GetBooksByTitle(const std::string& title) const override;
    std::optional<BookDto> GetBookById(const std::string& book_id) const override;

private:
    domain::AuthorRepository& authors_;
    domain::BookRepository& books_;
};

}  // namespace app

